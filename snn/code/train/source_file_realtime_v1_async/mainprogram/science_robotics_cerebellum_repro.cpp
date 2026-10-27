#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

struct ReproConfig {
    int dof = 2;
    int mf_per_joint = 28;
    int cf_per_joint = 100;
    int gc_per_joint = 2401;
    int pc_per_joint = 100;
    int dcn_per_joint = 100;

    int simulation_steps = 6000;
    float timestep_seconds = 0.0005f;
    int queue_count = 1;

    int rbf_bins_per_channel = 7;
    int rbf_channel_count = 4;
    // The original 7MF source uses joint-specific limits from launch files.
    // This 2DOF repro keeps one shared range, so use the envelope of the first
    // two Baxter joints to stay aligned with that source family.
    double mf_min_position = -2.14;
    double mf_max_position = 1.7;
    double mf_min_velocity = -2.0;
    double mf_max_velocity = 2.0;
    double mf_max_spike_frequency = 250.0;
    double mf_overlapping_factor = 1.05;

    double cf_error_max = 0.45;
    double cf_min_spike_frequency = 0.0;
    double cf_max_spike_frequency = 12.0;
    double cf_overlapping_factor = 1.2;
    int cf_burst_spacing_steps = 4;
    unsigned int poisson_seed = 7;

    double trajectory_frequency_hz = 0.35;
    double circle_center_x = 1.0;
    double circle_center_y = 0.2;
    double circle_radius = 0.3;
    double actual_gain = 0.9;
    double actual_lag_seconds = 0.04;
    double actual_bias = 0.03;
    int weight_save_interval_steps = 0;

    float mf_dcn_weight = 0.1f;
    float mf_grc_weight = 0.28f;
    float gc_pc_initial_weight = 1.6f;
    float gc_pc_max_weight = 5.0f;
    float io_pc_weight = 0.0f;
    float io_dcn_ampa_weight = 0.5f;
    float io_dcn_nmda_weight = 0.25f;
    float pc_dcn_weight = 1.0f;

    bool enable_mf_dcn = true;
    bool enable_cf_dcn = true;
    bool enable_pc_dcn = true;
    bool enable_learning = true;
    std::string neuron_model_profile =
#if SNN_CPU_ONLY
        "cpu_double_fallback";
#else
        "native_triple_gpu";
#endif

    bool debug_all_spike_output = true;
    float learning_rule_ltp = 0.002f;
    float learning_rule_ltd = -0.0008f;
    float learning_rule_maxpos = 110.0f;
    float learning_rule_initpos = 90.0f;

    bool debug_gc_output = false;
    bool diagnostic_print_time = false;
};

struct LayerOffsets {
    int joint_stride;
    int total_mf;
    int total_cf;
    int total_gc;
    int total_pc;
    int total_dcn;
};

struct PhysicalLifSpec {
    float v_rest;
    float v_reset;
    float v_th;
    float cm_farad;
    float grest_ns;
    float excit_tau_s;
    float inhibitory_tau_s;
    float nmda_tau_s;
    float excit_reversal_v;
    float inhibitory_reversal_v;
    float refractory_s;
    bool has_inhibitory = false;
    bool has_nmda = false;
};

std::string OutputRoot() {
    return "science_robotics_outputs";
}

bool EnvFlagOrDefault(const char* name, bool default_value) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return default_value;
    }
    const std::string text(value);
    if (text == "0" || text == "false" || text == "FALSE" || text == "False") {
        return false;
    }
    if (text == "1" || text == "true" || text == "TRUE" || text == "True") {
        return true;
    }
    return default_value;
}

int EnvIntOrDefault(const char* name, int default_value) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return default_value;
    }
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

