#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>
#include <iostream>
#include <filesystem>
#include <list>
#include <vector>

int main() {
    try {
        // Neuron layers: LIF pre(0), LIF post(1), current input(2), current input(3)
        std::list<NeuronLayerDescription> layers;
        NeuronLayerDescription pre;
        pre.ModelName = "TimeDrivenLIF_Exponential_Decay";
        pre.numberofneuron = 1;
        pre.isMonitored = true;
        pre.isOutput = true;
        pre.NeuronParameter["V_th"] = -55.0f;
        layers.push_back(pre);

        NeuronLayerDescription post = pre;
        layers.push_back(post);

        NeuronLayerDescription current_a;
        current_a.ModelName = "InputCurrentNeuronModel";
        current_a.numberofneuron = 1;
        layers.push_back(current_a);

        NeuronLayerDescription current_b = current_a;
        layers.push_back(current_b);

        // Plastic pre -> post synapse referencing learning rule index 0
        std::list<ConnectionDescription> connections;
        ConnectionDescription plastic;
        plastic.SourceNeuron = {0};
        plastic.TargetNeuron = {1};
        plastic.Type = {0};
        plastic.Weight = {1.0f};
        plastic.MaxWeight = {10.0f};
        plastic.Delay = {5};
        plastic.SynapseRule = {0};
        plastic.TriggerRule = {-1};
        plastic.SynapseModel = {ModelDescription{}};
        connections.push_back(plastic);

        // Current inputs to both LIF layers (Type 3 = external current)
        ConnectionDescription current_to_pre;
        current_to_pre.SourceNeuron = {2};
        current_to_pre.TargetNeuron = {0};
        current_to_pre.Type = {3};
        current_to_pre.Weight = {1.0f};
        current_to_pre.MaxWeight = {1.0f};
        current_to_pre.Delay = {1};
        current_to_pre.SynapseRule = {-1};
        current_to_pre.TriggerRule = {-1};
        current_to_pre.SynapseModel = {ModelDescription{}};
        connections.push_back(current_to_pre);

        ConnectionDescription current_to_post;
        current_to_post.SourceNeuron = {3};
        current_to_post.TargetNeuron = {1};
        current_to_post.Type = {3};
        current_to_post.Weight = {1.0f};
        current_to_post.MaxWeight = {1.0f};
        current_to_post.Delay = {1};
        current_to_post.SynapseRule = {-1};
        current_to_post.TriggerRule = {-1};
        current_to_post.SynapseModel = {ModelDescription{}};
        connections.push_back(current_to_post);

        // STDP learning rule (index 0)
        std::list<LearningRuleDescription> learning_rules;
        LearningRuleDescription stdp;
        stdp.RuleName = "STDP";
        stdp.RuleParameter["Max_LTP"] = 0.92f;
        stdp.RuleParameter["LTP_tau"] = 16.8f;
        stdp.RuleParameter["Max_LTD"] = 0.53f;
        stdp.RuleParameter["LTD_tau"] = 33.1f;
        learning_rules.push_back(stdp);

        std::filesystem::create_directories("tutorial_output/stdp");

        Simulation simulation(layers, connections, learning_rules, 1000, 0.1f, 1);
        simulation.AddFileOutputSpikeDriver("tutorial_output/stdp/spikes.txt");
        simulation.AddFileOutputMonitorDriver("tutorial_output/stdp/monitor.txt");
        simulation.AddFileOutputWeightDriver("tutorial_output/stdp/weights.txt", 100);

        simulation.InitSimulation();
        simulation.AddExternalCurrentActivity({0, 80}, {2, 3}, {22.5f, 22.5f});
        const float initial_weight = simulation.network->inters[0].weight;
        simulation.RunSimulationStep(1000);
        const float final_weight = simulation.network->inters[0].weight;

        std::cout << "STDP tutorial finished." << std::endl;
        std::cout << "Initial weight: " << initial_weight << std::endl;
        std::cout << "Final weight: " << final_weight << std::endl;
        std::cout << "Results: tutorial_output/stdp" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "tutorial_stdp exception: " << e.what() << std::endl;
        return 1;
    }
}
