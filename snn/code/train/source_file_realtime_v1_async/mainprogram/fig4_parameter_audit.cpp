// Read effective parameters after the actual factory/initialization path.
#include "inc/Fig4ClosedLoopNetwork.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include <filesystem>
#include <iostream>

using namespace fig4;
int main(int argc,char** argv) {
    try {
        if(argc!=2)throw std::runtime_error("Usage: fig4_parameter_audit output_directory");
        std::string out=argv[1];std::filesystem::create_directories(out);
        NetworkSettings cfg;auto sim=build_network(cfg);sim->InitSimulation();
        std::ofstream csv(out+"/effective_parameters.csv");
        csv<<"neuron,type,requested_a,requested_b,requested_c,requested_d,effective_a,effective_b,effective_c,effective_d,effective_V_reset\n"<<std::setprecision(9);
        for(int n=2;n<12;++n) {
            auto p=sim->network->neurons[n].neuron_model->getParameters();
            const char* type=n<4?"RS":n<8?"CH":"LTS";
            float a=.02f,b=n<8?.2f:.25f,c=n>=4&&n<8?-50.0f:-65.0f,d=n<4?8.0f:2.0f;
            csv<<n<<','<<type<<','<<a<<','<<b<<','<<c<<','<<d;
            for(const char* key : {"a","b","c","d","V_reset"})csv<<','<<boost::any_cast<float>(p.at(key));
            csv<<'\n';
        }
        // Drive a CH directly through its existing Type-3 input. RS stays silent.
        sim->AddExternalCurrentActivity({0},{12},{10.0f});
        std::ofstream trace(out+"/ch_direct_trace.csv"),spikes(out+"/ch_direct_spikes.csv");
        trace<<"step,v,u\n"<<std::setprecision(9);spikes<<"spike_step,observed_v_after_step\n"<<std::setprecision(9);
        auto& neuron=sim->network->neurons[4];
        for(int s=0;s<3000;++s) {
            sim->RunSimulationStep(1);int t,n;
            auto* state=neuron.neuron_state_vector->GetNeuronState(neuron.index_in_NeuronModel);
            trace<<sim->currenttime[0]<<','<<state[0]<<','<<state[1]<<'\n';
            while(sim->output_spike_driver->RemoveBufferedSpike(t,n))if(n==4)spikes<<t<<','<<state[0]<<'\n';
        }
        std::cout<<"PARAMETER AUDIT WRITTEN\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
