// fig4_two_units.cpp
// Full Fig.4 structure: TWO decision units.
//   Unit LEFT  : RS_L -> {CH_LL, CH_LR} + LTS cross-inhibition
//   Unit RIGHT : RS_R -> {CH_RL, CH_RR} + LTS cross-inhibition
// A cue_position (LEFT/RIGHT) stimulates only that unit's RS; the other unit
// stays silent. Within the stimulated unit the two CH compete (PaperPSP
// synapses, independent per-CH noise). No DA-STDP, no reward, no robot.
//
// Usage: fig4_two_units [noise_sigma]
//
// Neuron global indices:
//   0,1        cue currents  (cue_left, cue_right)
//   2,3        RS           (RS_L, RS_R)
//   4,5,6,7    CH           (CH_LL, CH_LR, CH_RL, CH_RR)
//   8,9,10,11  LTS          (LTS_LL, LTS_LR, LTS_RL, LTS_RR)
//   12,13,14,15 noise injectors for the four CH

#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "inc/Fig4EffectiveParameterAudit.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <vector>

const int TOTAL_STEPS = 5000;
const int WIN_START = 100, WIN_END = 4000;

static ModelDescription MakePaperPSP() {
    ModelDescription m; m.ModelName = "PaperPSP"; m.ModelParameter["tau"] = 5.0f; return m;
}
static void add_single(std::list<ConnectionDescription>& c, int s, int t, int type,
                       float w, int d, const ModelDescription& sm) {
    ConnectionDescription x;
    x.SourceNeuron = {s}; x.TargetNeuron = {t}; x.Type = {type};
    x.Weight = {w}; x.MaxWeight = {w >= 0 ? w : -w}; x.Delay = {d};
    x.SynapseRule = {-1}; x.TriggerRule = {-1}; x.SynapseModel = {sm};
    c.push_back(x);
}
static void add_bundle(std::list<ConnectionDescription>& c, int s, int t, int n,
                       int dmin, int dmax, int type, std::mt19937& gen,
                       float w_mean, float jitter, const ModelDescription& sm) {
    ConnectionDescription x;
    std::normal_distribution<float> jd(0.0f, w_mean * jitter);
    for (int i = 0; i < n; ++i) {
        int d = (n == 1) ? dmin : dmin + (int)((long long)(dmax - dmin) * i / (n - 1));
        float w = w_mean + jd(gen);
        x.SourceNeuron.push_back(s); x.TargetNeuron.push_back(t); x.Type.push_back(type);
        x.Weight.push_back(w); x.MaxWeight.push_back(w >= 0 ? w : -w); x.Delay.push_back(d);
        x.SynapseRule.push_back(-1); x.TriggerRule.push_back(-1); x.SynapseModel.push_back(sm);
    }
    c.push_back(x);
}

struct UnitResult { int nL, nR, fL, fR; float D; };

static UnitResult metrics_unit(const std::string& spike_file, int chL, int chR) {
    std::ifstream in(spike_file);
    std::vector<int> tL, tR; std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line); int t, id; ss >> t >> id;
        if (id == chL) tL.push_back(t); else if (id == chR) tR.push_back(t);
    }
    auto cnt = [](const std::vector<int>& v) {
        int c = 0; for (int x : v) if (x >= WIN_START && x < WIN_END) c++; return c;
    };
    UnitResult r; r.nL = cnt(tL); r.nR = cnt(tR);
    auto first_in_win = [](const std::vector<int>& v) {
        for (int x : v) if (x >= WIN_START && x < WIN_END) return x;
        return -1;
    };
    r.fL = first_in_win(tL); r.fR = first_in_win(tR);
    r.D = (r.nL + r.nR) ? (float)(r.nR - r.nL) / (r.nL + r.nR) : 0.0f;
    return r;
}

