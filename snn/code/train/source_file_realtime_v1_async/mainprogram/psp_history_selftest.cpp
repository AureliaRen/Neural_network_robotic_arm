#include "../source_file_realtime_v1_async/mainprogram/inc/Fig4ClosedLoopNetwork.h"
#include "../source_file_realtime_v1_async/SynModel/inc/non_int/PaperPSPHistory.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <iostream>
static double alpha(double x){return x>0?x/5*std::exp(1-x/5):0;}
int main() {try {
 auto sim=fig4::build_network({});sim->InitSimulation();auto* rule=fig4::get_rule(*sim);
 auto* c=rule->plastic[0];auto* s=c->SourceNeuron->neuron_state_vector;int n=c->SourceNeuron->index_in_NeuronModel;
 long long checks=0;
 for(int type:{4,5})for(float sign:{-1.0f,1.0f})for(int d:{10,200,500})
 for(int history:{0,1,2})for(bool cw:{false,true}) {
  if(history==2&&!cw)continue;
  PaperPSPHistory p(1);p.setParameters({{"history",history},{"current_weight",cw},{"dt",.1f}});p.InitSynState(1);
  c->TargetNeuronModelIndex=0;c->type=type;c->delay=d;
  struct A {int step;double w;};std::vector<A> arrivals;
  int latest=-1;
  for(int t=0;t<=1500;++t) {
   if(t==100||t==300)latest=t;
   s->LastUpdate[n]=t;s->LastSpike[n]=latest<0?10000+t:t-latest;
   c->weight=sign*(t<700?.5f:.8f);double w=(type==5?-1:1)*c->weight;
   if(t==100+d||t==300+d){p.OnArrival(c,t,static_cast<float>(w));arrivals.push_back({t,w});}
   p.BeginStep(t);double actual=p.GetCurrent(nullptr,0),expected=0;
   if(history==2) {if(latest>=0)expected=w*alpha((t-latest-d)*static_cast<double>(.1f));}
   else for(size_t a=0;a<arrivals.size();++a) {
    if(history==1&&a+1!=arrivals.size())continue;
    expected+=(cw?w:arrivals[a].w)*alpha((t-arrivals[a].step)*static_cast<double>(.1f));
   }
   if(std::fabs(actual-expected)>2e-6)throw std::runtime_error("History/weight/sign/current mismatch");++checks;
  }
 }
 bool threw=false;try {PaperPSPHistory p(1);p.ProcessInput(c,nullptr,.5f);}catch(const std::runtime_error&){threw=true;}
 if(!threw)throw std::runtime_error("Missing arrival hook went undetected");++checks;
 std::cout<<"PSP_HISTORY_SELFTEST PASS checks="<<checks<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
