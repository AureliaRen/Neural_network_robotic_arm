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
        // Same topology as tutorial_synapse_network: InputSpike(0) -> pre(1) -> post(2)
        // but only ONE external input spike, to observe the decaying burst.
        std::list<NeuronLayerDescription> layers;
        NeuronLayerDescription input;
        input.ModelName = "InputSpikeNeuronModel";
        input.numberofneuron = 1;
        layers.push_back(input);

        NeuronLayerDescription pre;
        pre.ModelName = "TimeDrivenLIF_Exponential_Decay";
        pre.numberofneuron = 1;
        pre.isMonitored = true;
        pre.isOutput = true;
        pre.NeuronParameter["V_th"] = -55.0f;
        layers.push_back(pre);

        NeuronLayerDescription post = pre;
        layers.push_back(post);

        std::list<ConnectionDescription> connections;
        ConnectionDescription input_to_pre;
        input_to_pre.SourceNeuron = {0};
        input_to_pre.TargetNeuron = {1};
        input_to_pre.Type = {0};
        input_to_pre.Weight = {20.0f};
        input_to_pre.MaxWeight = {20.0f};
        input_to_pre.Delay = {1};
        input_to_pre.SynapseRule = {-1};
        input_to_pre.TriggerRule = {-1};
        input_to_pre.SynapseModel = {ModelDescription{}};
        connections.push_back(input_to_pre);

        ConnectionDescription pre_to_post;
        pre_to_post.SourceNeuron = {1};
        pre_to_post.TargetNeuron = {2};
        pre_to_post.Type = {0};
        pre_to_post.Weight = {20.0f};
        pre_to_post.MaxWeight = {20.0f};
        pre_to_post.Delay = {5};
        pre_to_post.SynapseRule = {-1};
        pre_to_post.TriggerRule = {-1};
        pre_to_post.SynapseModel = {ModelDescription{}};
        connections.push_back(pre_to_post);

        std::list<LearningRuleDescription> learning_rules;

        std::filesystem::create_directories("tutorial_output/single_input");

        Simulation simulation(layers, connections, learning_rules, 1000, 0.1f, 1);
        simulation.AddFileOutputSpikeDriver("tutorial_output/single_input/spikes.txt");
        simulation.AddFileOutputMonitorDriver("tutorial_output/single_input/monitor.txt");

        simulation.InitSimulation();
        std::vector<int> input_times = {10};
        std::vector<int> input_neurons(input_times.size(), 0);
        simulation.AddExternalSpikeActivity(input_times, input_neurons);
        simulation.RunSimulationStep(1000);

        std::cout << "Single input tutorial finished." << std::endl;
        std::cout << "Results: tutorial_output/single_input" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "tutorial_single_input exception: " << e.what() << std::endl;
        return 1;
    }
}
