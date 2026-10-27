#include "../source_file_realtime_v1_async/SynModel/inc/non_int/PaperPSPHistory.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <cmath>
#include <stdexcept>
void PaperPSPHistory::advance(Entry& e,int step) {
 if(step<e.step)throw std::runtime_error("PSP history moved backwards");
 double dt=(step-e.step)*static_cast<double>(dt_),decay=std::exp(-dt/tau_);
 e.g=(e.g+e.h*dt)*decay;e.h*=decay;e.step=step;
}
void PaperPSPHistory::OnArrival(Interconnections* c,int step,float weight) {
 auto& es=entries_.at(c->TargetNeuronModelIndex);
 Entry* found=nullptr;
 for(auto& e:es)if(e.c==c){found=&e;break;}
 if(!found){es.push_back({c,0,0,step});found=&es.back();}
 advance(*found,step);
 if(history_==1){found->g=0;found->h=0;}
 if(history_!=2)found->h+=currentWeight_?1.0:weight;
}
void PaperPSPHistory::ProcessInput(Interconnections*,float*,float) {
 throw std::runtime_error("PSP history needs explicit arrival timestamp hook");
}
float PaperPSPHistory::GetCurrent(float*,int index) {
 double total=0;
 for(auto& e:entries_.at(index)) {
  double sign=e.c->type==5?-1.0:1.0;
  double w=sign*e.c->weight;
  if(history_==2) {
   auto* source=e.c->SourceNeuron;auto* s=source->neuron_state_vector;
   if(!s||!s->TimeDriven||source->neuron_model->getTimestepSize()!=1)
    throw std::runtime_error("Latest-source PSP supports one-step CPU time-driven sources only");
   int n=source->index_in_NeuronModel,last=s->LastUpdate[n]-s->LastSpike[n];
   e.observedSource=last;
   if(last>currentStep_)throw std::runtime_error("PSP observed a future source spike");
   double x=(currentStep_-last-e.c->delay)*static_cast<double>(dt_);
   if(last>=0&&x>0)total+=w*x/tau_*std::exp(1-x/tau_);
  }else {
   advance(e,currentStep_);
   total+=std::exp(1.0)*e.g/tau_*(currentWeight_?w:1.0);
  }
 }
 lastCurrent_.at(index)=static_cast<float>(total);return lastCurrent_.at(index);
}
void PaperPSPHistory::setParameters(std::map<std::string,boost::any> p) {
 if(p.count("tau"))tau_=boost::any_cast<float>(p.at("tau"));
 if(p.count("dt"))dt_=boost::any_cast<float>(p.at("dt"));
 if(p.count("history"))history_=boost::any_cast<int>(p.at("history"));
 if(p.count("current_weight"))currentWeight_=boost::any_cast<bool>(p.at("current_weight"));
 if(tau_<=0||dt_<=0||!std::isfinite(tau_)||!std::isfinite(dt_)||history_<0||history_>2||
    (history_==2&&!currentWeight_))throw std::runtime_error("Bad PSP history parameters");
}
std::map<std::string,boost::any> PaperPSPHistory::getParameters() {
 return {{"tau",tau_},{"dt",dt_},{"history",history_},{"current_weight",currentWeight_}};
}
bool PaperPSPHistory::compare(Synmodel* s) {
 auto* p=dynamic_cast<PaperPSPHistory*>(s);
 return p&&tau_==p->tau_&&dt_==p->dt_&&history_==p->history_&&currentWeight_==p->currentWeight_;
}
