#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/DASTDP.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenIzhikevic_Exponential_Decay.h"
#include "../source_file_realtime_v1_async/SynModel/inc/Synmodel.h"
#include "../source_file_realtime_v1_async/SynModel/inc/int/PaperPSP.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <iostream>

static void check(bool ok,const char* message) {
    std::cout << (ok?"PASS ":"FAIL ") << message << '\n';
    if (!ok) throw std::runtime_error(message);
}

int main() {
    try {
        NeuronLayerDescription input; input.ModelName="InputSpikeNeuronModel"; input.numberofneuron=1;
        NeuronLayerDescription output; output.ModelName="TimeDrivenIzhikevic_Exponential_Decay"; output.numberofneuron=2;
        ModelDescription psp; psp.ModelName="PaperPSP"; psp.ModelParameter["tau"]=5.0f;
        std::list<ConnectionDescription> connections;
        for(int target=1;target<=2;++target) {
            ConnectionDescription c;
            c.SourceNeuron={0}; c.TargetNeuron={target}; c.Type={4}; c.Weight={0.5f};
            c.MaxWeight={2.0f}; c.Delay={10}; c.SynapseRule={0}; c.TriggerRule={-1}; c.SynapseModel={psp};
            connections.push_back(c);
        }
        LearningRuleDescription description; description.RuleName="DA-STDP";
        Simulation sim({input,output},connections,{description},1000,0.1f,1);
        sim.InitSimulation();
        auto* rule=dynamic_cast<DASTDP*>(sim.network->LearningRules[0]); rule->Collect(sim.network);
        auto* first=rule->plastic[0]; auto* second=rule->plastic[1];
        int i=first->LearningRuleIndex_withPost, j=second->LearningRuleIndex_withPost;
        check(i!=j && rule->eligibility.size()==2,"independent unique eligibility indices");
        check(std::fabs(rule->StdpWindow(-12.7f)-1.0f)<1e-6,"Eq.8 peak at -12.7 ms");
        rule->ApplyPostSynaticSpike(first->TargetNeuron,100,&sim);
        rule->ApplyPreSynaticSpike(first,227,&sim);
        check(std::fabs(rule->eligibility[i]-1.0f)<1e-6 && rule->eligibility[j]==0,"known pair adds unit impulse to correct state");
        rule->Step(&sim,0.0005f);
        float expectedC=std::exp(-0.1f/5.0f);
        check(std::fabs(rule->eligibility[i]-expectedC)<1e-7 &&
            std::fabs(first->weight-(0.5f+0.0005f*expectedC*0.1f))<1e-7,"Eq.5/6 decay and millisecond integration");
        float w=first->weight; rule->Step(&sim,0);
        check(first->weight==w && second->weight==0.5f,"d=0 preserves weights exactly");
        rule->emissionClock=true; rule->eligibility.assign(2,0);
        rule->ApplyPreSynaticSpike(first,300,&sim);
        check(rule->eligibility[i]==0,"emission mode excludes arrival callback");
        rule->ApplySourceSpike(first->SourceNeuron,227,&sim);
        check(std::fabs(rule->eligibility[i]-1)<1e-6 && rule->eligibility[j]==0,"emission callback injects once, respects target history");
        rule->sourceDelayReference=true;rule->eligibility.assign(2,0);
        rule->ApplySourceSpike(first->SourceNeuron,217,&sim);
        check(std::fabs(rule->eligibility[i]-1)<1e-6 && rule->eligibility[j]==0,
            "source-delay hypothesis uses raw+delay once with known past post");
        rule->ApplyPreSynaticSpike(first,227,&sim);
        check(std::fabs(rule->eligibility[i]-1)<1e-6,"source-delay hypothesis skips duplicate arrival injection");
        rule->sourceDelayReference=false;
        rule->eligibility[i]=100000; rule->Step(&sim,1);
        check(first->weight==2,"upper bound clips");
        rule->eligibility[i]=100000; rule->Step(&sim,-1);
        check(first->weight==0,"lower bound clips");
        auto* model=dynamic_cast<TimeDrivenIzhikevic_I_Exponential_Decay*>(first->TargetNeuronModel);
        auto* syn=dynamic_cast<PaperPSP*>(model->SynapseModelVector[first->Synmodel_Index]);
        float* state=syn->GetState(first->TargetNeuronModelIndex);
        state[0]=0;state[1]=0;first->weight=0.25f;model->ProcessSpike(first,0);
        check(std::fabs(state[1]-0.25f)<1e-7,"PSP reads current connection weight at arrival");
        first->weight=0.75f;model->ProcessSpike(first,1);
        check(std::fabs(state[1]-1.0f)<1e-7,"changed weight contributes immediately at next event");
        rule->eligibility[i]=std::numeric_limits<float>::quiet_NaN();
        bool rejected=false;try {rule->Step(&sim,0);}catch(const std::runtime_error&) {rejected=true;}
        check(rejected,"NaN fails loudly rather than being replaced with zero");
        rejected=false;
        try {model->CheckValidIntegeration(123,std::numeric_limits<float>::quiet_NaN());}
        catch(const std::runtime_error&) {rejected=true;}
        check(rejected,"non-finite neuron integration fails before legacy unbounded reset loop");
        std::cout << "DA-STDP AUDIT PASS\n";return 0;
    }catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