// cue_side: 0 = LEFT unit stimulated, 1 = RIGHT unit stimulated.
static void run_cue(int cue_side, float noise_sigma, unsigned seed,
                    float w_mean, int n_syn, float jitter,
                    float w_ch_lts, float w_lts_ch, float i_rs,
                    const std::string& out_dir) {
    std::filesystem::create_directories(out_dir);
    std::list<NeuronLayerDescription> layers;

    auto izh = [](int n, float a, float b, float c, float d, bool io) {
        NeuronLayerDescription l;
        l.ModelName = "TimeDrivenIzhikevic_Exponential_Decay";
        l.numberofneuron = n; l.isMonitored = true; l.isOutput = io;
        l.NeuronParameter["a"] = a; l.NeuronParameter["b"] = b;
        l.NeuronParameter["c"] = c; l.NeuronParameter["d"] = d;
    l.NeuronParameter["V_rest"] = -65.0f; // c alone controls reset; V_reset would override it.
        l.NeuronParameter["V_th"] = 30.0f; l.NeuronParameter["R"] = 1.0f;
        l.NeuronParameter["ampa_tau"] = 5.0f; l.NeuronParameter["gaba_tau"] = 10.0f;
        l.NeuronParameter["random_sigma"] = std::array<float, 6>{0, 0, 0, 0, 0, 0};
        return l;
    };
    NeuronLayerDescription cue; cue.ModelName = "InputCurrentNeuronModel"; cue.numberofneuron = 2;
    layers.push_back(cue);                 // 0,1
    layers.push_back(izh(2, 0.02f, 0.20f, -65.0f, 8.0f, true));  // 2,3 RS
    layers.push_back(izh(4, 0.02f, 0.20f, -50.0f, 2.0f, true));  // 4..7 CH
    layers.push_back(izh(4, 0.02f, 0.25f, -65.0f, 2.0f, true));  // 8..11 LTS
    NeuronLayerDescription nz; nz.ModelName = "InputCurrentNeuronModel"; nz.numberofneuron = 4;
    layers.push_back(nz);                  // 12..15 noise

    std::list<ConnectionDescription> conns;
    ModelDescription psp = MakePaperPSP();
    add_single(conns, 0, 2, 3, 1.0f, 1, ModelDescription{});   // cue_left  -> RS_L
    add_single(conns, 1, 3, 3, 1.0f, 1, ModelDescription{});   // cue_right -> RS_R

    std::mt19937 gen(seed);
    // unit 0 (left)
    add_bundle(conns, 2, 4, n_syn, 10, 1000, 4, gen, w_mean, jitter, psp);
    add_bundle(conns, 2, 5, n_syn, 10, 1000, 4, gen, w_mean, jitter, psp);
    // unit 1 (right)
    add_bundle(conns, 3, 6, n_syn, 10, 1000, 4, gen, w_mean, jitter, psp);
    add_bundle(conns, 3, 7, n_syn, 10, 1000, 4, gen, w_mean, jitter, psp);

    add_single(conns, 4, 8, 4, w_ch_lts, 10, psp);   // CH_LL -> LTS_LL
    add_single(conns, 5, 9, 4, w_ch_lts, 10, psp);   // CH_LR -> LTS_LR
    add_single(conns, 6, 10, 4, w_ch_lts, 10, psp);  // CH_RL -> LTS_RL
    add_single(conns, 7, 11, 4, w_ch_lts, 10, psp);  // CH_RR -> LTS_RR
    add_single(conns, 8, 5, 4, -w_lts_ch, 20, psp);  // LTS_LL -> CH_LR
    add_single(conns, 9, 4, 4, -w_lts_ch, 20, psp);  // LTS_LR -> CH_LL
    add_single(conns, 10, 7, 4, -w_lts_ch, 20, psp); // LTS_RL -> CH_RR
    add_single(conns, 11, 6, 4, -w_lts_ch, 20, psp); // LTS_RR -> CH_RL

    add_single(conns, 12, 4, 3, 1.0f, 1, ModelDescription{});
    add_single(conns, 13, 5, 3, 1.0f, 1, ModelDescription{});
    add_single(conns, 14, 6, 3, 1.0f, 1, ModelDescription{});
    add_single(conns, 15, 7, 3, 1.0f, 1, ModelDescription{});

    // per-CH independent noise (only the stimulated unit, as in Fig.4)
    std::vector<std::vector<float>> noise(4, std::vector<float>(TOTAL_STEPS, 0.0f));
    for (int ch = 0; ch < 4; ++ch) {
        bool active = (cue_side == 0) ? (ch < 2) : (ch >= 2);
        if (!active) continue;                 // inactive unit: no drive, no noise
        std::mt19937 g(seed * 7u + (unsigned)ch + 1u);
        std::normal_distribution<float> dist(0.0f, noise_sigma);
        for (int s = 0; s < TOTAL_STEPS; ++s) noise[ch][s] = dist(g);
    }

    std::list<LearningRuleDescription> rules;
    {
        Simulation sim(layers, conns, rules, TOTAL_STEPS, 0.1f, 1);
        sim.AddFileOutputSpikeDriver(out_dir + "/spikes.txt");
        sim.AddFileOutputMonitorDriver(out_dir + "/monitor.txt");
        sim.InitSimulation();
        fig4::require_paper_ch(sim,4,4);
        int cue_neuron = (cue_side == 0) ? 0 : 1;
        sim.AddExternalCurrentActivity({100, 1000}, {cue_neuron, cue_neuron}, {i_rs, 0.0f});
        for (int ch = 0; ch < 4; ++ch) {
            std::vector<int> t(TOTAL_STEPS), nid(TOTAL_STEPS, 12 + ch);
            for (int s = 0; s < TOTAL_STEPS; ++s) t[s] = s;
            sim.AddExternalCurrentActivity(t, nid, noise[ch]);
        }
        sim.RunSimulationStep(TOTAL_STEPS);
    }

    UnitResult L = metrics_unit(out_dir + "/spikes.txt", 4, 5);  // unit left
    UnitResult R = metrics_unit(out_dir + "/spikes.txt", 6, 7);  // unit right
    std::cout << "=== cue = " << (cue_side == 0 ? "LEFT" : "RIGHT") << " ===\n";
    std::cout << "  LEFT  unit: CH_LL n=" << L.nL << " (f=" << L.fL << ")  CH_LR n=" << L.nR
              << " (f=" << L.fR << ")  D=" << L.D << "\n";
    std::cout << "  RIGHT unit: CH_RL n=" << R.nL << " (f=" << R.fL << ")  CH_RR n=" << R.nR
              << " (f=" << R.fR << ")  D=" << R.D << "\n";
    std::cout << "  inactive unit total spikes = "
              << ((cue_side == 0) ? (R.nL + R.nR) : (L.nL + L.nR)) << "\n";
}

int main(int argc, char** argv) {
    float noise_sigma = 20.0f;
    if (argc > 1) noise_sigma = std::stof(argv[1]);
    // fixed demo seed; weights follow the single-unit defaults
    run_cue(0, noise_sigma, 20260929u, 0.5f, 100, 0.10f, 30.0f, 40.0f, 22.5f,
            "tutorial_output/fig4/units_cue_left");
    run_cue(1, noise_sigma, 20260929u, 0.5f, 100, 0.10f, 30.0f, 40.0f, 22.5f,
            "tutorial_output/fig4/units_cue_right");
    return 0;
}
