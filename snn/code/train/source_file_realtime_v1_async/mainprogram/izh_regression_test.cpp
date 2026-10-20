// izh_regression_test.cpp
// Deterministic (random_sigma = 0) Izhikevich regression test exercising the
// original connection Types: 0 = excitatory (AMPA), 1 = inhibitory (GABA),
// 2 = NMDA, 3 = external current. Used to confirm that adding Type 4/5
// (PaperPSP) support did not change the behaviour of Types 0..3.

#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>
#include <array>
#include <filesystem>
#include <list>
#include <vector>

static ConnectionDescription MakeConn(int src, int tgt, int type, float w, int delay) {
    ConnectionDescription c;
    c.SourceNeuron = {src};
    c.TargetNeuron = {tgt};
    c.Type = {type};
    c.Weight = {w};
    c.MaxWeight = {w};
    c.Delay = {delay};
    c.SynapseRule = {-1};
    c.TriggerRule = {-1};
    c.SynapseModel = {ModelDescription{}};
    return c;
}

int main() {
    std::list<NeuronLayerDescription> layers;

    // 0,1: two input spike neurons
    NeuronLayerDescription inp;
    inp.ModelName = "InputSpikeNeuronModel";
    inp.numberofneuron = 2;
    layers.push_back(inp);

    // 2,3: two Izhikevich neurons (c=-50, d=2 -> they fire when driven)
    NeuronLayerDescription izh;
    izh.ModelName = "TimeDrivenIzhikevic_I_Exponential_Decay";
    izh.numberofneuron = 2;
    izh.isMonitored = true;
    izh.isOutput = true;
    izh.NeuronParameter["a"] = 0.02f;
    izh.NeuronParameter["b"] = 0.20f;
    izh.NeuronParameter["c"] = -50.0f;
    izh.NeuronParameter["d"] = 2.0f;
    izh.NeuronParameter["V_rest"] = -65.0f;
    izh.NeuronParameter["V_reset"] = -65.0f;
    izh.NeuronParameter["V_th"] = 30.0f;
    izh.NeuronParameter["R"] = 1.0f;
    izh.NeuronParameter["E_ampa"] = 0.0f;
    izh.NeuronParameter["ampa_tau"] = 5.0f;
    izh.NeuronParameter["E_gaba"] = -80.0f;
    izh.NeuronParameter["gaba_tau"] = 10.0f;
    izh.NeuronParameter["nmda_tau"] = 80.0f;
    izh.NeuronParameter["random_sigma"] = std::array<float, 6>{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    layers.push_back(izh);

    std::list<ConnectionDescription> connections;
    connections.push_back(MakeConn(0, 2, 0, 20.0f, 1));  // exc input -> neuron 2
    connections.push_back(MakeConn(1, 3, 0, 20.0f, 1));  // exc input -> neuron 3
    connections.push_back(MakeConn(2, 3, 1, 5.0f, 1));   // inhibitory 2 -> 3
    connections.push_back(MakeConn(3, 2, 1, 5.0f, 1));   // inhibitory 3 -> 2
    connections.push_back(MakeConn(0, 3, 2, 4.5f, 1));   // NMDA 0 -> 3

    std::list<LearningRuleDescription> rules;
    std::filesystem::create_directories("tutorial_output/izh_regression");

    {
        Simulation sim(layers, connections, rules, 1000, 0.1f, 1);
        sim.AddFileOutputSpikeDriver("tutorial_output/izh_regression/spikes.txt");
        sim.AddFileOutputMonitorDriver("tutorial_output/izh_regression/monitor.txt");
        sim.InitSimulation();
        std::vector<int> times;
        std::vector<int> neurons;
        for (int t = 20; t < 1000; t += 100) { times.push_back(t); neurons.push_back(0); }
        for (int t = 40; t < 1000; t += 150) { times.push_back(t); neurons.push_back(1); }
        sim.AddExternalSpikeActivity(times, neurons);
        sim.RunSimulationStep(1000);
    }
    return 0;
}
