// Actual emission, arrival scheduling and target-derivative current audit.
#include "../source_file_realtime_v1_async/mainprogram/inc/Fig4ClosedLoopNetwork.h"
#include "../source_file_realtime_v1_async/SynModel/inc/non_int/PaperPSPHistory.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenIzhikevic_Exponential_Decay.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <filesystem>
#include <iostream>
using namespace fig4;
static double alpha(double x){return x>0?x/5*std::exp(1-x/5):0;}
int main(int argc,char** argv) {try {
 std::string out=argc>1?argv[1]:"psp_history_event_test";std::filesystem::create_directories(out);
 std::ofstream log(out+"/checks.csv"),trace(out+"/currents.csv"),spikes(out+"/spikes.csv");
 log<<"policy,input_current,checks,max_current_error\n";
 trace<<"policy,input_current,step,target,current_used,expected\n"<<std::setprecision(17);
 spikes<<"policy,input_current,step,neuron\n";
 for(std::string policy:{"sum-arrival","sum-current","latest-arrival","latest-source"})for(float input:{8.8779296875f,22.5f}) {
  NetworkSettings cfg;cfg.pspHistory=policy;cfg.jitter=0;cfg.i_rs=input;
  auto sim=build_network(cfg);sim->InitSimulation();get_rule(*sim);
  sim->AddExternalCurrentActivity({0,0},{0,1},{input,0});
  std::array<std::vector<int>,16> hist;double maxerr=0;long long checks=0;
  for(int k=0;k<10000;++k) {
   sim->RunSimulationStep(1);int t,n;
   while(sim->output_spike_driver->RemoveBufferedSpike(t,n)){
    hist[n].push_back(t);spikes<<policy<<','<<input<<','<<t<<','<<n<<'\n';
   }
   // Model state clocks, not sim.currenttime (which denotes next step).
   for(int target=4;target<12;++target) {
    auto& neuron=sim->network->neurons[target];int index=neuron.index_in_NeuronModel;
    int step=neuron.neuron_state_vector->LastUpdate[index];
    auto* nm=dynamic_cast<TimeDrivenIzhikevic_I_Exponential_Decay*>(neuron.neuron_model);
    for(auto* sm:nm->SynapseModelVector) {
     auto* p=dynamic_cast<PaperPSPHistory*>(sm);if(!p)throw std::runtime_error("Missing alternative synapse");
     double expected=0;
     if(policy=="latest-source") {
      auto observations=p->SourceObservations(index);
      for(auto o:observations) {
       auto* c=o.first;int source=c->SourceNeuron->Neuron_index,last=o.second;
       int inclusive=-1,strict=-1;
       for(int s:hist[source]){if(s<=step)inclusive=s;if(s<step)strict=s;}
       if(last>=0&&last!=inclusive&&last!=strict)throw std::runtime_error("Source history differs from actual spike buffers");
       if(last>=0)expected+=c->weight*alpha((step-last-c->delay)*static_cast<double>(DT));++checks;
      }
     }else {
      // Every connection's arrivals are reconstructed from actual source spikes.
      for(int ci=0;ci<sim->network->intersNum;++ci) {
       auto* c=&sim->network->inters[ci];if(c->TargetNeuron->Neuron_index!=target||c->type!=4)continue;
       int source=c->SourceNeuron->Neuron_index,last=-1;
       for(int s:hist[source]) {
        if(s+c->delay>step)break;
        if(policy=="latest-arrival")last=s+c->delay;
        else expected+=c->weight*alpha((step-s-c->delay)*static_cast<double>(DT));
       }
       if(last>=0)expected+=c->weight*alpha((step-last)*static_cast<double>(DT));
      }
     }
     double err=std::fabs(p->LastCurrent(index)-expected);maxerr=std::max(maxerr,err);++checks;
     if(err>2e-6+2e-7*std::fabs(expected))throw std::runtime_error("Scheduled PSP mismatch policy="+policy+" step="+std::to_string(step)+" target="+std::to_string(target)+" actual="+std::to_string(p->LastCurrent(index))+" expected="+std::to_string(expected));
     if(k%100==0)trace<<policy<<','<<input<<','<<step<<','<<target<<','<<p->LastCurrent(index)<<','<<expected<<'\n';
    }
   }
  }
  log<<policy<<','<<input<<','<<checks<<','<<maxerr<<'\n';
 }
 std::cout<<"PSP_HISTORY_EVENT_TEST PASS 8 cases\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
