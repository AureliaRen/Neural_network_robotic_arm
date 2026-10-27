// fig4_decision_circuit.cpp
// C++ build of Fig.4 of Wei, Bu & Zhu (BSPC 2020): one decision unit
//   RS -> {CH_L, CH_R} with LTS cross-inhibition.
// Each "line" is N parallel PaperPSP synapses (Eq.4 additive-current PSP),
// delays spread over 1..100 ms, weights from ONE distribution with small
// random jitter. Symmetry is broken ONLY by independent per-CH random noise
// (as drawn in Fig.4), never by a systematic left/right bias.
// No DA-STDP, no reward/punishment, no robot.
//
// Modes:
//   fig4_decision_circuit                       -> demo: symmetric + A + B, writes files
//   fig4_decision_circuit seeds <K>             -> seed study: A/B over K seeds -> fig4_seed_study.csv
// Optional overrides: <noise_sigma> <n_syn> <jitter> <w_mean> <w_ch_lts> <w_lts_ch> <i_rs>
//
// Neuron global indices (one unit):
//   0   cue current        1  RS
//   2,3 CH_L, CH_R         4,5 LTS_L, LTS_R
//   6,7 noise injectors for CH_L, CH_R (InputCurrent + Type 3)

#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "inc/Fig4EffectiveParameterAudit.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <boost/any.hpp>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <random>
#include <sstream>
#include <string>
#include <vector>

const int TOTAL_STEPS = 5000;   // 500 ms
const int WIN_START = 100;
const int WIN_END = 4000;
const int ID_RS = 1, ID_CHL = 2, ID_CHR = 3, ID_LTSL = 4, ID_LTSR = 5;

struct Config {
    int n_syn = 100;
    int delay_min_steps = 10;    // 1 ms
    int delay_max_steps = 1000;  // 100 ms
    float w_mean = 0.5f;
    float w_jitter = 0.10f;
    float w_ch_lts = 30.0f;
    float w_lts_ch = 40.0f;
    float i_rs = 22.5f;
    float noise_sigma = 20.0f;   // independent white current noise on each CH
    unsigned seed = 12345u;
};

struct Metrics { int nL, nR, fL, fR; float D; const char* winner; };

// ------------------------------ helpers ------------------------------------
static ModelDescription MakePaperPSP() {
    ModelDescription m;
    m.ModelName = "PaperPSP";
    m.ModelParameter["tau"] = 5.0f;   // paper tau = 5 ms
    return m;
}

static void add_single(std::list<ConnectionDescription>& conns, int src, int tgt,
                       int type, float w, int delay, const ModelDescription& synmodel) {
    ConnectionDescription c;
    c.SourceNeuron = {src};
    c.TargetNeuron = {tgt};
    c.Type = {type};
    c.Weight = {w};
    c.MaxWeight = {w >= 0.0f ? w : -w};
    c.Delay = {delay};
    c.SynapseRule = {-1};
    c.TriggerRule = {-1};
    c.SynapseModel = {synmodel};
    conns.push_back(c);
}

// N parallel synapses; both CH bundles use the SAME distribution (no bias).
static void add_bundle(std::list<ConnectionDescription>& conns, int src, int tgt,
                       int n, int dmin, int dmax, int type, std::mt19937& gen,
                       float w_mean, float w_jitter, const ModelDescription& synmodel) {
    ConnectionDescription c;
    std::normal_distribution<float> jitter(0.0f, w_mean * w_jitter);
    for (int i = 0; i < n; ++i) {
        int delay = (n == 1) ? dmin
                             : dmin + (int)((long long)(dmax - dmin) * i / (n - 1));
        float w = w_mean + (w_jitter > 0.0f ? jitter(gen) : 0.0f);
        c.SourceNeuron.push_back(src);
        c.TargetNeuron.push_back(tgt);
        c.Type.push_back(type);
        c.Weight.push_back(w);
        c.MaxWeight.push_back(w >= 0.0f ? w : -w);
        c.Delay.push_back(delay);
        c.SynapseRule.push_back(-1);
        c.TriggerRule.push_back(-1);
        c.SynapseModel.push_back(synmodel);
    }
    conns.push_back(c);
}

