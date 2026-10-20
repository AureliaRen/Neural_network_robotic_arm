// fig4_da_stdp_training.cpp
// DA-STDP learning experiment on the Fig.4 two-unit decision network.
// Implements Eq.(5)-(8) via the framework rule DASTDP (LearningRule/inc/Rule/DASTDP.h).
//
// Per trial: cue(L/R) -> that unit's RS -> two CH compete via LTS cross-inhibition
// -> decode action from CH spikes in the decision window -> virtual reward /
// punishment -> d drives ds=c*d -> weights persist into the next trial.
// Only the 4 RS->CH bundles (100 synapses each) are plastic; the rest are fixed.
// No arm / vision / RPC.
//
// CLI: --mode control|reward|punishment  --trials N  --seed S
//      --tau-c T  --w-max W  --noise-sigma N  --learning-ms 2000  --freeze 0|1
//      --out DIR  --diag-tau   (deterministic delay diagnostic, then exit)

#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "inc/Fig4EffectiveParameterAudit.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/Rule/DASTDP.h"
#include <boost/any.hpp>
#include <algorithm>
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
#include <iomanip>
#include <stdexcept>
#include <map>

const float DT = 0.1f;
const int N_SYN = 100;
const int DELAY_MIN = 10, DELAY_MAX = 1000;   // 1..100 ms
const int INIT_OFFSET = 100;
const int DECISION_STEPS = 500;               // 50 ms decision window

const int ID_CUE_L = 0, ID_CUE_R = 1;
const int ID_RS_L = 2, ID_RS_R = 3;
const int ID_CH_LL = 4, ID_CH_LR = 5, ID_CH_RL = 6, ID_CH_RR = 7;
const int ID_LTS_LL = 8, ID_LTS_LR = 9, ID_LTS_RL = 10, ID_LTS_RR = 11;
const int ID_NZ_LL = 12, ID_NZ_LR = 13, ID_NZ_RL = 14, ID_NZ_RR = 15;

struct Config {
    std::string mode = "control";
    int trials = 100;
    unsigned seed = 1;
    float tau_c = 5.0f;          // paper: tau_c = 5.0 ms
    float w_max = 2.0f;
    float noise_sigma = 20.0f;
    float w_mean = 0.5f;
    float w_jitter = 0.10f;
    float w_ch_lts = 30.0f;
    float w_lts_ch = 40.0f;
    float i_rs = 22.5f;
    float learning_ms = 2000.0f; // paper: 2000 ms extra learning
    bool freeze = false;         // version B
    std::string out = "tutorial_output/da_stdp";
    bool diagnostics = false;
    bool emissionClock = false;
    bool feedbackCueOff = false; // hypothesis: centered target removes lateral cue
    bool referenceSign = false;
    bool eval = false;           // evaluation mode (fixed weights, d=0)
    std::string weights_file;
    unsigned eval_seed = 999;
};

static ModelDescription MakePaperPSP() {
    ModelDescription m; m.ModelName = "PaperPSP"; m.ModelParameter["tau"] = 5.0f; return m;
}

static void add_single(std::list<ConnectionDescription>& c, int s, int t, int type,
                       float w, int delay, int synRule, float maxw, const ModelDescription& sm) {
    ConnectionDescription x;
    x.SourceNeuron = {s}; x.TargetNeuron = {t}; x.Type = {type};
    x.Weight = {w}; x.MaxWeight = {maxw}; x.Delay = {delay};
    x.SynapseRule = {synRule}; x.TriggerRule = {-1}; x.SynapseModel = {sm};
    c.push_back(x);
}

static void add_bundle(std::list<ConnectionDescription>& c, int s, int t, int n,
                       int dmin, int dmax, int type, std::mt19937& gen,
                       float w_mean, float jitter, int synRule, float maxw,
                       const ModelDescription& sm) {
    ConnectionDescription x;
    std::normal_distribution<float> jd(0.0f, w_mean * jitter);
    for (int i = 0; i < n; ++i) {
        int d = (n == 1) ? dmin : dmin + (int)((long long)(dmax - dmin) * i / (n - 1));
        float w = w_mean + jd(gen);
        x.SourceNeuron.push_back(s); x.TargetNeuron.push_back(t); x.Type.push_back(type);
        x.Weight.push_back(w); x.MaxWeight.push_back(maxw); x.Delay.push_back(d);
        x.SynapseRule.push_back(synRule); x.TriggerRule.push_back(-1);
        x.SynapseModel.push_back(sm);
    }
    c.push_back(x);
}

static NeuronLayerDescription make_izh(int n, float a, float b, float cpar, float d) {
    NeuronLayerDescription l;
    l.ModelName = "TimeDrivenIzhikevic_Exponential_Decay";
    l.numberofneuron = n; l.isMonitored = false; l.isOutput = true;
    l.NeuronParameter["a"] = a; l.NeuronParameter["b"] = b;
    l.NeuronParameter["c"] = cpar; l.NeuronParameter["d"] = d;
    l.NeuronParameter["V_rest"] = -65.0f; // c alone controls reset; V_reset would override it.
    l.NeuronParameter["V_th"] = 30.0f; l.NeuronParameter["R"] = 1.0f;
    l.NeuronParameter["ampa_tau"] = 5.0f; l.NeuronParameter["gaba_tau"] = 10.0f;
    l.NeuronParameter["random_sigma"] = std::array<float, 6>{0, 0, 0, 0, 0, 0};
    return l;
}

