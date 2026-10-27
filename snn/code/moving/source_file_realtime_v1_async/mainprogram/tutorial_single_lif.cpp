#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>
#include <iostream>
#include <filesystem>
#include <list>

int main() {
    try {
        // Neuron layers: LIF(0) + current input(1)
        std::list<NeuronLayerDescription> layers;
        NeuronLayerDescription lif;
        lif.ModelName = "TimeDrivenLIF_Exponential_Decay";
        lif.numberofneuron = 1;
        lif.isMonitored = true;
        lif.isOutput = true;

        lif.NeuronParameter["V_rest"] = -65.0f;
        lif.NeuronParameter["tau"] = 20.0f;
        lif.NeuronParameter["V_th"] = -55.0f;
        lif.NeuronParameter["R"] = 1.0f;
        lif.NeuronParameter["V_reset"] = -65.0f;
        lif.NeuronParameter["t_ref"] = 50;
        layers.push_back(lif);

        NeuronLayerDescription current_input;
        current_input.ModelName = "InputCurrentNeuronModel";
        current_input.numberofneuron = 1;
        layers.push_back(current_input);

        // Current connection (Type 3) into the LIF neuron
        std::list<ConnectionDescription> connections;
        ConnectionDescription current_to_lif;
        current_to_lif.SourceNeuron = {1};
        current_to_lif.TargetNeuron = {0};
        current_to_lif.Type = {3};
        current_to_lif.Weight = {1.0f};
        current_to_lif.MaxWeight = {1.0f};
        current_to_lif.Delay = {1};
        current_to_lif.SynapseRule = {-1};
        current_to_lif.TriggerRule = {-1};
        current_to_lif.SynapseModel = {ModelDescription{}};
        connections.push_back(current_to_lif);

        std::list<LearningRuleDescription> learning_rules;

        std::filesystem::create_directories("tutorial_output/single_lif");

        Simulation simulation(layers, connections, learning_rules, 1000, 0.1f, 1);
        simulation.AddFileOutputSpikeDriver("tutorial_output/single_lif/spikes.txt");
        simulation.AddFileOutputMonitorDriver("tutorial_output/single_lif/monitor.txt");

        simulation.InitSimulation();
        simulation.AddExternalCurrentActivity({0}, {1}, {22.5f});
        simulation.RunSimulationStep(1000);

        std::cout << "Single LIF tutorial finished." << std::endl;
        std::cout << "Results: tutorial_output/single_lif" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "tutorial_single_lif exception: " << e.what() << std::endl;
        return 1;
    }
}