static Metrics read_metrics(const std::string& spike_file) {
    std::ifstream in(spike_file);
    std::vector<int> tL, tR;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        int t, nid;
        ss >> t >> nid;
        if (nid == ID_CHL) tL.push_back(t);
        else if (nid == ID_CHR) tR.push_back(t);
    }
    auto cnt = [](const std::vector<int>& v) {
        int c = 0;
        for (int x : v) if (x >= WIN_START && x < WIN_END) c++;
        return c;
    };
    Metrics m;
    m.nL = cnt(tL); m.nR = cnt(tR);
    auto first_in_win = [](const std::vector<int>& v) {
        for (int x : v) if (x >= WIN_START && x < WIN_END) return x;
        return -1;
    };
    m.fL = first_in_win(tL);
    m.fR = first_in_win(tR);
    m.D = (m.nL + m.nR) ? (float)(m.nR - m.nL) / (float)(m.nL + m.nR) : 0.0f;
    m.winner = (m.nL == m.nR) ? "TIE" : ((m.nL > m.nR) ? "CH_L" : "CH_R");
    return m;
}

// ------------------------------ one run ------------------------------------
// Builds and runs one single-unit simulation. If write_files, writes spikes.
static Metrics run_once(const Config& cfg, bool with_inhibition, bool symmetric,
                        const std::vector<float>& noise_L, const std::vector<float>& noise_R,
                        const std::string& out_dir, bool write_files) {
    std::list<NeuronLayerDescription> layers;

    NeuronLayerDescription cue;
    cue.ModelName = "InputCurrentNeuronModel"; cue.numberofneuron = 1;
    layers.push_back(cue);

    NeuronLayerDescription rs;
    rs.ModelName = "TimeDrivenIzhikevic_Exponential_Decay";
    rs.numberofneuron = 1; rs.isMonitored = true; rs.isOutput = true;
    rs.NeuronParameter["a"] = 0.02f; rs.NeuronParameter["b"] = 0.20f;
    rs.NeuronParameter["c"] = -65.0f; rs.NeuronParameter["d"] = 8.0f;
    rs.NeuronParameter["V_rest"] = -65.0f; rs.NeuronParameter["V_reset"] = -65.0f;
    rs.NeuronParameter["V_th"] = 30.0f; rs.NeuronParameter["R"] = 1.0f;
    rs.NeuronParameter["ampa_tau"] = 5.0f; rs.NeuronParameter["gaba_tau"] = 10.0f;
    rs.NeuronParameter["random_sigma"] = std::array<float, 6>{0, 0, 0, 0, 0, 0};
    layers.push_back(rs);

    auto make_ch = []() {
        NeuronLayerDescription l;
        l.ModelName = "TimeDrivenIzhikevic_Exponential_Decay";
        l.numberofneuron = 2; l.isMonitored = true; l.isOutput = true;
        l.NeuronParameter["a"] = 0.02f; l.NeuronParameter["b"] = 0.20f;
        l.NeuronParameter["c"] = -50.0f; l.NeuronParameter["d"] = 2.0f;
        l.NeuronParameter["V_rest"] = -65.0f; // CH reset comes from c=-50, not a conflicting alias.
        l.NeuronParameter["V_th"] = 30.0f; l.NeuronParameter["R"] = 1.0f;
        l.NeuronParameter["ampa_tau"] = 5.0f; l.NeuronParameter["gaba_tau"] = 10.0f;
        l.NeuronParameter["random_sigma"] = std::array<float, 6>{0, 0, 0, 0, 0, 0};
        return l;
    };
    layers.push_back(make_ch());

    NeuronLayerDescription lts;
    lts.ModelName = "TimeDrivenIzhikevic_Exponential_Decay";
    lts.numberofneuron = 2; lts.isMonitored = true; lts.isOutput = true;
    lts.NeuronParameter["a"] = 0.02f; lts.NeuronParameter["b"] = 0.25f;
    lts.NeuronParameter["c"] = -65.0f; lts.NeuronParameter["d"] = 2.0f;
    lts.NeuronParameter["V_rest"] = -65.0f; lts.NeuronParameter["V_reset"] = -65.0f;
    lts.NeuronParameter["V_th"] = 30.0f; lts.NeuronParameter["R"] = 1.0f;
    lts.NeuronParameter["ampa_tau"] = 5.0f; lts.NeuronParameter["gaba_tau"] = 10.0f;
    lts.NeuronParameter["random_sigma"] = std::array<float, 6>{0, 0, 0, 0, 0, 0};
    layers.push_back(lts);

    NeuronLayerDescription noise;
    noise.ModelName = "InputCurrentNeuronModel"; noise.numberofneuron = 2;   // ids 6,7
    layers.push_back(noise);

    std::list<ConnectionDescription> connections;
    ModelDescription psp = MakePaperPSP();
    add_single(connections, 0, ID_RS, 3, 1.0f, 1, ModelDescription{});       // cue current -> RS

    std::mt19937 gen(cfg.seed);
    float jit = symmetric ? 0.0f : cfg.w_jitter;
    add_bundle(connections, ID_RS, ID_CHL, cfg.n_syn, cfg.delay_min_steps,
               cfg.delay_max_steps, 4, gen, cfg.w_mean, jit, psp);
    add_bundle(connections, ID_RS, ID_CHR, cfg.n_syn, cfg.delay_min_steps,
               cfg.delay_max_steps, 4, gen, cfg.w_mean, jit, psp);

    add_single(connections, ID_CHL, ID_LTSL, 4, cfg.w_ch_lts, 10, psp);
    add_single(connections, ID_CHR, ID_LTSR, 4, cfg.w_ch_lts, 10, psp);
    if (with_inhibition) {
        add_single(connections, ID_LTSL, ID_CHR, 4, -cfg.w_lts_ch, 20, psp);
        add_single(connections, ID_LTSR, ID_CHL, 4, -cfg.w_lts_ch, 20, psp);
    }
    // independent noise injectors (Type 3 current) -> CH_L, CH_R
    add_single(connections, 6, ID_CHL, 3, 1.0f, 1, ModelDescription{});
    add_single(connections, 7, ID_CHR, 3, 1.0f, 1, ModelDescription{});

    std::list<LearningRuleDescription> rules;

    if (write_files) std::filesystem::create_directories(out_dir);
    {
        Simulation sim(layers, connections, rules, TOTAL_STEPS, 0.1f, 1);
        if (write_files) {
            sim.AddFileOutputSpikeDriver(out_dir + "/spikes.txt");
            sim.AddFileOutputMonitorDriver(out_dir + "/monitor.txt");
        }
        sim.InitSimulation();
        fig4::require_paper_ch(sim,2,2);
        sim.AddExternalCurrentActivity({100, 1000}, {0, 0}, {cfg.i_rs, 0.0f});
        // per-step independent noise currents (one event per step per CH)
        {
            std::vector<int> t, n6, n7; std::vector<float> v6, v7;
            t.reserve(TOTAL_STEPS); n6.reserve(TOTAL_STEPS); n7.reserve(TOTAL_STEPS);
            v6.reserve(TOTAL_STEPS); v7.reserve(TOTAL_STEPS);
            for (int s = 0; s < TOTAL_STEPS; ++s) {
                t.push_back(s); n6.push_back(6); n7.push_back(7);
                v6.push_back(noise_L.empty() ? 0.0f : noise_L[s]);
                v7.push_back(noise_R.empty() ? 0.0f : noise_R[s]);
            }
            sim.AddExternalCurrentActivity(t, n6, v6);
            sim.AddExternalCurrentActivity(t, n7, v7);
        }
        sim.RunSimulationStep(TOTAL_STEPS);
    }

    return read_metrics(out_dir + "/spikes.txt");
}