// deterministic delay diagnostic: InputSpike fires at a KNOWN raw step through
// two Type-4 PaperPSP connections with different delays; verify the rule sees
// arrival = raw + delay (i.e. SpikeTime already contains the delay).
static int run_tau_diag(Config cfg) {
    const int T = 100;
    const int D1 = 10, D2 = 1000;
    std::filesystem::create_directories(cfg.out);
    std::list<NeuronLayerDescription> layers;
    NeuronLayerDescription inp; inp.ModelName = "InputSpikeNeuronModel"; inp.numberofneuron = 1;
    layers.push_back(inp);
    layers.push_back(make_izh(1, 0.02f, 0.20f, -50.0f, 2.0f));
    std::list<ConnectionDescription> conns;
    ModelDescription psp = MakePaperPSP();
    add_single(conns, 0, 1, 4, 5.0f, D1, 0, cfg.w_max, psp);
    add_single(conns, 0, 1, 4, 5.0f, D2, 0, cfg.w_max, psp);
    std::list<LearningRuleDescription> rules;
    LearningRuleDescription da; da.RuleName = "DA-STDP";
    da.RuleParameter["tau_c"] = cfg.tau_c; rules.push_back(da);
    {
        Simulation sim(layers, conns, rules, T + D2 + 50, DT, 1);
        sim.InitSimulation();
        DASTDP* rule = dynamic_cast<DASTDP*>(sim.network->LearningRules[0]);
        rule->Collect(sim.network);
        rule->tracing = true;
        sim.AddExternalSpikeActivity({T}, {0});
        sim.RunSimulationStep(T + D2 + 50);

        std::ofstream csv(cfg.out + "/tau_delay_trace.csv");
        csv << "synapse_index,raw_pre_spike_step,connection_delay_steps,arrival_step,"
               "SpikeTime_received_by_learning_rule,last_post_step,tau_ms,STDP_tau,"
               "eligibility_before,eligibility_after\n";
        for (const auto& r : rule->trace) {
            csv << r.connIndex << "," << T << "," << r.delayStep << "," << (T + r.delayStep) << ","
                << r.spikeTime << "," << r.lastPostStep << "," << r.tau_ms << "," << r.stdp << ","
                << r.eligBefore << "," << r.eligAfter << "\n";
        }
        csv.close();
        bool f1 = false, f2 = false;
        for (const auto& r : rule->trace) {
            if (r.delayStep == D1 && r.spikeTime == T + D1) f1 = true;
            if (r.delayStep == D2 && r.spikeTime == T + D2) f2 = true;
        }
        std::cout << "tau/delay diagnostic (raw pre step " << T << ", delays " << D1 << "/" << D2 << "):\n";
        std::cout << "  [" << (f1 ? "PASS" : "FAIL") << "] delay " << D1 << ": SpikeTime==raw+delay ("
                  << T + D1 << ")\n";
        std::cout << "  [" << (f2 ? "PASS" : "FAIL") << "] delay " << D2 << ": SpikeTime==raw+delay ("
                  << T + D2 << ")\n";
        std::cout << "  -> SpikeTime is the ARRIVAL time; delay already included (do NOT add again)\n";
        bool ok = f1 && f2;
        std::cout << (ok ? "TAU DIAG PASSED\n" : "TAU DIAG FAILED\n");
        return ok ? 0 : 1;
    }
}

// minimal diagnostic: does a Type-3 external current persist after events stop?
static int run_diag_noise(Config cfg) {
    std::filesystem::create_directories(cfg.out);
    std::list<NeuronLayerDescription> layers;
    NeuronLayerDescription in; in.ModelName = "InputCurrentNeuronModel"; in.numberofneuron = 1;
    layers.push_back(in);
    NeuronLayerDescription tgt = make_izh(1, 0.02f, 0.20f, -50.0f, 2.0f);
    tgt.isMonitored = true;
    layers.push_back(tgt);
    std::list<ConnectionDescription> conns;
    add_single(conns, 0, 1, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});   // Type-3 current -> target
    std::list<LearningRuleDescription> rules;
    const int T = 200;
    {
        Simulation sim(layers, conns, rules, T, DT, 1);
        sim.AddFileOutputMonitorDriver(cfg.out + "/noise_diag_monitor.txt");
        sim.InitSimulation();
        sim.AddExternalCurrentActivity({20, 80}, {0, 0}, {5.0f, 0.0f});   // set 5.0 at 20, 0.0 at 80
        sim.RunSimulationStep(T);
    }
    std::vector<int> t; std::vector<float> iext;
    {
        std::ifstream in(cfg.out + "/noise_diag_monitor.txt");
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line); int tt, id; float v, u, ga, gg, gn, ie;
            ss >> tt >> id >> v >> u >> ga >> gg >> gn >> ie;
            if (id == 1) { t.push_back(tt); iext.push_back(ie); }
        }
    }
    std::ofstream csv(cfg.out + "/noise_boundary_trace.csv");
    csv << "time_step,neuron,I_ext\n";
    float at25 = 0, at60 = 0, at120 = 0;
    for (size_t i = 0; i < t.size(); ++i) {
        csv << t[i] << ",1," << iext[i] << "\n";
        if (t[i] == 25) at25 = iext[i];
        if (t[i] == 60) at60 = iext[i];
        if (t[i] == 120) at120 = iext[i];
    }
    csv.close();
    bool held = std::fabs(at60 - at25) < 1e-3 && std::fabs(at25) > 1e-3;
    bool cleared = std::fabs(at120) < 1e-3;
    std::cout << "noise persistence diagnostic (Type-3 current to a neuron):\n";
    std::cout << "  request 5.0 at step 20, request 0.0 at step 80\n";
    std::cout << "  I_ext at step 25  = " << at25 << "\n";
    std::cout << "  I_ext at step 60  = " << at60 << "  -> "
              << (held ? "HELD (persistent, NOT auto-zero)" : "changed") << "\n";
    std::cout << "  I_ext at step 120 = " << at120 << "  -> "
              << (cleared ? "cleared by the explicit 0 event (1-step delay)" : "still nonzero") << "\n";
    bool ok = held && cleared;
    std::cout << (ok ? "NOISE PERSISTENCE CONFIRMED; must send explicit 0 events\n"
                     : "UNEXPECTED noise behaviour\n");
    return ok ? 0 : 1;
}

