// paperpsp_delay_test.cpp
// End-to-end delay test THROUGH the simulator's connection + event scheduler
// (not a hand-injected impulse): an InputSpike neuron fires at t_pre, the spike
// travels a Type-4 PaperPSP connection with a given delay, and the target
// Izhikevich neuron starts responding exactly at t_pre + delay.
//
// Method: run the same network twice with two different delays (D and D+200);
// the first step where the two target membrane-potential traces differ is the
// earlier arrival step. Assert arrival == t_pre + D (+ one Euler step), and that
// the onset difference tracks the delay difference. Returns non-zero on failure.

#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

static ModelDescription MakePaperPSP() {
    ModelDescription m; m.ModelName = "PaperPSP"; m.ModelParameter["tau"] = 5.0f; return m;
}

// returns V trace of the target neuron (global id 1) for a Type-4 connection of
// the given delay
static std::vector<float> run_target_v(int delay, float w, int t_pre, int total,
                                       const std::string& file) {
    std::filesystem::create_directories("tutorial_output/paperpsp_delay");
    std::list<NeuronLayerDescription> layers;
    NeuronLayerDescription inp;
    inp.ModelName = "InputSpikeNeuronModel"; inp.numberofneuron = 1;
    layers.push_back(inp);                                   // id 0

    NeuronLayerDescription tgt;
    tgt.ModelName = "TimeDrivenIzhikevic_Exponential_Decay";
    tgt.numberofneuron = 1; tgt.isMonitored = true; tgt.isOutput = true;
    tgt.NeuronParameter["a"] = 0.02f; tgt.NeuronParameter["b"] = 0.20f;
    tgt.NeuronParameter["c"] = -65.0f; tgt.NeuronParameter["d"] = 8.0f;
    tgt.NeuronParameter["V_rest"] = -65.0f; tgt.NeuronParameter["V_reset"] = -65.0f;
    tgt.NeuronParameter["V_th"] = 30.0f; tgt.NeuronParameter["R"] = 1.0f;
    tgt.NeuronParameter["ampa_tau"] = 5.0f; tgt.NeuronParameter["gaba_tau"] = 10.0f;
    tgt.NeuronParameter["random_sigma"] = std::array<float, 6>{0, 0, 0, 0, 0, 0};
    layers.push_back(tgt);                                   // id 1

    std::list<ConnectionDescription> conns;
    ConnectionDescription c;
    c.SourceNeuron = {0}; c.TargetNeuron = {1}; c.Type = {4};
    c.Weight = {w}; c.MaxWeight = {w}; c.Delay = {delay};
    c.SynapseRule = {-1}; c.TriggerRule = {-1}; c.SynapseModel = {MakePaperPSP()};
    conns.push_back(c);

    std::list<LearningRuleDescription> rules;
    {
        Simulation sim(layers, conns, rules, total, 0.1f, 1);
        sim.AddFileOutputMonitorDriver(file);
        sim.InitSimulation();
        sim.AddExternalSpikeActivity({t_pre}, {0});
        sim.RunSimulationStep(total);
    }
    std::vector<float> v;
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line); int t, id; float val;
        ss >> t >> id >> val;
        if (id == 1) v.push_back(val);
    }
    return v;
}

static int first_diff_step(const std::vector<float>& a, const std::vector<float>& b) {
    size_t n = std::min(a.size(), b.size());
    for (size_t k = 0; k < n; ++k) if (std::fabs(a[k] - b[k]) > 1e-4f) return (int)k;
    return -1;
}

int main() {
    const int t_pre = 50;       // step of the input spike (5 ms)
    const float w = 5.0f;
    bool ok = true;

    std::cout << "PaperPSP end-to-end delay test (spike at step " << t_pre
              << ", through Type-4 connection + event scheduler)\n";

    auto test_delay = [&](int delay) {
        int total = t_pre + delay + 500;
        // reference uses a much later arrival (delay + 200) so the two traces
        // are identical until the tested connection arrives
        std::vector<float> late = run_target_v(delay + 200, w, t_pre, total,
                                               "tutorial_output/paperpsp_delay/late.txt");
        std::vector<float> con = run_target_v(delay, w, t_pre, total,
                                              "tutorial_output/paperpsp_delay/conn.txt");
        int onset = first_diff_step(late, con);
        int expect = t_pre + delay;
        bool pass = (onset >= expect) && (onset <= expect + 2);
        std::cout << "  [" << (pass ? "PASS" : "FAIL") << "] delay=" << delay
                  << " steps (" << delay * 0.1f << " ms): observed arrival step=" << onset
                  << ", expected " << expect << ".." << expect + 2 << "\n";
        ok = ok && pass;
        return onset;
    };

    int onset30 = test_delay(30);    // 3 ms
    int onset100 = test_delay(100);  // 10 ms

    bool pass_diff = (onset100 - onset30) == 70;
    std::cout << "  [" << (pass_diff ? "PASS" : "FAIL") << "] delay difference 100-30=70 steps -> "
              << "onset difference = " << (onset100 - onset30) << "\n";
    ok = ok && pass_diff;

    std::cout << (ok ? "ALL DELAY CHECKS PASSED\n" : "SOME DELAY CHECKS FAILED\n");
    return ok ? 0 : 1;
}
