#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>
#include <iostream>
#include <filesystem>
#include <list>
#include <vector>
#include <array>

static ConnectionDescription MakeConn(int src, int tgt, int type, float w, float maxw,
                                      int delay, int synRule, int trigRule) {
    ConnectionDescription c;
    c.SourceNeuron = {src};
    c.TargetNeuron = {tgt};
    c.Type = {type};
    c.Weight = {w};
    c.MaxWeight = {maxw};
    c.Delay = {delay};
    c.SynapseRule = {synRule};
    c.TriggerRule = {trigRule};
    c.SynapseModel = {ModelDescription{}};
    return c;
}

int main() {
    try {
        // ===== 1. neuron layers =====
        std::list<NeuronLayerDescription> layers;

        NeuronLayerDescription input;
        input.ModelName = "InputSpikeNeuronModel";
        input.numberofneuron = 1;
        input.isMonitored = false;
        input.isOutput = false;
        layers.push_back(input);

        NeuronLayerDescription output;
        output.ModelName = "TimeDrivenIzhikevic_I_Exponential_Decay";
        output.numberofneuron = 2;
        output.isMonitored = true;
        output.isOutput = true;
        output.NeuronParameter["a"] = 0.02f;
        output.NeuronParameter["b"] = 0.2f;
        output.NeuronParameter["c"] = -50.0f;
        output.NeuronParameter["d"] = 2.0f;
        output.NeuronParameter["V_th"] = 30.0f;
        output.NeuronParameter["V_rest"] = -65.0f;
        output.NeuronParameter["V_reset"] = -65.0f;
        output.NeuronParameter["R"] = 1.0f;
        output.NeuronParameter["E_ampa"] = 0.0f;
        output.NeuronParameter["ampa_tau"] = 5.0f;
        output.NeuronParameter["E_gaba"] = -80.0f;
        output.NeuronParameter["gaba_tau"] = 10.0f;
        output.NeuronParameter["nmda_tau"] = 100.0f;
        output.NeuronParameter["random_sigma"] = std::array<float, 6>{2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        layers.push_back(output);
        
        NeuronLayerDescription trigger;
        trigger.ModelName = "InputSpikeNeuronModel";
        trigger.numberofneuron = 2;
        trigger.isMonitored = false;
        trigger.isOutput = false;
        layers.push_back(trigger);

        // ===== 2. connections =====
        std::list<ConnectionDescription> connections;
        // plastic input -> output (R-STDP learns these)
        connections.push_back(MakeConn(0, 1, 0, 2.5f, 20.0f, 1, 0, -1));
        connections.push_back(MakeConn(0, 2, 0, 3.0f, 20.0f, 1, 0, -1));
        // winner-take-all lateral inhibition
        connections.push_back(MakeConn(1, 2, 1, 5.0f, 5.0f, 1, -1, -1));
        connections.push_back(MakeConn(2, 1, 1, 5.0f, 5.0f, 1, -1, -1));
        // reward trigger (neuron 3) and punishment trigger (neuron 4)
        connections.push_back(MakeConn(3, 1, 0, 0.0f, 0.0f, 1, -1, 0));
        connections.push_back(MakeConn(3, 2, 0, 0.0f, 0.0f, 1, -1, 0));
        connections.push_back(MakeConn(4, 1, 1, 0.0f, 0.0f, 1, -1, 0));
        connections.push_back(MakeConn(4, 2, 1, 0.0f, 0.0f, 1, -1, 0));

        // ===== 3. learning rules =====
        std::list<LearningRuleDescription> learning_rules;
        
        LearningRuleDescription rstdp;
        rstdp.RuleName = "R-STDP";
        rstdp.RuleParameter["Max_LTP"] = 0.08f;
        rstdp.RuleParameter["Max_LTD"] = 0.05f;
        rstdp.RuleParameter["LTP_tau"] = 20.0f;
        rstdp.RuleParameter["LTD_tau"] = 30.0f;
        rstdp.RuleParameter["RewardFactor"] = 1.0f;
        rstdp.RuleParameter["PunishmentFactor"] = -1.0f;
        learning_rules.push_back(rstdp);

        // ===== 4. output dir =====
        std::filesystem::create_directories("tutorial_output/decision_circuit");

        // ===== 5. simulation =====
        Simulation simulation(layers, connections, learning_rules, 20000, 0.1f, 1);
        simulation.AddFileOutputSpikeDriver("tutorial_output/decision_circuit/spikes.txt");
        simulation.AddFileOutputMonitorDriver("tutorial_output/decision_circuit/monitor.txt");
        simulation.AddFileOutputWeightDriver("tutorial_output/decision_circuit/weights.txt", 200);

        // ===== 6. init + external stimuli =====
        simulation.InitSimulation();
        
        std::vector<int> spike_times;
        std::vector<int> spike_neurons;

        // stimulus to neuron 0 every 200 steps
        for (int t = 200; t < 20000; t += 200) {
            spike_times.push_back(t);
            spike_neurons.push_back(0);
        }
        // reward to neuron 3, 60 steps after each stimulus
        for (int t = 260; t < 20000; t += 200) {
            spike_times.push_back(t);
            spike_neurons.push_back(3);
        }

        simulation.AddExternalSpikeActivity(spike_times, spike_neurons);

        simulation.RunSimulationStep(20000);
        
        std::cout << "Decision circuit finished." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "decision circuit exception: " << e.what() << std::endl;
        return 1;
    }
}