// independent evaluation: a FRESH Simulation per trial (no residual potential /
// PSP / eligibility / pending events / buffers), fixed weights, d=0.
static int run_eval_fresh(Config cfg) {
    std::ifstream wf(cfg.weights_file);
    std::vector<float> ws; int wid; float wv;
    struct WeightIdentity { int id, source, target, delay; float weight; };
    std::map<int,WeightIdentity> identities;
    bool identityFormat=(std::filesystem::path(cfg.weights_file).extension()==".csv");
    if (identityFormat) {
        std::string line;
        std::getline(wf,line);
        if (line!="conn_id,source,target,delay_steps,weight") return 3;
        while (std::getline(wf,line)) {
            std::replace(line.begin(),line.end(),',',' ');
            std::istringstream row(line); WeightIdentity x; std::string extra;
            if (!(row>>x.id>>x.source>>x.target>>x.delay>>x.weight) || (row>>extra) ||
                !std::isfinite(x.weight) || x.weight<0 || x.weight>cfg.w_max ||
                !identities.emplace(x.id,x).second) {
                std::cerr << "Invalid identity weight row\n"; return 3;
            }
            ws.push_back(x.weight);
        }
    } else {
        while (wf >> wid >> wv) {
            if (wid != (int)ws.size() || !std::isfinite(wv) || wv<0 || wv>cfg.w_max) {
                std::cerr << "Invalid legacy weight row ID/value\n"; return 3;
            }
            ws.push_back(wv);
        }
        if (!wf.eof()) { std::cerr << "Malformed weight file\n"; return 3; }
    }
    if (ws.size() != 4u * N_SYN) { std::cerr << "weight count " << ws.size()
                                            << " != " << 4 * N_SYN << "\n"; return 3; }
    std::filesystem::create_directories(cfg.out);
    std::vector<int> cues(100);
    for (int k = 0; k < 100; ++k) cues[k] = (k < 50) ? 0 : 1;   // 50 LEFT + 50 RIGHT
    { std::mt19937 sg(cfg.eval_seed); std::shuffle(cues.begin(), cues.end(), sg); }
    const int T = INIT_OFFSET + DECISION_STEPS;

    std::ofstream csv(cfg.out + "/evaluation.csv");
    csv << "trial,cue_side,action,n_left,n_right,first_left,first_right,success\n";
    int succ = 0, none = 0, succL = 0, totL = 0, succR = 0, totR = 0;
    double maxdw = 0.0;
    for (int tr = 0; tr < 100; ++tr) {
        std::list<NeuronLayerDescription> layers;
        NeuronLayerDescription cue; cue.ModelName = "InputCurrentNeuronModel"; cue.numberofneuron = 2;
        layers.push_back(cue);
        layers.push_back(make_izh(2, 0.02f, 0.20f, -65.0f, 8.0f));
        layers.push_back(make_izh(4, 0.02f, 0.20f, -50.0f, 2.0f));
        layers.push_back(make_izh(4, 0.02f, 0.25f, -65.0f, 2.0f));
        NeuronLayerDescription nz; nz.ModelName = "InputCurrentNeuronModel"; nz.numberofneuron = 4;
        layers.push_back(nz);
        std::list<ConnectionDescription> conns;
        ModelDescription psp = MakePaperPSP();
        add_single(conns, ID_CUE_L, ID_RS_L, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
        add_single(conns, ID_CUE_R, ID_RS_R, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
        std::mt19937 gen(cfg.seed);   // same initial jitter as training; weights overwritten below
        add_bundle(conns, ID_RS_L, ID_CH_LL, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);
        add_bundle(conns, ID_RS_L, ID_CH_LR, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);
        add_bundle(conns, ID_RS_R, ID_CH_RL, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);
        add_bundle(conns, ID_RS_R, ID_CH_RR, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);
        add_single(conns, ID_CH_LL, ID_LTS_LL, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
        add_single(conns, ID_CH_LR, ID_LTS_LR, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
        add_single(conns, ID_CH_RL, ID_LTS_RL, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
        add_single(conns, ID_CH_RR, ID_LTS_RR, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
        add_single(conns, ID_LTS_LL, ID_CH_LR, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);
        add_single(conns, ID_LTS_LR, ID_CH_LL, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);
        add_single(conns, ID_LTS_RL, ID_CH_RR, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);
        add_single(conns, ID_LTS_RR, ID_CH_RL, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);
        add_single(conns, ID_NZ_LL, ID_CH_LL, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
        add_single(conns, ID_NZ_LR, ID_CH_LR, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
        add_single(conns, ID_NZ_RL, ID_CH_RL, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
        add_single(conns, ID_NZ_RR, ID_CH_RR, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
        std::list<LearningRuleDescription> rules;
        LearningRuleDescription da; da.RuleName = "DA-STDP"; da.RuleParameter["tau_c"] = cfg.tau_c;
        rules.push_back(da);

        Simulation sim(layers, conns, rules, T, DT, 1);
        sim.InitSimulation();
        DASTDP* rule = dynamic_cast<DASTDP*>(sim.network->LearningRules[0]);
        rule->Collect(sim.network);
        for (size_t i = 0; i < rule->plastic.size(); ++i) {
            auto* c=rule->plastic[i];
            if (identityFormat) {
                auto it=identities.find(c->Index);
                if (it==identities.end() || it->second.source!=c->SourceNeuron->Neuron_index ||
                    it->second.target!=c->TargetNeuron->Neuron_index || it->second.delay!=c->delay) {
                    std::cerr << "Connection identity mismatch\n"; return 3;
                }
                c->weight=it->second.weight;
            } else c->weight = ws[i];
        }
        std::vector<float> before; for (auto* c : rule->plastic) before.push_back(c->weight);

        int act = (cues[tr] == 0) ? ID_CUE_L : ID_CUE_R, ina = (cues[tr] == 0) ? ID_CUE_R : ID_CUE_L;
        sim.AddExternalCurrentActivity({INIT_OFFSET, INIT_OFFSET}, {act, ina}, {cfg.i_rs, 0.0f});
        int ch0 = (cues[tr] == 0) ? ID_NZ_LL : ID_NZ_RL, ch1 = (cues[tr] == 0) ? ID_NZ_LR : ID_NZ_RR;
        std::mt19937 ng(cfg.eval_seed * 7u + (unsigned)tr + 1u);
        std::normal_distribution<float> nd(0.0f, cfg.noise_sigma);
        std::vector<int> nt, n0, n1; std::vector<float> v0, v1;
        for (int s = INIT_OFFSET; s < INIT_OFFSET + DECISION_STEPS; ++s) {
            nt.push_back(s); n0.push_back(ch0); n1.push_back(ch1);
            v0.push_back(nd(ng)); v1.push_back(nd(ng));
        }
        sim.AddExternalCurrentActivity(nt, n0, v0);
        sim.AddExternalCurrentActivity(nt, n1, v1);

        int chL = (cues[tr] == 0) ? ID_CH_LL : ID_CH_RL, chR = (cues[tr] == 0) ? ID_CH_LR : ID_CH_RR;
        int nL = 0, nR = 0; long long fL = -1, fR = -1;
        for (int step = 0; step < T; ++step) {
            sim.RunSimulationStep(1);
            int tt, cc;
            while (sim.output_spike_driver->RemoveBufferedSpike(tt, cc)) {
                if (step >= INIT_OFFSET) {
        
            if (cc == chL) { nL++; if (fL < 0) fL = step + 1; }
                    else if (cc == chR) { nR++; if (fR < 0) fR = step + 1; }
                }
            }
        }
        int action;
        if (nL > nR) action = 0; else if (nR > nL) action = 1;
        else if (fL >= 0 && (fR < 0 || fL < fR)) action = 0;
        else if (fR >= 0 && (fL < 0 || fR < fL)) action = 1; else action = 2;
        bool toward = (action != 2) && ((cues[tr] == 0 && action == 0) || (cues[tr] == 1 && action == 1));
        bool success = (cfg.mode == "punishment") ? (action != 2 && !toward) : toward;
        if (action == 2) none++;
        if (success) succ++;
        if (cues[tr] == 0) { totL++; if (success) succL++; } else { totR++; if (success) succR++; }
        for (size_t i = 0; i < rule->plastic.size(); ++i)
            maxdw = std::max(maxdw, (double)std::fabs(rule->plastic[i]->weight - before[i]));
        csv << tr << "," << (cues[tr] == 0 ? "LEFT" : "RIGHT") << ","
            << (action == 0 ? "TURN_LEFT" : (action == 1 ? "TURN_RIGHT" : "NO_DECISION")) << ","
            << nL << "," << nR << "," << fL << "," << fR << "," << (success ? 1 : 0) << "\n";
    }
    csv.close();
    bool finite_ok = true, range_ok = true;
    for (float x : ws) { if (!std::isfinite(x)) finite_ok = false; if (x < 0.0f || x > cfg.w_max + 1e-6f) range_ok = false; }
    std::cout << "EVAL weights=" << cfg.weights_file << " mode=" << cfg.mode << " eval_seed=" << cfg.eval_seed << "\n";
    std::cout << "  count=" << ws.size() << " finite=" << finite_ok << " in_range=" << range_ok << "\n";
    std::cout << "  overall=" << (double)succ / 100 << "  LEFT=" << (double)succL / (totL ? totL : 1)
              << " (" << succL << "/" << totL << ")  RIGHT=" << (double)succR / (totR ? totR : 1)
              << " (" << succR << "/" << totR << ")\n";
    std::cout << "  no_decision=" << (double)none / 100 << "  max|dw|=" << maxdw << "\n";
    bool ok = finite_ok && range_ok && (maxdw < 1e-6);
    std::cout << (ok ? "EVAL OK (weights fixed across eval)\n" : "EVAL FAILED\n");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    Config cfg;
    bool diag = false, diag_noise = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) {
            if (i + 1 >= argc) { std::cerr << "missing value for " << name << "\n"; std::exit(2); }
            return std::string(argv[++i]);
        };
        if (a == "--mode") cfg.mode = next("--mode");
        else if (a == "--trials") cfg.trials = std::stoi(next("--trials"));
        else if (a == "--seed") cfg.seed = (unsigned)std::stoul(next("--seed"));
        else if (a == "--tau-c") cfg.tau_c = std::stof(next("--tau-c"));
        else if (a == "--w-max") cfg.w_max = std::stof(next("--w-max"));
        else if (a == "--noise-sigma") cfg.noise_sigma = std::stof(next("--noise-sigma"));
        else if (a == "--learning-ms") cfg.learning_ms = std::stof(next("--learning-ms"));
        else if (a == "--freeze") cfg.freeze = (std::stoi(next("--freeze")) != 0);
        else if (a == "--out") cfg.out = next("--out");
        else if (a == "--eval") { cfg.eval = true; cfg.weights_file = next("--eval"); }
        else if (a == "--eval-seed") cfg.eval_seed = (unsigned)std::stoul(next("--eval-seed"));
        else if (a == "--diagnostics") cfg.diagnostics = true;
        else if (a == "--pair-sign") {
            std::string sign = next("--pair-sign");
            if (sign != "paper2020" && sign != "reference2017") return 2;
            cfg.referenceSign = (sign == "reference2017");
        }
        else if (a == "--feedback-cue") {
            std::string cue = next("--feedback-cue");
            if (cue != "hold" && cue != "off") return 2;
            cfg.feedbackCueOff = (cue == "off");
        }
        else if (a == "--pre-clock") {
            std::string clock = next("--pre-clock");
            if (clock != "arrival" && clock != "emission") return 2;
            cfg.emissionClock = (clock == "emission");
        }
        else if (a == "--diag-tau") diag = true;
        else if (a == "--diag-noise") diag_noise = true;
        else { std::cerr << "unknown arg " << a << "\n"; return 2; }
    }
    if (cfg.mode!="reward" && cfg.mode!="punishment" && cfg.mode!="control") return 2;
    if (cfg.trials<=0 || !std::isfinite(cfg.tau_c) || cfg.tau_c<=0 ||
        !std::isfinite(cfg.w_max) || cfg.w_max<cfg.w_mean ||
        !std::isfinite(cfg.learning_ms) || cfg.learning_ms<0 ||
        !std::isfinite(cfg.noise_sigma) || cfg.noise_sigma<0) return 2;
    if (diag) return run_tau_diag(cfg);
    if (diag_noise) return run_diag_noise(cfg);
    if (cfg.eval) return run_eval_fresh(cfg);

    if (cfg.eval) cfg.seed = cfg.eval_seed;   // evaluation uses an independent seed
    const int learning_steps = cfg.eval ? 50 : (int)std::lround(cfg.learning_ms / DT);
    const int T_TRIAL = DECISION_STEPS + learning_steps;
    const float lr = 0.00025f * cfg.w_max;
    std::filesystem::create_directories(cfg.out);

    std::list<NeuronLayerDescription> layers;
    NeuronLayerDescription cue; cue.ModelName = "InputCurrentNeuronModel"; cue.numberofneuron = 2;
    layers.push_back(cue);
    layers.push_back(make_izh(2, 0.02f, 0.20f, -65.0f, 8.0f));
    layers.push_back(make_izh(4, 0.02f, 0.20f, -50.0f, 2.0f));
    layers.push_back(make_izh(4, 0.02f, 0.25f, -65.0f, 2.0f));
    NeuronLayerDescription nz; nz.ModelName = "InputCurrentNeuronModel"; nz.numberofneuron = 4;
    layers.push_back(nz);

    std::list<ConnectionDescription> conns;
    ModelDescription psp = MakePaperPSP();
    add_single(conns, ID_CUE_L, ID_RS_L, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
    add_single(conns, ID_CUE_R, ID_RS_R, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});

    std::mt19937 gen(cfg.seed);
    add_bundle(conns, ID_RS_L, ID_CH_LL, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);
    add_bundle(conns, ID_RS_L, ID_CH_LR, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);
    add_bundle(conns, ID_RS_R, ID_CH_RL, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);
    add_bundle(conns, ID_RS_R, ID_CH_RR, N_SYN, DELAY_MIN, DELAY_MAX, 4, gen, cfg.w_mean, cfg.w_jitter, 0, cfg.w_max, psp);

    add_single(conns, ID_CH_LL, ID_LTS_LL, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
    add_single(conns, ID_CH_LR, ID_LTS_LR, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
    add_single(conns, ID_CH_RL, ID_LTS_RL, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
    add_single(conns, ID_CH_RR, ID_LTS_RR, 4, cfg.w_ch_lts, 10, -1, cfg.w_ch_lts, psp);
    add_single(conns, ID_LTS_LL, ID_CH_LR, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);
    add_single(conns, ID_LTS_LR, ID_CH_LL, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);
    add_single(conns, ID_LTS_RL, ID_CH_RR, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);
    add_single(conns, ID_LTS_RR, ID_CH_RL, 4, -cfg.w_lts_ch, 20, -1, cfg.w_lts_ch, psp);

    add_single(conns, ID_NZ_LL, ID_CH_LL, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
    add_single(conns, ID_NZ_LR, ID_CH_LR, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
    add_single(conns, ID_NZ_RL, ID_CH_RL, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});
    add_single(conns, ID_NZ_RR, ID_CH_RR, 3, 1.0f, 1, -1, 1.0f, ModelDescription{});

    std::list<LearningRuleDescription> rules;
    LearningRuleDescription da; da.RuleName = "DA-STDP";
    da.RuleParameter["tau_c"] = cfg.tau_c;
    da.RuleParameter["b"] = -0.21f;
    da.RuleParameter["c_peak"] = -12.7f;
    da.RuleParameter["beta"] = 13.61f;
    rules.push_back(da);

    const int total_steps = INIT_OFFSET + cfg.trials * T_TRIAL;
    Simulation sim(layers, conns, rules, total_steps, DT, 1);
    sim.AddFileOutputSpikeDriver(cfg.out + "/spikes.txt");
    sim.InitSimulation();
    fig4::require_paper_ch(sim,4,4);

    DASTDP* rule = dynamic_cast<DASTDP*>(sim.network->LearningRules[0]);
    if (rule == 0) { std::cerr << "DA-STDP rule not created\n"; return 3; }
    rule->Collect(sim.network);
    if (rule->plastic.empty()) { std::cerr << "no plastic connections collected\n"; return 3; }

    if (cfg.eval) {
        std::ifstream wf(cfg.weights_file);
        int id; float w; std::vector<float> ws;
        while (wf >> id >> w) ws.push_back(w);
        if (ws.size() == rule->plastic.size()) {
            for (size_t i = 0; i < ws.size(); ++i) rule->plastic[i]->weight = ws[i];
            std::cout << "loaded " << ws.size() << " weights from " << cfg.weights_file << "\n";
        } else {
            std::cerr << "weight file size mismatch (" << ws.size() << " vs "
                      << rule->plastic.size() << ")\n";
            return 3;
        }
    }

    std::vector<int> cues(cfg.trials);
    {
        std::mt19937 sg(cfg.seed ^ 0x9e3779b9u);
        for (int k = 0; k < cfg.trials; ++k) cues[k] = (k % 2 == 0) ? 0 : 1;
        std::shuffle(cues.begin(), cues.end(), sg);
    }
    std::vector<int> cue_t, cue_n; std::vector<float> cue_v;
    for (int k = 0; k < cfg.trials; ++k) {
        int t0 = INIT_OFFSET + k * T_TRIAL;
        int act = (cues[k] == 0) ? ID_CUE_L : ID_CUE_R;
        int ina = (cues[k] == 0) ? ID_CUE_R : ID_CUE_L;
        cue_t.push_back(t0); cue_n.push_back(act); cue_v.push_back(cfg.i_rs);
        cue_t.push_back(t0); cue_n.push_back(ina); cue_v.push_back(0.0f);
    }
    // independent white noise on the two CH of the active unit ONLY during its
    // decision window (compact per-channel event lists)
    std::vector<std::vector<int>> nz_t(4);
    std::vector<std::vector<float>> nz_v(4);
    {
        std::vector<std::mt19937> gens;
        for (int ch = 0; ch < 4; ++ch) gens.emplace_back(cfg.seed * 131u + (unsigned)ch + 1u);
        std::vector<std::normal_distribution<float>> dists(4, std::normal_distribution<float>(0.0f, cfg.noise_sigma));
        for (int k = 0; k < cfg.trials; ++k) {
            int t0 = INIT_OFFSET + k * T_TRIAL;
            bool left = (cues[k] == 0);
            int a0 = left ? 0 : 2, a1 = left ? 1 : 3;
            for (int s = t0; s < t0 + DECISION_STEPS; ++s) {
                nz_t[a0].push_back(s); nz_v[a0].push_back(dists[a0](gens[a0]));
                nz_t[a1].push_back(s); nz_v[a1].push_back(dists[a1](gens[a1]));
            }
            // Type-3 currents PERSIST at their last value, so explicitly set ALL
            // four noise injectors to 0 right after the decision window (arrives
            // at the CH one step later due to the delay-1 connection).
            int t_end = t0 + DECISION_STEPS;
            for (int ch = 0; ch < 4; ++ch) { nz_t[ch].push_back(t_end); nz_v[ch].push_back(0.0f); }
        }
    }
    const int nz_id[4] = {ID_NZ_LL, ID_NZ_LR, ID_NZ_RL, ID_NZ_RR};
    for (int ch = 0; ch < 4; ++ch) {
        if (!nz_t[ch].empty()) {
            std::vector<int> ids(nz_t[ch].size(), nz_id[ch]);
            sim.AddExternalCurrentActivity(nz_t[ch], ids, nz_v[ch]);
        }
    }
    sim.AddExternalCurrentActivity(cue_t, cue_n, cue_v);

    auto snapshot = [&](std::vector<float>& out) {
        out.clear(); for (auto* c : rule->plastic) out.push_back(c->weight);
    };
    std::vector<float> w_init; snapshot(w_init);

    struct GroupMean { float wll, wlr, wrl, wrr, wc, ww, gap; };
    auto group_mean = [&]() {
        double sll = 0, slr = 0, srl = 0, srr = 0; int nll = 0, nlr = 0, nrl = 0, nrr = 0;
        for (auto* c : rule->plastic) {
            int s = c->SourceNeuron->Neuron_index, t = c->TargetNeuron->Neuron_index;
            float w = c->weight;
            if (s == ID_RS_L && t == ID_CH_LL) { sll += w; nll++; }
            else if (s == ID_RS_L && t == ID_CH_LR) { slr += w; nlr++; }
            else if (s == ID_RS_R && t == ID_CH_RL) { srl += w; nrl++; }
            else if (s == ID_RS_R && t == ID_CH_RR) { srr += w; nrr++; }
        }
        GroupMean g;
        g.wll = nll ? (float)(sll / nll) : 0.0f;
        g.wlr = nlr ? (float)(slr / nlr) : 0.0f;
        g.wrl = nrl ? (float)(srl / nrl) : 0.0f;
        g.wrr = nrr ? (float)(srr / nrr) : 0.0f;
        g.wc = 0.5f * (g.wll + g.wrr);
        g.ww = 0.5f * (g.wlr + g.wrl);
        g.gap = g.wc - g.ww;
        return g;
    };

    std::ofstream csv(cfg.out + "/" + (cfg.eval ? std::string("evaluation.csv") : std::string("training.csv")));
    csv << "trial,seed,mode,cue_side,action,n_left,n_right,first_left,first_right,D,feedback,success,"
           "cumulative_success_rate,mean_W_LL,mean_W_LR,mean_W_RL,mean_W_RR,mean_correct,mean_wrong,"
           "correct_wrong_gap\n";

    rule->tracing = true; rule->traceLimit = 40;
    rule->emissionClock = cfg.emissionClock;
    rule->referenceSign = cfg.referenceSign;
    rule->diagnostics = cfg.diagnostics;
    std::ofstream mechanism, phaseSpikes;
    std::array<int,16> spikeCounts{};
    if (cfg.diagnostics) {
        mechanism.open(cfg.out+"/mechanism_synapses.csv");
        mechanism << std::setprecision(17);
        mechanism << "trial,phase,cue,action,d,conn_id,state_index,source,target,delay_steps,w_before,w_after,c_begin,c_end,integral_c_dt,integral_carried_c_dt,predicted_dw,actual_dw,clip_correction,round_correction,stdp_positive,stdp_negative,pre_events,paired_events,sum_tau_ms,clip_low_steps,clip_high_steps\n";
        phaseSpikes.open(cfg.out+"/phase_spikes.csv");
        phaseSpikes << "trial,phase,neuron,count\n";
        rule->pairCsv.open(cfg.out+"/mechanism_pairs.csv");
        rule->pairCsv << std::setprecision(10);
        rule->pairCsv << "trial,phase,conn_id,state_index,source,target,delay_steps,raw_pre_step,arrival_step,learning_step,last_post_step,tau_ms,stdp,c_before,c_after\n";
    }
    auto beginPhase = [&](int tr, const std::string& phase) {
        if (cfg.diagnostics) { spikeCounts.fill(0); rule->BeginPhase(tr,phase); }
    };
    auto endPhase = [&](int tr, const std::string& phase, int action, float d) {
        if (cfg.diagnostics) {
            rule->EndPhase(mechanism,cues[tr],action,d);
            for(int n=2;n<12;++n) phaseSpikes << tr << ',' << phase << ',' << n << ',' << spikeCounts[n] << '\n';
        }
    };
    int global_step = 0;
    auto drain_counts = [&](int chL, int chR, int& nL, int& nR, long long& fL, long long& fR) {
        int tt, cc;
        while (sim.output_spike_driver->RemoveBufferedSpike(tt, cc)) {
            if (cfg.diagnostics && cc>=0 && cc<16) spikeCounts[cc]++;
            if (cc == chL) { nL++; if (fL < 0) fL = global_step; }
            else if (cc == chR) { nR++; if (fR < 0) fR = global_step; }
        }
    };
    // drains the spike buffer WITHOUT reading/incrementing any counters
    auto drain_clear = [&]() { int tt, cc; while (sim.output_spike_driver->RemoveBufferedSpike(tt, cc)) {
        if (cfg.diagnostics && cc>=0 && cc<16) spikeCounts[cc]++;
    } };

    int successes = 0, decided = 0, feedback_count = 0, toward_count = 0;
    int successesL = 0, totalL = 0, successesR = 0, totalR = 0;
    std::vector<int> action_hist(3, 0);
    for (int trial = 0; trial < cfg.trials; ++trial) {
        int t0 = INIT_OFFSET + trial * T_TRIAL;
        while (global_step < t0) { sim.RunSimulationStep(1); global_step++; drain_clear(); rule->Step(&sim, 0.0f); }

        beginPhase(trial,"decision");
        int chL = (cues[trial] == 0) ? ID_CH_LL : ID_CH_RL;
        int chR = (cues[trial] == 0) ? ID_CH_LR : ID_CH_RR;
        int nL = 0, nR = 0; long long fL = -1, fR = -1;
        for (int s = 0; s < DECISION_STEPS; ++s) {
            sim.RunSimulationStep(1); global_step++;
            drain_counts(chL, chR, nL, nR, fL, fR);
            rule->Step(&sim, 0.0f);
        }
        float D = (nL + nR) ? (float)(nR - nL) / (float)(nL + nR) : 0.0f;
        int action;
        if (nL > nR) action = 0;
        else if (nR > nL) action = 1;
        else if (fL >= 0 && (fR < 0 || fL < fR)) action = 0;
        else if (fR >= 0 && (fL < 0 || fR < fL)) action = 1;
        else action = 2;
        action_hist[action]++;
        bool toward = (action != 2) && ((cues[trial] == 0 && action == 0) || (cues[trial] == 1 && action == 1));
        if (action != 2) decided++;
        if (toward) toward_count++;
        bool success = (cfg.mode == "punishment") ? (action != 2 && !toward) : toward;
        if (success) successes++;
        if (cues[trial] == 0) { totalL++; if (success) successesL++; }
        else { totalR++; if (success) successesR++; }

        float d = 0.0f; int feedback = 0;
        if (!cfg.eval) {
            if (cfg.mode == "reward" && toward) { d = +lr; feedback = 1; }
            else if (cfg.mode == "punishment" && toward) { d = -lr; feedback = -1; }
        }
        if (feedback != 0) feedback_count++;

        // A separate center-input assumption, NOT an established paper fix.
        // This changes cue activity, not eligibility or action-specific gates.
        if (cfg.feedbackCueOff && feedback != 0) {
            sim.AddExternalCurrentActivity({global_step, global_step},
                {ID_CUE_L, ID_CUE_R}, {0.0f, 0.0f});
        }

        endPhase(trial,"decision",action,0.0f);
        beginPhase(trial,"feedback");
        rule->blockNewEligibility = cfg.freeze;
        for (int s = 0; s < learning_steps; ++s) {
            sim.RunSimulationStep(1); global_step++;
            drain_clear();
            rule->Step(&sim, d);
        }
        endPhase(trial,"feedback",action,d);
        rule->blockNewEligibility = false;

        GroupMean g = group_mean();
        csv << trial << "," << cfg.seed << "," << cfg.mode << ","
            << (cues[trial] == 0 ? "LEFT" : "RIGHT") << ","
            << (action == 0 ? "TURN_LEFT" : (action == 1 ? "TURN_RIGHT" : "NO_DECISION")) << ","
            << nL << "," << nR << "," << fL << "," << fR << "," << D << ","
            << feedback << "," << (success ? 1 : 0) << ","
            << (decided ? (double)successes / (trial + 1) : 0.0) << ","
            << g.wll << "," << g.wlr << "," << g.wrl << "," << g.wrr << ","
            << g.wc << "," << g.ww << "," << g.gap << "\n";
    }
    csv.close();

    {
        std::ofstream tc(cfg.out + "/tau_delay_trace.csv");
        tc << "synapse_index,raw_pre_spike_step,connection_delay_steps,arrival_step,"
              "SpikeTime_received_by_learning_rule,last_post_step,tau_ms,STDP_tau,"
              "eligibility_before,eligibility_after\n";
        for (const auto& r : rule->trace) {
            tc << r.connIndex << "," << (cfg.emissionClock ? r.spikeTime : r.spikeTime-r.delayStep) << "," << r.delayStep << ","
               << (cfg.emissionClock ? r.spikeTime+r.delayStep : r.spikeTime) << "," << r.spikeTime << "," << r.lastPostStep << "," << r.tau_ms << ","
               << r.stdp << "," << r.eligBefore << "," << r.eligAfter << "\n";
        }
    }

    std::vector<float> w_final; snapshot(w_final);
    {
        std::ofstream wi(cfg.out + "/weights_init.txt");
        std::ofstream wf(cfg.out + "/weights_final.txt");
        wi << std::setprecision(9); wf << std::setprecision(9);
        std::ofstream wic(cfg.out+"/weights_init.csv"), wfc(cfg.out+"/weights_final.csv");
        wic << "conn_id,source,target,delay_steps,weight\n";
        wfc << "conn_id,source,target,delay_steps,weight\n";
        wic << std::setprecision(9); wfc << std::setprecision(9);
        std::ofstream identities(cfg.out+"/connection_identity.csv");
        identities << "legacy_row,conn_id,state_index,source,target,delay_steps,group,w_init,w_final\n";
        identities << std::setprecision(9);
        std::ofstream wd(cfg.out + "/weights_vs_delay.txt");
        wd << "group,delay_steps,delay_ms,w_init,w_final\n";
        const char* gname[4] = {"W_LL", "W_LR", "W_RL", "W_RR"};
        for (size_t i = 0; i < rule->plastic.size(); ++i) {
            wi << i << " " << w_init[i] << "\n";
            wf << i << " " << w_final[i] << "\n";
            auto* conn=rule->plastic[i];
            int grp=conn->TargetNeuron->Neuron_index-ID_CH_LL;
            if (grp<0 || grp>3) throw std::runtime_error("Unexpected plastic target");
            for (int snap=0;snap<2;++snap) {
                auto& wcsv=snap?wfc:wic;
                wcsv << conn->Index << ',' << conn->SourceNeuron->Neuron_index << ','
                    << conn->TargetNeuron->Neuron_index << ',' << conn->delay << ','
                    << (snap?w_final[i]:w_init[i]) << '\n';
            }
            identities << i << ',' << conn->Index << ',' << rule->plasticState[i] << ','
                << conn->SourceNeuron->Neuron_index << ',' << conn->TargetNeuron->Neuron_index << ','
                << conn->delay << ',' << gname[grp] << ',' << w_init[i] << ',' << w_final[i] << '\n';
            wd << gname[grp % 4] << "," << rule->plastic[i]->delay << ","
               << rule->plastic[i]->delay * DT << "," << w_init[i] << "," << w_final[i] << "\n";
        }
    }

    bool ok = true;
    auto report = [&](const std::string& nm, bool pass, const std::string& det) {
        std::cout << "[" << (pass ? "PASS" : "FAIL") << "] " << nm << " : " << det << "\n";
        ok = ok && pass;
    };
    double max_abs_change = 0.0; bool finite_ok = true, range_ok = true;
    for (size_t i = 0; i < rule->plastic.size(); ++i) {
        float w = rule->plastic[i]->weight;
        if (!std::isfinite(w)) finite_ok = false;
        if (w < 0.0f || w > cfg.w_max + 1e-6f) range_ok = false;
        max_abs_change = std::max(max_abs_change, (double)std::fabs(w - w_init[i]));
    }
    report("weights finite", finite_ok, "all plastic weights isfinite");
    report("weights in range", range_ok, "0 <= w <= w_max");
    {
        std::ifstream fi(cfg.out + "/weights_init.txt"), ff(cfg.out + "/weights_final.txt");
        int id; float wi_, wf_; bool init_match = true, final_match = true; size_t k = 0;
        while (fi >> id >> wi_) { if (k < w_init.size() && std::fabs(wi_ - w_init[k]) > 1e-4f) init_match = false; k++; }
        k = 0;
        while (ff >> id >> wf_) { if (k < w_final.size() && std::fabs(wf_ - w_final[k]) > 1e-4f) final_match = false; k++; }
        report("weights_init.txt == pre-run snapshot", init_match, "");
        report("weights_final.txt == post-run snapshot", final_match, "");
    }
    if (cfg.mode == "control") {
        report("control: weights unchanged", max_abs_change <= 1e-6,
               "max|dw|=" + std::to_string(max_abs_change));
    } else {
        report(cfg.mode + ": some weights changed", max_abs_change > 1e-6,
               "max|dw|=" + std::to_string(max_abs_change) + ", feedback events=" + std::to_string(feedback_count));
    }

    GroupMean gfin = group_mean();
    std::cout << "mode=" << cfg.mode << " trials=" << cfg.trials << " seed=" << cfg.seed
              << " learning_ms=" << cfg.learning_ms << " freeze=" << (cfg.freeze ? 1 : 0)
              << " lr=" << lr << " w_max=" << cfg.w_max << "\n";
    std::cout << "decided=" << decided << "/" << cfg.trials << "  toward=" << toward_count
              << "  success(" << cfg.mode << ")=" << successes << "\n";
    std::cout << "actions: left=" << action_hist[0] << " right=" << action_hist[1]
              << " none=" << action_hist[2] << "  no_decision_ratio="
              << (cfg.trials ? (double)action_hist[2] / cfg.trials : 0.0) << "\n";
    std::cout << "success by cue: LEFT=" << successesL << "/" << totalL
              << " (" << (totalL ? (double)successesL / totalL : 0.0) << ")"
              << "  RIGHT=" << successesR << "/" << totalR
              << " (" << (totalR ? (double)successesR / totalR : 0.0) << ")\n";
    std::cout << "eval max|dw| = " << max_abs_change << "\n";
    std::cout << "final groups: W_LL=" << gfin.wll << " W_LR=" << gfin.wlr
              << " W_RL=" << gfin.wrl << " W_RR=" << gfin.wrr << "\n";
    std::cout << "final correct=" << gfin.wc << " wrong=" << gfin.ww << " gap=" << gfin.gap << "\n";
    if (cfg.mode == "reward") {
        bool sep = (gfin.wll > gfin.wlr) && (gfin.wrr > gfin.wrl);
        std::cout << "[OBSERVATION] reward separation=" << sep << " gap=" << gfin.gap << "\n";
    }
    std::cout << "wrote " << cfg.out << "\n";
    return ok ? 0 : 1;
}