float EnvFloatOrDefault(const char* name, float default_value) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return default_value;
    }
    try {
        return std::stof(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

std::string JoinPath(const std::string& base, const std::string& leaf) {
    return (std::filesystem::path(base) / leaf).string();
}

std::string RoundDirectory(const std::string& base, int round_index) {
    std::ostringstream stream;
    stream << "round_" << std::setw(3) << std::setfill('0') << round_index;
    return JoinPath(base, stream.str());
}

LayerOffsets ComputeOffsets(const ReproConfig& cfg) {
    LayerOffsets offsets{};
    offsets.joint_stride = cfg.mf_per_joint + cfg.cf_per_joint + cfg.gc_per_joint + cfg.pc_per_joint + cfg.dcn_per_joint;
    offsets.total_mf = cfg.dof * cfg.mf_per_joint;
    offsets.total_cf = cfg.dof * cfg.cf_per_joint;
    offsets.total_gc = cfg.dof * cfg.gc_per_joint;
    offsets.total_pc = cfg.dof * cfg.pc_per_joint;
    offsets.total_dcn = cfg.dof * cfg.dcn_per_joint;
    return offsets;
}

int MF0(const LayerOffsets& offsets, int joint) { return joint * offsets.joint_stride; }
int CF0(const ReproConfig& cfg, const LayerOffsets& offsets, int joint) { return MF0(offsets, joint) + cfg.mf_per_joint; }
int GC0(const ReproConfig& cfg, const LayerOffsets& offsets, int joint) { return CF0(cfg, offsets, joint) + cfg.cf_per_joint; }
int PC0(const ReproConfig& cfg, const LayerOffsets& offsets, int joint) { return GC0(cfg, offsets, joint) + cfg.gc_per_joint; }
int DCN0(const ReproConfig& cfg, const LayerOffsets& offsets, int joint) { return PC0(cfg, offsets, joint) + cfg.pc_per_joint; }

double StepSeconds(const ReproConfig& cfg) {
    return static_cast<double>(cfg.timestep_seconds);
}

float StepMilliseconds(const ReproConfig& cfg) {
    return cfg.timestep_seconds * 1000.0f;
}

float VoltageToModelUnits(float volts) {
    return volts * 1000.0f;
}

float TimeToModelMilliseconds(float seconds) {
    return seconds * 1000.0f;
}

void NormalizeConfigTo7MfSource(ReproConfig& cfg) {
    cfg.mf_per_joint = cfg.rbf_channel_count * cfg.rbf_bins_per_channel;

    int gc_per_joint = 1;
    for (int channel = 0; channel < cfg.rbf_channel_count; ++channel) {
        gc_per_joint *= cfg.rbf_bins_per_channel;
    }
    cfg.gc_per_joint = gc_per_joint;
}

float TauSecondsFromCmAndGrest(const PhysicalLifSpec& spec) {
    const float grest_siemens = spec.grest_ns * 1.0e-9f;
    if (grest_siemens <= 0.0f) {
        throw std::runtime_error("Invalid Grest when deriving tau.");
    }
    return spec.cm_farad / grest_siemens;
}

float ResistanceScaleFromGrest(const PhysicalLifSpec& spec) {
    if (spec.grest_ns <= 0.0f) {
        throw std::runtime_error("Invalid Grest when deriving R.");
    }
    // This model multiplies accumulated conductance-like weights by R numerically.
    // Matching the legacy examples requires using inverse Grest in nS scale, not SI ohms.
    return 1.0f / spec.grest_ns;
}

int RefractorySteps(const ReproConfig& cfg, float refractory_s) {
    return std::max(1, static_cast<int>(std::lround(refractory_s / StepSeconds(cfg))));
}

std::string TripleLifModelName() {
    return "TimeDrivenLIF_Exponential_triple_GPU";
}

std::string SelectedLifModelName(const ReproConfig& cfg) {
    if (cfg.neuron_model_profile == "native_triple_gpu") {
#if SNN_CPU_ONLY
        throw std::runtime_error(
            "Requested native_triple_gpu, but this is a CPU-only build. "
            "Use the GPU solution or switch REPRO_NEURON_MODEL=cpu_double_fallback.");
#else
        return TripleLifModelName();
#endif
    }
    if (cfg.neuron_model_profile == "cpu_double_fallback") {
        return "TimeDrivenLIF_Exponential_double";
    }
#if !SNN_CPU_ONLY
    if (cfg.neuron_model_profile == "edlutlike_gpu") {
        return "EDLUTLikeLIF_GPU";
    }
#endif
    throw std::runtime_error("Unknown REPRO_NEURON_MODEL profile: " + cfg.neuron_model_profile);
}

std::string BackendSummary() {
#if SNN_CPU_ONLY
    return "CPU";
#else
    return "GPU";
#endif
}

void SetExcitatoryParams(NeuronLayerDescription& layer, float reversal, float tau) {
#if SNN_CPU_ONLY
    layer.NeuronParameter["Eexc"] = reversal;
    layer.NeuronParameter["gexc_tau"] = tau;
#else
    layer.NeuronParameter["AMPA"] = true;
    layer.NeuronParameter["E_ampa"] = reversal;
    layer.NeuronParameter["ampa_tau"] = tau;
#endif
}

void SetInhibitoryParams(NeuronLayerDescription& layer, float reversal, float tau) {
#if SNN_CPU_ONLY
    layer.NeuronParameter["Einh"] = reversal;
    layer.NeuronParameter["ginh_tau"] = tau;
#else
    layer.NeuronParameter["GABA"] = true;
    layer.NeuronParameter["E_gaba"] = reversal;
    layer.NeuronParameter["gaba_tau"] = tau;
#endif
}

int NmdaConnectionType() {
#if SNN_CPU_ONLY
    return 0;
#else
    return 2;
#endif
}

NeuronLayerDescription MakeInputLayer(const ReproConfig& cfg, int count) {
    NeuronLayerDescription layer;
    layer.ModelName = "InputSpikeNeuronModel";
    layer.numberofneuron = count;
    layer.isMonitored = false;
    layer.isOutput = cfg.debug_all_spike_output;
    return layer;
}

void ApplyPhysicalLifParams(
    const ReproConfig& cfg,
    const PhysicalLifSpec& spec,
    NeuronLayerDescription& layer) {
    layer.NeuronParameter["tau"] = TimeToModelMilliseconds(TauSecondsFromCmAndGrest(spec));
    layer.NeuronParameter["R"] = ResistanceScaleFromGrest(spec);
    layer.NeuronParameter["V_rest"] = VoltageToModelUnits(spec.v_rest);
    layer.NeuronParameter["V_reset"] = VoltageToModelUnits(spec.v_reset);
    layer.NeuronParameter["V_th"] = VoltageToModelUnits(spec.v_th);
    layer.NeuronParameter["t_ref"] = RefractorySteps(cfg, spec.refractory_s);
    SetExcitatoryParams(
        layer,
        VoltageToModelUnits(spec.excit_reversal_v),
        TimeToModelMilliseconds(spec.excit_tau_s));
    if (spec.has_inhibitory) {
        SetInhibitoryParams(
            layer,
            VoltageToModelUnits(spec.inhibitory_reversal_v),
            TimeToModelMilliseconds(spec.inhibitory_tau_s));
    }
#if !SNN_CPU_ONLY
    if (spec.has_nmda) {
        layer.NeuronParameter["NMDA"] = true;
        layer.NeuronParameter["nmda_tau"] = TimeToModelMilliseconds(spec.nmda_tau_s);
    }
#endif
}

NeuronLayerDescription MakeGranuleLayer(const ReproConfig& cfg, int count) {
    NeuronLayerDescription layer;
    layer.ModelName = SelectedLifModelName(cfg);
    layer.numberofneuron = count;
    layer.isOutput = cfg.debug_all_spike_output || cfg.debug_gc_output;
    layer.isMonitored = false;
    ApplyPhysicalLifParams(
        cfg,
        PhysicalLifSpec{
            -0.065f,
            -0.065f,
            -0.050f,
            2.0e-12f,
            1.0f,
            1.0e-3f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0e-3f,
            false,
            false},
        layer);
#if SNN_CPU_ONLY
    layer.NeuronParameter["random_sigma"] = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
#else
    layer.NeuronParameter["random_sigma"] = std::array<float, 5>{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
#endif
    return layer;
}

NeuronLayerDescription MakePurkinjeLayer(const ReproConfig& cfg, int count) {
    NeuronLayerDescription layer;
    layer.ModelName = SelectedLifModelName(cfg);
    layer.numberofneuron = count;
    layer.isMonitored = true;
    layer.isOutput = cfg.debug_all_spike_output;
    ApplyPhysicalLifParams(
        cfg,
        PhysicalLifSpec{
            -0.070f,
            -0.070f,
            -0.052f,
            100.0e-12f,
            6.0f,
            1.2e-3f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            2.0e-3f,
            false,
            false},
        layer);
#if SNN_CPU_ONLY
    layer.NeuronParameter["random_sigma"] = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
#else
    layer.NeuronParameter["random_sigma"] = std::array<float, 5>{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
#endif
    return layer;
}

NeuronLayerDescription MakeDcnLayer(const ReproConfig& cfg, int count) {
    NeuronLayerDescription layer;
    layer.ModelName = SelectedLifModelName(cfg);
    layer.numberofneuron = count;
    layer.isMonitored = true;
    layer.isOutput = true;
    ApplyPhysicalLifParams(
        cfg,
        PhysicalLifSpec{
            -0.070f,
            -0.070f,
            -0.040f,
            2.0e-12f,
            0.2f,
            0.5e-3f,
            10.0e-3f,
            14.0e-3f,
            0.0f,
            -0.080f,
            1.0e-3f,
            true,
            true},
        layer);
#if SNN_CPU_ONLY
    layer.NeuronParameter["random_sigma"] = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
#else
    layer.NeuronParameter["random_sigma"] = std::array<float, 5>{0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
#endif
    return layer;
}

void AppendConnection(
    std::list<ConnectionDescription>& synapses,
    const std::vector<int>& sources,
    const std::vector<int>& targets,
    const std::vector<int>& types,
    const std::vector<float>& weights,
    const std::vector<float>& max_weights,
    const std::vector<int>& delays,
    const std::vector<int>& synapse_rules,
    const std::vector<int>& trigger_rules) {
    ConnectionDescription desc;
    desc.SourceNeuron = sources;
    desc.TargetNeuron = targets;
    desc.Type = types;
    desc.Weight = weights;
    desc.MaxWeight = max_weights;
    desc.Delay = delays;
    desc.SynapseRule = synapse_rules;
    desc.TriggerRule = trigger_rules;
    desc.SynapseModel.assign(sources.size(), ModelDescription{});
    synapses.push_back(std::move(desc));
}

void BuildNeuronLayers(const ReproConfig& cfg, std::list<NeuronLayerDescription>& neurons) {
    for (int joint = 0; joint < cfg.dof; ++joint) {
        (void)joint;
        neurons.push_back(MakeInputLayer(cfg, cfg.mf_per_joint));
        neurons.push_back(MakeInputLayer(cfg, cfg.cf_per_joint));
        neurons.push_back(MakeGranuleLayer(cfg, cfg.gc_per_joint));
        neurons.push_back(MakePurkinjeLayer(cfg, cfg.pc_per_joint));
        neurons.push_back(MakeDcnLayer(cfg, cfg.dcn_per_joint));
    }
}

int JointLearningRuleOrDisabled(const ReproConfig& cfg, int joint) {
    return cfg.enable_learning ? joint : -1;
}

void BuildLocalCerebellarConnections(
    const ReproConfig& cfg,
    const LayerOffsets& offsets,
    std::list<ConnectionDescription>& synapses) {
    if (cfg.rbf_channel_count != 4) {
        throw std::runtime_error("BuildLocalCerebellarConnections expects 4 MF channels.");
    }

    for (int joint = 0; joint < cfg.dof; ++joint) {
        const int mf_start = MF0(offsets, joint);
        const int cf_start = CF0(cfg, offsets, joint);
        const int gc_start = GC0(cfg, offsets, joint);
        const int pc_start = PC0(cfg, offsets, joint);
        const int dcn_start = DCN0(cfg, offsets, joint);

        {
            std::vector<int> sources;
            std::vector<int> targets;
            std::vector<int> types;
            std::vector<float> weights;
            std::vector<float> max_weights;
            std::vector<int> delays;
            std::vector<int> synapse_rules;
            std::vector<int> trigger_rules;

            int gc_neuron = gc_start;
            const int bins = cfg.rbf_bins_per_channel;
            const int c0 = 0;
            const int c1 = bins;
            const int c2 = 2 * bins;
            const int c3 = 3 * bins;
            const int gc_end = gc_start + cfg.gc_per_joint;

            for (int g1 = 0; g1 < bins; ++g1) {
                for (int g2 = c1; g2 < c1 + bins; ++g2) {
                    for (int g3 = c2; g3 < c2 + bins; ++g3) {
                        for (int g4 = c3; g4 < c3 + bins; ++g4) {
                            if (gc_neuron >= gc_end) {
                                throw std::runtime_error("MF->GC generator exceeded configured GC count.");
                            }
                            const int mf_presyn[4] = {
                                mf_start + c0 + g1,
                                mf_start + g2,
                                mf_start + g3,
                                mf_start + g4};
                            for (int k = 0; k < 4; ++k) {
                                sources.push_back(mf_presyn[k]);
                                targets.push_back(gc_neuron);
                                types.push_back(0);
                                weights.push_back(cfg.mf_grc_weight);
                                max_weights.push_back(cfg.mf_grc_weight);
                                delays.push_back(1);
                                synapse_rules.push_back(-1);
                                trigger_rules.push_back(-1);
                            }
                            ++gc_neuron;
                        }
                    }
                }
            }
            if (gc_neuron != gc_end) {
                throw std::runtime_error("MF->GC generator did not fill the configured GC count.");
            }
            AppendConnection(synapses, sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules);
        }

        {
            std::vector<int> sources;
            std::vector<int> targets;
            std::vector<int> types;
            std::vector<float> weights;
            std::vector<float> max_weights;
            std::vector<int> delays;
            std::vector<int> synapse_rules;
            std::vector<int> trigger_rules;

            for (int k = 0; k < cfg.pc_per_joint; ++k) {
                if (!cfg.enable_pc_dcn) {
                    continue;
                }
                sources.push_back(pc_start + k);
                targets.push_back(dcn_start + k);
                types.push_back(1);
                weights.push_back(cfg.pc_dcn_weight);
                max_weights.push_back(cfg.pc_dcn_weight);
                delays.push_back(1);
                synapse_rules.push_back(-1);
                trigger_rules.push_back(-1);
            }
            AppendConnection(synapses, sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules);
        }

        {
            std::vector<int> sources;
            std::vector<int> targets;
            std::vector<int> types;
            std::vector<float> weights;
            std::vector<float> max_weights;
            std::vector<int> delays;
            std::vector<int> synapse_rules;
            std::vector<int> trigger_rules;

            for (int k = 0; k < cfg.cf_per_joint; ++k) {
                sources.push_back(cf_start + k);
                targets.push_back(pc_start + k);
                types.push_back(0);
                weights.push_back(cfg.io_pc_weight);
                max_weights.push_back(cfg.io_pc_weight);
                delays.push_back(1);
                synapse_rules.push_back(-1);
                trigger_rules.push_back(JointLearningRuleOrDisabled(cfg, joint));
            }
            AppendConnection(synapses, sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules);
        }

        {
            std::vector<int> sources;
            std::vector<int> targets;
            std::vector<int> types;
            std::vector<float> weights;
            std::vector<float> max_weights;
            std::vector<int> delays;
            std::vector<int> synapse_rules;
            std::vector<int> trigger_rules;

            for (int k = 0; k < cfg.cf_per_joint; ++k) {
                if (!cfg.enable_cf_dcn) {
                    continue;
                }
                sources.push_back(cf_start + k);
                targets.push_back(dcn_start + k);
                types.push_back(0);
                weights.push_back(cfg.io_dcn_ampa_weight);
                max_weights.push_back(cfg.io_dcn_ampa_weight);
                delays.push_back(1);
                synapse_rules.push_back(-1);
                trigger_rules.push_back(-1);
            }
            AppendConnection(synapses, sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules);
        }

        {
            std::vector<int> sources;
            std::vector<int> targets;
            std::vector<int> types;
            std::vector<float> weights;
            std::vector<float> max_weights;
            std::vector<int> delays;
            std::vector<int> synapse_rules;
            std::vector<int> trigger_rules;

            for (int k = 0; k < cfg.cf_per_joint; ++k) {
                if (!cfg.enable_cf_dcn) {
                    continue;
                }
                sources.push_back(cf_start + k);
                targets.push_back(dcn_start + k);
                types.push_back(NmdaConnectionType());
                weights.push_back(cfg.io_dcn_nmda_weight);
                max_weights.push_back(cfg.io_dcn_nmda_weight);
                delays.push_back(1);
                synapse_rules.push_back(-1);
                trigger_rules.push_back(-1);
            }
            AppendConnection(synapses, sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules);
        }
    }
}

void BuildGlobalCerebellarConnections(
    const ReproConfig& cfg,
    const LayerOffsets& offsets,
    std::list<ConnectionDescription>& synapses) {
    std::vector<int> all_mf;
    std::vector<int> all_gc;
    std::vector<int> all_pc;
    std::vector<int> all_dcn;

    all_mf.reserve(offsets.total_mf);
    all_gc.reserve(offsets.total_gc);
    all_pc.reserve(offsets.total_pc);
    all_dcn.reserve(offsets.total_dcn);

    for (int joint = 0; joint < cfg.dof; ++joint) {
        for (int k = 0; k < cfg.mf_per_joint; ++k) {
            all_mf.push_back(MF0(offsets, joint) + k);
        }
        for (int k = 0; k < cfg.gc_per_joint; ++k) {
            all_gc.push_back(GC0(cfg, offsets, joint) + k);
        }
        for (int k = 0; k < cfg.pc_per_joint; ++k) {
            all_pc.push_back(PC0(cfg, offsets, joint) + k);
        }
        for (int k = 0; k < cfg.dcn_per_joint; ++k) {
            all_dcn.push_back(DCN0(cfg, offsets, joint) + k);
        }
    }

    {
        std::vector<int> sources;
        std::vector<int> targets;
        std::vector<int> types;
        std::vector<float> weights;
        std::vector<float> max_weights;
        std::vector<int> delays;
        std::vector<int> synapse_rules;
        std::vector<int> trigger_rules;

        sources.reserve(all_mf.size() * all_dcn.size());
        targets.reserve(all_mf.size() * all_dcn.size());
        for (int dcn : all_dcn) {
            for (int mf : all_mf) {
                if (!cfg.enable_mf_dcn) {
                    continue;
                }
                sources.push_back(mf);
                targets.push_back(dcn);
                types.push_back(0);
                weights.push_back(cfg.mf_dcn_weight);
                max_weights.push_back(cfg.mf_dcn_weight);
                delays.push_back(1);
                synapse_rules.push_back(-1);
                trigger_rules.push_back(-1);
            }
        }
        AppendConnection(synapses, sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules);
    }

    {
        for (int joint = 0; joint < cfg.dof; ++joint) {
            std::vector<int> sources;
            std::vector<int> targets;
            std::vector<int> types;
            std::vector<float> weights;
            std::vector<float> max_weights;
            std::vector<int> delays;
            std::vector<int> synapse_rules;
            std::vector<int> trigger_rules;

            sources.reserve(all_gc.size() * cfg.pc_per_joint);
            targets.reserve(all_gc.size() * cfg.pc_per_joint);
            const int pc_start = PC0(cfg, offsets, joint);
            for (int pc_offset = 0; pc_offset < cfg.pc_per_joint; ++pc_offset) {
                const int pc = pc_start + pc_offset;
                for (int gc : all_gc) {
                    sources.push_back(gc);
                    targets.push_back(pc);
                    types.push_back(0);
                    weights.push_back(cfg.gc_pc_initial_weight);
                    max_weights.push_back(cfg.gc_pc_max_weight);
                    delays.push_back(1);
                    synapse_rules.push_back(JointLearningRuleOrDisabled(cfg, joint));
                    trigger_rules.push_back(-1);
                }
            }
            AppendConnection(synapses, sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules);
        }
    }
}

void BuildLearningRules(const ReproConfig& cfg, std::list<LearningRuleDescription>& rules) {
    if (!cfg.enable_learning) {
        return;
    }
    for (int joint = 0; joint < cfg.dof; ++joint) {
        LearningRuleDescription cerebellar_learning;
        cerebellar_learning.RuleName = "CerebullarLearningRule";
        cerebellar_learning.RuleParameter["fixweightchange"] = cfg.learning_rule_ltp;
        cerebellar_learning.RuleParameter["kernalchange"] = cfg.learning_rule_ltd;
        cerebellar_learning.RuleParameter["maxpos"] = cfg.learning_rule_maxpos;
        cerebellar_learning.RuleParameter["initpos"] = cfg.learning_rule_initpos;
        rules.push_back(std::move(cerebellar_learning));
    }
}

void ReleaseConstructionMemory(
    std::list<NeuronLayerDescription>& neuron_layers,
    std::list<ConnectionDescription>& connections,
    std::list<LearningRuleDescription>& learning_rules) {
    std::list<NeuronLayerDescription>().swap(neuron_layers);
    std::list<ConnectionDescription>().swap(connections);
    std::list<LearningRuleDescription>().swap(learning_rules);
}

void ReleaseInputMemory(std::vector<int>& spike_times, std::vector<int>& spike_neurons) {
    std::vector<int>().swap(spike_times);
    std::vector<int>().swap(spike_neurons);
}

double ClampToRange(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(max_value, value));
}

double UniformCenter(int index, int count, double min_value, double max_value) {
    if (count <= 1) {
        return 0.5 * (min_value + max_value);
    }
    const double step = (max_value - min_value) / static_cast<double>(count - 1);
    return min_value + step * static_cast<double>(index);
}

double UniformWidth(int count, double min_value, double max_value, double overlap) {
    if (count <= 1) {
        return std::max(1e-6, (max_value - min_value) * overlap);
    }
    const double step = (max_value - min_value) / static_cast<double>(count - 1);
    return std::max(1e-6, step * overlap);
}

bool SquareRbfActive(double value, double center, double width) {
    return std::fabs((value - center) / width) < 1.0;
}

double TanhRateActivation(double value, double center, double width) {
    return (std::tanh((value - center) / width) + 1.0) * 0.5;
}

double DesiredPosition(const ReproConfig& cfg, int joint, double time_s) {
    const double omega = 2.0 * 3.14159265358979323846 * cfg.trajectory_frequency_hz;
    const double phase = 0.45 * static_cast<double>(joint);
    const double amplitude = 0.7 + 0.08 * static_cast<double>(joint);
    return amplitude * std::sin(omega * time_s + phase);
}

double DesiredVelocity(const ReproConfig& cfg, int joint, double time_s) {
    const double omega = 2.0 * 3.14159265358979323846 * cfg.trajectory_frequency_hz;
    const double phase = 0.45 * static_cast<double>(joint);
    const double amplitude = 0.7 + 0.08 * static_cast<double>(joint);
    return amplitude * omega * std::cos(omega * time_s + phase);
}

double ActualPosition(const ReproConfig& cfg, int joint, double time_s) {
    const double lagged_time = std::max(0.0, time_s - cfg.actual_lag_seconds);
    return cfg.actual_gain * DesiredPosition(cfg, joint, lagged_time) + cfg.actual_bias * std::sin(0.5 * time_s + 0.45 * static_cast<double>(joint));
}

double ActualVelocity(const ReproConfig& cfg, int joint, double time_s) {
    const double lagged_time = std::max(0.0, time_s - cfg.actual_lag_seconds);
    return cfg.actual_gain * DesiredVelocity(cfg, joint, lagged_time) + 0.5 * cfg.actual_bias * std::cos(0.5 * time_s + 0.45 * static_cast<double>(joint));
}

double QuantizedPositiveError(const ReproConfig& cfg, double error_value) {
    return ClampToRange(std::max(0.0, error_value), 0.0, cfg.cf_error_max);
}

std::vector<int> MakeNeuronRange(int begin, int count) {
    std::vector<int> neurons;
    neurons.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        neurons.push_back(begin + i);
    }
    return neurons;
}

OuterDynamicDescription BuildOuterDynamicDescription(
    const ReproConfig& cfg,
    const LayerOffsets& offsets) {
    OuterDynamicDescription dynamic_desc;
    dynamic_desc.ModelName = "PlanarArm2DOFPinocchio";
    dynamic_desc.update_timestep = 1;
    dynamic_desc.communication_interval = std::max(1, cfg.cf_burst_spacing_steps);
    dynamic_desc.queue_index = 0;

    dynamic_desc.ModelParameter["link_lengths"] = std::vector<double>{1.0, 1.0};
    dynamic_desc.ModelParameter["link_masses"] = std::vector<double>{1.0, 1.0};
    dynamic_desc.ModelParameter["joint_damping"] = std::vector<double>{0.05, 0.05};
    dynamic_desc.ModelParameter["pd_kp"] = std::vector<double>{0.0, 0.0};
    dynamic_desc.ModelParameter["pd_kd"] = std::vector<double>{0.0, 0.0};
    dynamic_desc.ModelParameter["enable_inverse_dynamics"] = false;
    dynamic_desc.ModelParameter["trajectory_frequency_hz"] = cfg.trajectory_frequency_hz;
    dynamic_desc.ModelParameter["circle_center_x"] = cfg.circle_center_x;
    dynamic_desc.ModelParameter["circle_center_y"] = cfg.circle_center_y;
    dynamic_desc.ModelParameter["circle_radius"] = cfg.circle_radius;
    dynamic_desc.ModelParameter["vertical_plane"] = true;
    dynamic_desc.ModelParameter["elbow_down"] = true;
    dynamic_desc.ModelParameter["dcn_torque_gain"] = 0.0015;
    dynamic_desc.ModelParameter["cf_max_torque"] = 1.0;
    dynamic_desc.ModelParameter["cf_error_kp"] = 0.5;
    dynamic_desc.ModelParameter["cf_error_kd"] = 0.5 / (2.0 * 3.14159265358979323846);
    dynamic_desc.ModelParameter["cf_learning_max_spike_frequency"] = 5.0;
    dynamic_desc.ModelParameter["cf_learning_base_current"] = 0.0;
    dynamic_desc.ModelParameter["cf_max_low_torque_factor"] = 0.2;
    dynamic_desc.ModelParameter["mf_min_position"] = cfg.mf_min_position;
    dynamic_desc.ModelParameter["mf_max_position"] = cfg.mf_max_position;
    dynamic_desc.ModelParameter["mf_min_velocity"] = cfg.mf_min_velocity;
    dynamic_desc.ModelParameter["mf_max_velocity"] = cfg.mf_max_velocity;
    dynamic_desc.ModelParameter["mf_max_spike_frequency"] = cfg.mf_max_spike_frequency;
    dynamic_desc.ModelParameter["mf_overlapping_factor"] = cfg.mf_overlapping_factor;
    dynamic_desc.ModelParameter["cf_error_max"] = cfg.cf_error_max;
    dynamic_desc.ModelParameter["cf_min_spike_frequency"] = cfg.cf_min_spike_frequency;
    dynamic_desc.ModelParameter["cf_max_spike_frequency"] = cfg.cf_max_spike_frequency;
    dynamic_desc.ModelParameter["cf_overlapping_factor"] = cfg.cf_overlapping_factor;
    dynamic_desc.ModelParameter["cf_burst_spacing_steps"] = cfg.cf_burst_spacing_steps;
    dynamic_desc.ModelParameter["poisson_seed"] = static_cast<int>(cfg.poisson_seed);
    dynamic_desc.ModelParameter["spike_retention_steps"] =
        std::max(256, cfg.cf_burst_spacing_steps * 64);

    const int cf_half = cfg.cf_per_joint / 2;
    const int dcn_half = cfg.dcn_per_joint / 2;
    for (int joint = 0; joint < cfg.dof; ++joint) {
        dynamic_desc.mf_neuron_indices_by_joint.push_back(
            MakeNeuronRange(MF0(offsets, joint), cfg.mf_per_joint));
        dynamic_desc.cf_positive_neuron_indices_by_joint.push_back(
            MakeNeuronRange(CF0(cfg, offsets, joint), cf_half));
        dynamic_desc.cf_negative_neuron_indices_by_joint.push_back(
            MakeNeuronRange(CF0(cfg, offsets, joint) + cf_half, cf_half));
        dynamic_desc.dcn_positive_neuron_indices_by_joint.push_back(
            MakeNeuronRange(DCN0(cfg, offsets, joint), dcn_half));
        dynamic_desc.dcn_negative_neuron_indices_by_joint.push_back(
            MakeNeuronRange(DCN0(cfg, offsets, joint) + dcn_half, dcn_half));
    }

    return dynamic_desc;
}

int FrequencyToPeriodSteps(double frequency_hz, double timestep_s) {
    if (frequency_hz <= 0.0) {
        return std::numeric_limits<int>::max();
    }
    return std::max(1, static_cast<int>(std::llround(1.0 / (frequency_hz * timestep_s))));
}

void BuildOfflineInputSpikes(
    const ReproConfig& cfg,
    const LayerOffsets& offsets,
    std::vector<int>& spike_times,
    std::vector<int>& spike_neurons) {
    spike_times.clear();
    spike_neurons.clear();

    const double timestep_s = StepSeconds(cfg);
    const int total_network_neurons = cfg.dof * offsets.joint_stride;
    std::vector<int> next_mf_spike_step(total_network_neurons, -1);
    std::mt19937 rng(cfg.poisson_seed);
    std::uniform_real_distribution<double> unit_rand(0.0, 1.0);
    std::vector<std::pair<int, int>> scheduled_spikes;
    scheduled_spikes.reserve(cfg.simulation_steps * cfg.dof * (cfg.rbf_channel_count + cfg.cf_per_joint / 6));

    for (int step = 0; step < cfg.simulation_steps; ++step) {
        const double time_s = static_cast<double>(step) * timestep_s;
        for (int joint = 0; joint < cfg.dof; ++joint) {
            const std::array<double, 4> mf_values = {
                DesiredPosition(cfg, joint, time_s),
                ActualPosition(cfg, joint, time_s),
                DesiredVelocity(cfg, joint, time_s),
                ActualVelocity(cfg, joint, time_s),
            };
            const std::array<std::pair<double, double>, 4> mf_ranges = {{
                {cfg.mf_min_position, cfg.mf_max_position},
                {cfg.mf_min_position, cfg.mf_max_position},
                {cfg.mf_min_velocity, cfg.mf_max_velocity},
                {cfg.mf_min_velocity, cfg.mf_max_velocity},
            }};

            for (int channel = 0; channel < cfg.rbf_channel_count; ++channel) {
                const double clamped_value = ClampToRange(mf_values[channel], mf_ranges[channel].first, mf_ranges[channel].second);
                const double width = UniformWidth(
                    cfg.rbf_bins_per_channel,
                    mf_ranges[channel].first,
                    mf_ranges[channel].second,
                    cfg.mf_overlapping_factor);
                const int channel_start = MF0(offsets, joint) + channel * cfg.rbf_bins_per_channel;
                for (int bin = 0; bin < cfg.rbf_bins_per_channel; ++bin) {
                    const int neuron_id = channel_start + bin;
                    const double center = UniformCenter(bin, cfg.rbf_bins_per_channel, mf_ranges[channel].first, mf_ranges[channel].second);
                    if (!SquareRbfActive(clamped_value, center, width)) {
                        continue;
                    }
                    if (next_mf_spike_step[neuron_id] < 0) {
                        next_mf_spike_step[neuron_id] = step;
                    }
                    const int period_steps = FrequencyToPeriodSteps(cfg.mf_max_spike_frequency, timestep_s);
                    while (next_mf_spike_step[neuron_id] <= step) {
                        scheduled_spikes.emplace_back(next_mf_spike_step[neuron_id], neuron_id);
                        next_mf_spike_step[neuron_id] += period_steps;
                    }
                }
            }

            const double error_value = DesiredPosition(cfg, joint, time_s) - ActualPosition(cfg, joint, time_s);
            const double positive_error = QuantizedPositiveError(cfg, error_value);
            const double negative_error = QuantizedPositiveError(cfg, -error_value);
            const int cf_half = cfg.cf_per_joint / 2;
            const double cf_width = UniformWidth(cf_half, 0.0, cfg.cf_error_max, cfg.cf_overlapping_factor);

            for (int cf = 0; cf < cf_half; ++cf) {
                const double center = UniformCenter(cf, cf_half, 0.0, cfg.cf_error_max);
                const double pos_rate_norm = TanhRateActivation(positive_error, center, cf_width);
                const double neg_rate_norm = TanhRateActivation(negative_error, center, cf_width);
                const double pos_rate_hz = cfg.cf_min_spike_frequency + pos_rate_norm * (cfg.cf_max_spike_frequency - cfg.cf_min_spike_frequency);
                const double neg_rate_hz = cfg.cf_min_spike_frequency + neg_rate_norm * (cfg.cf_max_spike_frequency - cfg.cf_min_spike_frequency);

                if (unit_rand(rng) < pos_rate_hz * timestep_s) {
                    const int neuron_id = CF0(cfg, offsets, joint) + cf;
                    const int n_spikes = std::clamp(center > 1e-6 ? static_cast<int>(positive_error / center) : 1, 1, 6);
                    for (int burst = 0; burst < n_spikes; ++burst) {
                        const int burst_step = step + burst * cfg.cf_burst_spacing_steps;
                        if (burst_step < cfg.simulation_steps) {
                            scheduled_spikes.emplace_back(burst_step, neuron_id);
                        }
                    }
                }

                if (unit_rand(rng) < neg_rate_hz * timestep_s) {
                    const int neuron_id = CF0(cfg, offsets, joint) + cf_half + cf;
                    const int n_spikes = std::clamp(center > 1e-6 ? static_cast<int>(negative_error / center) : 1, 1, 6);
                    for (int burst = 0; burst < n_spikes; ++burst) {
                        const int burst_step = step + burst * cfg.cf_burst_spacing_steps;
                        if (burst_step < cfg.simulation_steps) {
                            scheduled_spikes.emplace_back(burst_step, neuron_id);
                        }
                    }
                }
            }
        }
    }

    std::sort(scheduled_spikes.begin(), scheduled_spikes.end(), [](const auto& lhs, const auto& rhs) {
        return std::tie(lhs.first, lhs.second) < std::tie(rhs.first, rhs.second);
    });

    spike_times.reserve(scheduled_spikes.size());
    spike_neurons.reserve(scheduled_spikes.size());
    for (const auto& spike : scheduled_spikes) {
        spike_times.push_back(spike.first);
        spike_neurons.push_back(spike.second);
    }
}

void WriteEdlutInputSpikeFile(
    const ReproConfig& cfg,
    const std::vector<int>& spike_times,
    const std::vector<int>& spike_neurons,
    const std::string& file_path) {
    std::ofstream out(file_path);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open EDLUT input file for writing: " + file_path);
    }

    out << "// Generated from science_robotics_cerebellum_repro\n";
    out << static_cast<long long>(spike_times.size()) << "\n";
    out << std::fixed << std::setprecision(6);

    const double timestep_s = StepSeconds(cfg);
    for (size_t i = 0; i < spike_times.size(); ++i) {
        const double spike_time_s = static_cast<double>(spike_times[i]) * timestep_s;
        out << spike_time_s << " 1 0.0 " << spike_neurons[i] << " 1\n";
    }
}

}  // namespace

int main() {
#if !SNN_WITH_PINOCCHIO
    std::cerr << "[repro] Pinocchio support is disabled in this build." << std::endl;
    return 1;
#else
    try {
    std::cerr << "[repro] main entered" << std::endl;
    ReproConfig cfg;
    NormalizeConfigTo7MfSource(cfg);
    cfg.enable_mf_dcn = EnvFlagOrDefault("REPRO_ENABLE_MF_DCN", cfg.enable_mf_dcn);
    cfg.enable_cf_dcn = EnvFlagOrDefault("REPRO_ENABLE_CF_DCN", cfg.enable_cf_dcn);
    cfg.enable_pc_dcn = EnvFlagOrDefault("REPRO_ENABLE_PC_DCN", cfg.enable_pc_dcn);
    cfg.enable_learning = EnvFlagOrDefault("REPRO_ENABLE_LEARNING", cfg.enable_learning);
    cfg.pc_dcn_weight = EnvFloatOrDefault("REPRO_PC_DCN_WEIGHT", cfg.pc_dcn_weight);
    const int training_rounds = std::max(1, EnvIntOrDefault("REPRO_TRAINING_ROUNDS", 1));
    if (const char* requested_model = std::getenv("REPRO_NEURON_MODEL")) {
        cfg.neuron_model_profile = requested_model;
    }
    if (!cfg.enable_learning) {
        cfg.gc_pc_max_weight = cfg.gc_pc_initial_weight;
        cfg.weight_save_interval_steps = 0;
    }
    const LayerOffsets offsets = ComputeOffsets(cfg);

    std::filesystem::create_directories(OutputRoot());
    std::cerr << "[repro] output directory ready: " << OutputRoot() << std::endl;

    std::list<NeuronLayerDescription> neuron_layers;
    std::list<ConnectionDescription> connections;
    std::list<LearningRuleDescription> learning_rules;
    std::list<OuterDynamicDescription> outer_dynamics;

    std::cerr << "[repro] building neuron layers..." << std::endl;
    BuildNeuronLayers(cfg, neuron_layers);
    std::cerr << "[repro] neuron layers built: " << neuron_layers.size() << std::endl;
    std::cerr << "[repro] building local connections..." << std::endl;
    BuildLocalCerebellarConnections(cfg, offsets, connections);
    std::cerr << "[repro] local connections built: " << connections.size() << std::endl;
    std::cerr << "[repro] building global connections..." << std::endl;
    BuildGlobalCerebellarConnections(cfg, offsets, connections);
    std::cerr << "[repro] global connections built: " << connections.size() << std::endl;
    std::cerr << "[repro] building learning rules..." << std::endl;
    BuildLearningRules(cfg, learning_rules);
    std::cerr << "[repro] learning rules built: " << learning_rules.size() << std::endl;
    std::cerr << "[repro] building outer dynamics..." << std::endl;
    outer_dynamics.push_back(BuildOuterDynamicDescription(cfg, offsets));
    std::cerr << "[repro] outer dynamics built: " << outer_dynamics.size() << std::endl;

    std::cout
        << "Science Robotics cerebellum repro\n"
        << "  Backend: " << BackendSummary() << "\n"
        << "  Neuron model profile: " << cfg.neuron_model_profile << "\n"
        << "  Neuron model type: " << SelectedLifModelName(cfg) << "\n"
        << "  DOF: " << cfg.dof << "\n"
        << "  Neurons/joint [MF, CF, GC, PC, DCN]: "
        << cfg.mf_per_joint << ", "
        << cfg.cf_per_joint << ", "
        << cfg.gc_per_joint << ", "
        << cfg.pc_per_joint << ", "
        << cfg.dcn_per_joint << "\n"
        << "  Training rounds: " << training_rounds << "\n"
        << "  Simulation steps: " << cfg.simulation_steps << "\n"
        << "  Learning enabled: " << cfg.enable_learning << "\n"
        << "  DCN inputs [MF, CF, PC]: "
        << cfg.enable_mf_dcn << ", "
        << cfg.enable_cf_dcn << ", "
        << cfg.enable_pc_dcn << "\n"
        << "  PC->DCN weight: " << cfg.pc_dcn_weight << "\n"
        << "  All-layer spike output: " << cfg.debug_all_spike_output << "\n"
        << "  MF encoder: square RBF over 4 channels x " << cfg.rbf_bins_per_channel << " bins\n"
        << "  CF encoder: signed error Poisson over " << (cfg.cf_per_joint / 2) << "+" << (cfg.cf_per_joint / 2) << " fibers\n"
        << "  Outer dynamics: PlanarArm2DOFPinocchio closed loop\n";

    for (int round = 0; round < training_rounds; ++round) {
        const std::string round_dir = RoundDirectory(OutputRoot(), round);
        const std::string joint_state_path = JoinPath(round_dir, "planar_arm_joint_state.tsv");
        const std::string spike_path = JoinPath(round_dir, "spikes.txt");
        const std::string monitor_path = JoinPath(round_dir, "monitor.txt");
        const std::string final_weight_path = JoinPath(round_dir, "weights_final.txt");
        const std::string previous_weight_path =
            (round > 0) ? JoinPath(RoundDirectory(OutputRoot(), round - 1), "weights_final.txt") : "";

        std::filesystem::create_directories(round_dir);

        std::cerr << "[repro] constructing simulation for round " << round << "..." << std::endl;
        Simulation simulation(
            neuron_layers,
            connections,
            learning_rules,
            outer_dynamics,
            cfg.simulation_steps,
            StepMilliseconds(cfg),
            cfg.queue_count);
        std::cerr << "[repro] simulation constructed for round " << round << std::endl;

        std::cerr << "[repro] attaching output drivers for round " << round << "..." << std::endl;
        simulation.AddFileOutputMonitorDriver(monitor_path);
        simulation.AddFileOutputSpikeDriver(spike_path);
        simulation.AddFileOuterDynamicStateDriver(joint_state_path);
        if (cfg.weight_save_interval_steps > 0) {
            simulation.AddFileOutputWeightDriver(JoinPath(round_dir, "weights.txt"), cfg.weight_save_interval_steps);
        }
        simulation.SetPrintSimulationTime(cfg.diagnostic_print_time);
        std::cerr << "[repro] drivers attached for round " << round << std::endl;

        if (round > 0) {
            std::cout << "Loading previous round weights: " << previous_weight_path << std::endl;
            simulation.LoadWeight(previous_weight_path.c_str());
        }

        std::cout << "Starting InitSimulation for round " << round << "..." << std::endl;
        simulation.InitSimulation();
        std::cout << "InitSimulation finished. Starting RunSimulationStep for round " << round << "..." << std::endl;
        simulation.RunSimulationStep(cfg.simulation_steps);
        std::cout << "RunSimulationStep finished for round " << round << "." << std::endl;

        simulation.SaveWeightToFile(final_weight_path.c_str());
        std::cout << "Saved round weights to " << final_weight_path << std::endl;
    }

    std::cout << "Finished. Outputs written to .\\" << OutputRoot() << std::endl;
    return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[repro] exception: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[repro] unknown exception" << std::endl;
        return 1;
    }
#endif
}