// noise helper (white, independent per CH)
static std::vector<float> make_noise(unsigned seed, int n, float sigma, int channel) {
    std::mt19937 g(seed * 2u + (unsigned)channel + 1u);
    std::normal_distribution<float> dist(0.0f, sigma);
    std::vector<float> v(n);
    for (int i = 0; i < n; ++i) v[i] = dist(g);
    return v;
}

static void print_metrics(const std::string& tag, const Metrics& m) {
    std::cout << "[" << tag << "] CH_L: first=" << m.fL << " n_win=" << m.nL
              << " | CH_R: first=" << m.fR << " n_win=" << m.nR
              << " | D=" << m.D << " winner=" << m.winner << "\n";
}

int main(int argc, char** argv) {
    Config cfg;
    int argi = 1;
    bool study = false; int k_seeds = 0;
    if (argc > 1 && std::string(argv[1]) == "seeds") {
        study = true;
        k_seeds = (argc > 2) ? std::atoi(argv[2]) : 50;
        argi = 3;
    }
    if (argc > argi + 0) cfg.noise_sigma = std::stof(argv[argi + 0]);
    if (argc > argi + 1) cfg.n_syn = std::stoi(argv[argi + 1]);
    if (argc > argi + 2) cfg.w_jitter = std::stof(argv[argi + 2]);
    if (argc > argi + 3) cfg.w_mean = std::stof(argv[argi + 3]);
    if (argc > argi + 4) cfg.w_ch_lts = std::stof(argv[argi + 4]);
    if (argc > argi + 5) cfg.w_lts_ch = std::stof(argv[argi + 5]);
    if (argc > argi + 6) cfg.i_rs = std::stof(argv[argi + 6]);

    if (!study) {
        // demo: strict symmetric control (no noise), then A (no inh) and B (inh) with noise
        std::vector<float> zero(TOTAL_STEPS, 0.0f);
        cfg.seed = 12345u;
        Metrics sym = run_once(cfg, true, true, zero, zero, "tutorial_output/fig4/symmetric", true);
        std::cout << "[symmetric] n_syn=" << cfg.n_syn << " (identical weights, no noise)\n";
        print_metrics("symmetric", sym);

        std::vector<float> nL = make_noise(cfg.seed, TOTAL_STEPS, cfg.noise_sigma, 0);
        std::vector<float> nR = make_noise(cfg.seed, TOTAL_STEPS, cfg.noise_sigma, 1);
        Metrics A = run_once(cfg, false, false, nL, nR, "tutorial_output/fig4/A_no_inhibition", true);
        Metrics B = run_once(cfg, true, false, nL, nR, "tutorial_output/fig4/B_with_inhibition", true);
        std::cout << "[A_no_inhibition] (independent CH noise, no LTS)\n"; print_metrics("A", A);
        std::cout << "[B_with_inhibition] (same noise, with LTS)\n"; print_metrics("B", B);
        std::cout << "|D_A|=" << std::fabs(A.D) << " |D_B|=" << std::fabs(B.D) << "\n";
        return 0;
    }

    // seed study: A vs B over k_seeds
    std::ofstream csv("fig4_seed_study.csv");
    csv << "seed,D_A,D_B,winner_A,winner_B,nL_A,nR_A,nL_B,nR_B\n";
    double sumA = 0.0, sumB = 0.0; int leftA = 0, rightA = 0, tieA = 0, leftB = 0, rightB = 0, tieB = 0;
    int n_gain = 0, n_both_decisive = 0, n_same = 0;
    for (int s = 1; s <= k_seeds; ++s) {
        cfg.seed = (unsigned)s;
        std::vector<float> nL = make_noise(cfg.seed, TOTAL_STEPS, cfg.noise_sigma, 0);
        std::vector<float> nR = make_noise(cfg.seed, TOTAL_STEPS, cfg.noise_sigma, 1);
        Metrics A = run_once(cfg, false, false, nL, nR, "tutorial_output/fig4/_study_tmp", true);
        Metrics B = run_once(cfg, true, false, nL, nR, "tutorial_output/fig4/_study_tmp", true);
        sumA += std::fabs(A.D); sumB += std::fabs(B.D);
        if (A.nL > A.nR) leftA++; else if (A.nR > A.nL) rightA++; else tieA++;
        if (B.nL > B.nR) leftB++; else if (B.nR > B.nL) rightB++; else tieB++;
        if (std::fabs(B.D) > std::fabs(A.D)) n_gain++;
        int swA = (A.nL > A.nR) ? 1 : ((A.nR > A.nL) ? -1 : 0);
        int swB = (B.nL > B.nR) ? 1 : ((B.nR > B.nL) ? -1 : 0);
        if (swA != 0 && swB != 0) { n_both_decisive++; if (swA == swB) n_same++; }
        csv << s << "," << A.D << "," << B.D << "," << A.winner << "," << B.winner
            << "," << A.nL << "," << A.nR << "," << B.nL << "," << B.nR << "\n";
    }
    csv.close();
    std::cout << "seeds=" << k_seeds
              << "  mean|D_A|=" << sumA / k_seeds << "  mean|D_B|=" << sumB / k_seeds << "\n";
    std::cout << "A: left=" << leftA << " right=" << rightA << " tie=" << tieA << "\n";
    std::cout << "B: left=" << leftB << " right=" << rightB << " tie=" << tieB << "\n";
    std::cout << "|D_B|>|D_A| in " << n_gain << "/" << k_seeds << " seeds\n";
    std::cout << "A/B winner same side (both decisive): " << n_same << "/" << n_both_decisive << "\n";
    std::cout << "wrote fig4_seed_study.csv\n";
    return 0;
}
