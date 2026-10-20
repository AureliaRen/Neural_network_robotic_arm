#ifndef FIG4_EFFECTIVE_PARAMETER_AUDIT_H
#define FIG4_EFFECTIVE_PARAMETER_AUDIT_H
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/Network.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include <boost/any.hpp>
#include <stdexcept>
namespace fig4 {
inline void require_paper_ch(Simulation& sim,int first,int count) {
    for(int n=first;n<first+count;++n) {
        auto p=sim.network->neurons[n].neuron_model->getParameters();
        if(boost::any_cast<float>(p.at("a"))!=.02f || boost::any_cast<float>(p.at("b"))!=.20f ||
           boost::any_cast<float>(p.at("c"))!=-50.0f || boost::any_cast<float>(p.at("d"))!=2.0f ||
           boost::any_cast<float>(p.at("V_reset"))!=-50.0f)
            throw std::runtime_error("CH effective parameters differ from paper; check reset alias");
    }
}
}
#endif
