// A contradictory legacy alias must be detected by the paper parameter guard.
#include "inc/Fig4ClosedLoopNetwork.h"
#include "inc/Fig4EffectiveParameterAudit.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDriven/TimeDrivenIzhikevic_Exponential_Decay.h"
#include <iostream>
int main() {
    try {
        fig4::NetworkSettings cfg;auto sim=fig4::build_network(cfg);sim->InitSimulation();
        fig4::require_paper_ch(*sim,4,4);
        auto* model=dynamic_cast<TimeDrivenIzhikevic_I_Exponential_Decay*>(sim->network->neurons[4].neuron_model);
        if(!model)throw std::runtime_error("Unexpected factory model");
        model->SetParameters({{"c",-50.0f},{"V_reset",-65.0f}},fig4::DT);
        bool caught=false;
        try{fig4::require_paper_ch(*sim,4,4);}catch(const std::runtime_error&){caught=true;}
        if(!caught)throw std::runtime_error("Conflicting reset alias went undetected");
        model->SetParameters({{"c",-50.0f},{"V_reset",-50.0f}},fig4::DT);
        fig4::require_paper_ch(*sim,4,4);fig4::get_rule(*sim);
        std::cout<<"RESET ALIAS GUARD PASS\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}
}
