#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Config {
    int dof = 2;
    int mf_bins_angle_1 = 15;
    int mf_bins_angle_2 = 15;
    int mf_bins_velocity_1 = 10;
    int mf_bins_velocity_2 = 10;
    int gc_per_joint = 15 * 15 * 10 * 10;
    int cf_per_sign_per_joint = 200;
    int pc_per_sign_per_joint = 200;
    int dcn_per_sign_per_joint = 200;

    int simulation_steps = 10000;
    float timestep_ms = 0.1f;
    int queue_count = 1;
    int outer_update_steps = 200;
    int training_rounds = 1;

    float gc_pc_initial_weight = 11.0f;
    float gc_pc_max_weight = 12.0f;
    float gc_dcn_weight = 12.0f;
    float pc_dcn_weight = 12.0f;
    float cf_pc_weight = 0.0f;
    float learning_ltp = 0.006f;
    float learning_ltd = -0.5f;
    float learning_maxpos = 12.0f;
    float learning_initpos = 11.0f;
};

std::string OutputRoot() {
    return "total_model_repro_outputs";
}

int EnvIntOrDefault(const char* name, int default_value) {
    const char* value = std::getenv(name);
    if (value == NULL) {
        return default_value;
    }
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

int JointStride(const Config& cfg) {
    return
        cfg.gc_per_joint +
        (2 * cfg.cf_per_sign_per_joint) +
        (2 * cfg.pc_per_sign_per_joint) +
        (2 * cfg.dcn_per_sign_per_joint);
}

int GC0(const Config& cfg, int joint) {
    return joint * JointStride(cfg);
}

int CF0(const Config& cfg, int joint) {
    return GC0(cfg, joint) + cfg.gc_per_joint;
}

int PC0(const Config& cfg, int joint) {
    return CF0(cfg, joint) + 2 * cfg.cf_per_sign_per_joint;
}

int DCN0(const Config& cfg, int joint) {
    return PC0(cfg, joint) + 2 * cfg.pc_per_sign_per_joint;
}

std::vector<int> MakeRange(int begin, int count) {
    std::vector<int> values;
    values.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        values.push_back(begin + i);
    }
    return values;
}

NeuronLayerDescription MakeInputLayer(int count, bool is_output) {
    NeuronLayerDescription layer;
    layer.ModelName = "InputSpikeNeuronModel";
    layer.numberofneuron = count;
    layer.isMonitored = false;
    layer.isOutput = is_output;
    return layer;
}

NeuronLayerDescription MakeLifLayer(
    int count,
    float v_rest,
    float v_reset,
    float v_th,
    float tau,
    float gexc_tau,
    float ginh_tau,
    bool is_output) {
    NeuronLayerDescription layer;
    layer.ModelName = "TimeDrivenLIF_Exponential_double";
    layer.numberofneuron = count;
    layer.isMonitored = true;
    layer.isOutput = is_output;
    layer.NeuronParameter["V_rest"] = v_rest;
    layer.NeuronParameter["V_reset"] = v_reset;
    layer.NeuronParameter["V_th"] = v_th;
    layer.NeuronParameter["tau"] = tau;
    layer.NeuronParameter["R"] = 1.0f;
    layer.NeuronParameter["t_ref"] = 10;
    layer.NeuronParameter["gexc_tau"] = gexc_tau;
    layer.NeuronParameter["ginh_tau"] = ginh_tau;
    layer.NeuronParameter["Eexc"] = 0.0f;
    layer.NeuronParameter["Einh"] = -80.0f;
    layer.NeuronParameter["random_sigma"] = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f };
    return layer;
}

void FinalizeConnection(ConnectionDescription& desc, std::list<ConnectionDescription>& connections) {
    desc.SynapseModel.resize(desc.SourceNeuron.size());
    connections.push_back(std::move(desc));
}

void AddDenseProjection(
    std::list<ConnectionDescription>& connections,
    const std::vector<int>& sources,
    const std::vector<int>& targets,
    int type,
    float weight,
    float max_weight,
    int synapse_rule,
    int trigger_rule) {
    ConnectionDescription desc;
    const std::size_t total = sources.size() * targets.size();
    desc.SourceNeuron.reserve(total);
    desc.TargetNeuron.reserve(total);
    desc.Type.reserve(total);
    desc.Weight.reserve(total);
    desc.MaxWeight.reserve(total);
    desc.Delay.reserve(total);
    desc.SynapseRule.reserve(total);
    desc.TriggerRule.reserve(total);

    for (std::size_t target_index = 0; target_index < targets.size(); ++target_index) {
        for (std::size_t source_index = 0; source_index < sources.size(); ++source_index) {
            desc.SourceNeuron.push_back(sources[source_index]);
            desc.TargetNeuron.push_back(targets[target_index]);
            desc.Type.push_back(type);
            desc.Weight.push_back(weight);
            desc.MaxWeight.push_back(max_weight);
            desc.Delay.push_back(1);
            desc.SynapseRule.push_back(synapse_rule);
            desc.TriggerRule.push_back(trigger_rule);
        }
    }
    FinalizeConnection(desc, connections);
}

void BuildNeuronLayers(const Config& cfg, std::list<NeuronLayerDescription>& neurons) {
    for (int joint = 0; joint < cfg.dof; ++joint) {
        (void)joint;
        neurons.push_back(MakeInputLayer(cfg.gc_per_joint, false));
        neurons.push_back(MakeInputLayer(2 * cfg.cf_per_sign_per_joint, false));
        neurons.push_back(MakeLifLayer(2 * cfg.pc_per_sign_per_joint, -80.0f, -80.0f, -40.0f, 10.0f / 6.0f, 1.0f, 1.0f, false));
        neurons.push_back(MakeLifLayer(2 * cfg.dcn_per_sign_per_joint, -80.0f, -80.0f, -40.0f, 10.0f / 6.0f, 1.0f, 1.0f, true));
    }
}

void BuildConnections(const Config& cfg, std::list<ConnectionDescription>& connections) {
    for (int joint = 0; joint < cfg.dof; ++joint) {
        const std::vector<int> gc = MakeRange(GC0(cfg, joint), cfg.gc_per_joint);
        const std::vector<int> cf_pos = MakeRange(CF0(cfg, joint), cfg.cf_per_sign_per_joint);
        const std::vector<int> cf_neg = MakeRange(CF0(cfg, joint) + cfg.cf_per_sign_per_joint, cfg.cf_per_sign_per_joint);
        const std::vector<int> pc_pos = MakeRange(PC0(cfg, joint), cfg.pc_per_sign_per_joint);
        const std::vector<int> pc_neg = MakeRange(PC0(cfg, joint) + cfg.pc_per_sign_per_joint, cfg.pc_per_sign_per_joint);
        const std::vector<int> dcn_pos = MakeRange(DCN0(cfg, joint), cfg.dcn_per_sign_per_joint);
        const std::vector<int> dcn_neg = MakeRange(DCN0(cfg, joint) + cfg.dcn_per_sign_per_joint, cfg.dcn_per_sign_per_joint);

        AddDenseProjection(connections, gc, pc_pos, 0, cfg.gc_pc_initial_weight, cfg.gc_pc_max_weight, joint, -1);
        AddDenseProjection(connections, gc, pc_neg, 0, cfg.gc_pc_initial_weight, cfg.gc_pc_max_weight, joint, -1);
        AddDenseProjection(connections, gc, dcn_pos, 0, cfg.gc_dcn_weight, cfg.gc_dcn_weight, -1, -1);
        AddDenseProjection(connections, gc, dcn_neg, 0, cfg.gc_dcn_weight, cfg.gc_dcn_weight, -1, -1);

        {
            ConnectionDescription desc;
            for (int i = 0; i < cfg.pc_per_sign_per_joint; ++i) {
                desc.SourceNeuron.push_back(pc_pos[i]);
                desc.TargetNeuron.push_back(dcn_pos[i]);
                desc.Type.push_back(1);
                desc.Weight.push_back(cfg.pc_dcn_weight);
                desc.MaxWeight.push_back(cfg.pc_dcn_weight);
                desc.Delay.push_back(1);
                desc.SynapseRule.push_back(-1);
                desc.TriggerRule.push_back(-1);
            }
            for (int i = 0; i < cfg.pc_per_sign_per_joint; ++i) {
                desc.SourceNeuron.push_back(pc_neg[i]);
                desc.TargetNeuron.push_back(dcn_neg[i]);
                desc.Type.push_back(1);
                desc.Weight.push_back(cfg.pc_dcn_weight);
                desc.MaxWeight.push_back(cfg.pc_dcn_weight);
                desc.Delay.push_back(1);
                desc.SynapseRule.push_back(-1);
                desc.TriggerRule.push_back(-1);
            }
            FinalizeConnection(desc, connections);
        }

        {
            ConnectionDescription desc;
            for (int i = 0; i < cfg.cf_per_sign_per_joint; ++i) {
                desc.SourceNeuron.push_back(cf_pos[i]);
                desc.TargetNeuron.push_back(pc_pos[i]);
                desc.Type.push_back(0);
                desc.Weight.push_back(cfg.cf_pc_weight);
                desc.MaxWeight.push_back(cfg.cf_pc_weight);
                desc.Delay.push_back(1);
                desc.SynapseRule.push_back(-1);
                desc.TriggerRule.push_back(joint);
            }
            for (int i = 0; i < cfg.cf_per_sign_per_joint; ++i) {
                desc.SourceNeuron.push_back(cf_neg[i]);
                desc.TargetNeuron.push_back(pc_neg[i]);
                desc.Type.push_back(0);
                desc.Weight.push_back(cfg.cf_pc_weight);
                desc.MaxWeight.push_back(cfg.cf_pc_weight);
                desc.Delay.push_back(1);
                desc.SynapseRule.push_back(-1);
                desc.TriggerRule.push_back(joint);
            }
            FinalizeConnection(desc, connections);
        }
    }
}

void BuildLearningRules(const Config& cfg, std::list<LearningRuleDescription>& rules) {
    for (int joint = 0; joint < cfg.dof; ++joint) {
        (void)joint;
        LearningRuleDescription learning;
        learning.RuleName = "CerebullarLearningRule";
        learning.RuleParameter["fixweightchange"] = cfg.learning_ltp;
        learning.RuleParameter["kernalchange"] = cfg.learning_ltd;
        learning.RuleParameter["maxpos"] = cfg.learning_maxpos;
        learning.RuleParameter["initpos"] = cfg.learning_initpos;
        rules.push_back(std::move(learning));
    }
}

void ReleaseConstructionMemory(
    std::list<NeuronLayerDescription>& neurons,
    std::list<ConnectionDescription>& connections,
    std::list<LearningRuleDescription>& learning_rules,
    std::list<OuterDynamicDescription>& outer_dynamics) {
    std::list<NeuronLayerDescription>().swap(neurons);
    std::list<ConnectionDescription>().swap(connections);
    std::list<LearningRuleDescription>().swap(learning_rules);
    std::list<OuterDynamicDescription>().swap(outer_dynamics);
}

OuterDynamicDescription BuildOuterDynamicDescription(const Config& cfg) {
    OuterDynamicDescription desc;
    desc.ModelName = "TotalModelPlanarArm2DOF";
    desc.update_timestep = cfg.outer_update_steps;
    desc.communication_interval = cfg.outer_update_steps;
    desc.queue_index = 0;
    desc.ModelParameter["link_lengths"] = std::vector<double>{ 0.34, 0.32 };
    desc.ModelParameter["link_masses"] = std::vector<double>{ 1.8, 1.6 };
    desc.ModelParameter["torque_scale"] = std::vector<double>{ 0.2, 0.05 };
    desc.ModelParameter["trajectory_center_x"] = 0.1;
    desc.ModelParameter["trajectory_center_y"] = 0.4;
    desc.ModelParameter["trajectory_radius"] = 0.15;
    desc.ModelParameter["desired_period_s"] = 1.0;
    desc.ModelParameter["cf_mix_position_weight"] = 0.8;
    desc.ModelParameter["cf_angle_norm_deg"] = 60.0;
    desc.ModelParameter["cf_elbow_norm_deg"] = 75.0;
    desc.ModelParameter["cf_velocity_norm_deg_s"] = 400.0;
    desc.ModelParameter["cf_spike_scale"] = 60.0;
    desc.ModelParameter["mf_channel_bins"] = std::vector<int>{ cfg.mf_bins_angle_1, cfg.mf_bins_angle_2, cfg.mf_bins_velocity_1, cfg.mf_bins_velocity_2 };
    desc.ModelParameter["spike_retention_steps"] = std::max(512, cfg.outer_update_steps * 4);
    desc.ModelParameter["poisson_seed"] = 7;

    for (int joint = 0; joint < cfg.dof; ++joint) {
        desc.gc_neuron_indices_by_joint.push_back(MakeRange(GC0(cfg, joint), cfg.gc_per_joint));
        desc.cf_positive_neuron_indices_by_joint.push_back(MakeRange(CF0(cfg, joint), cfg.cf_per_sign_per_joint));
        desc.cf_negative_neuron_indices_by_joint.push_back(MakeRange(CF0(cfg, joint) + cfg.cf_per_sign_per_joint, cfg.cf_per_sign_per_joint));
        desc.dcn_positive_neuron_indices_by_joint.push_back(MakeRange(DCN0(cfg, joint), cfg.dcn_per_sign_per_joint));
        desc.dcn_negative_neuron_indices_by_joint.push_back(MakeRange(DCN0(cfg, joint) + cfg.dcn_per_sign_per_joint, cfg.dcn_per_sign_per_joint));
    }

    return desc;
}

std::string RoundDirectory(const std::string& base, int round) {
    return (std::filesystem::path(base) / ("round_" + std::to_string(round))).string();
}

} // namespace

int main() {
    try {
        Config cfg;
        cfg.training_rounds = std::max(1, EnvIntOrDefault("TOTAL_MODEL_TRAINING_ROUNDS", cfg.training_rounds));
        cfg.simulation_steps = std::max(1, EnvIntOrDefault("TOTAL_MODEL_STEPS", cfg.simulation_steps));

        std::filesystem::create_directories(OutputRoot());

        std::cout
            << "total_model cerebellum repro\n"
            << "  outputs: " << OutputRoot() << "\n"
            << "  steps: " << cfg.simulation_steps << "\n"
            << "  timestep ms: " << cfg.timestep_ms << "\n"
            << "  training rounds: " << cfg.training_rounds << "\n"
            << "  GC/joint: " << cfg.gc_per_joint << "\n"
            << "  PC sign channels/joint: " << cfg.pc_per_sign_per_joint << "+" << cfg.pc_per_sign_per_joint << "\n"
            << "  DCN sign channels/joint: " << cfg.dcn_per_sign_per_joint << "+" << cfg.dcn_per_sign_per_joint << std::endl;

        for (int round = 0; round < cfg.training_rounds; ++round) {
            const std::string round_dir = RoundDirectory(OutputRoot(), round);
            const std::string monitor_path = (std::filesystem::path(round_dir) / "monitor.txt").string();
            const std::string spike_path = (std::filesystem::path(round_dir) / "spikes.txt").string();
            const std::string state_path = (std::filesystem::path(round_dir) / "joint_state.tsv").string();
            const std::string final_weight_path = (std::filesystem::path(round_dir) / "weights_final.txt").string();
            const std::string prev_weight_path =
                round > 0 ? (std::filesystem::path(RoundDirectory(OutputRoot(), round - 1)) / "weights_final.txt").string() : "";
            std::filesystem::create_directories(round_dir);

            std::list<NeuronLayerDescription> neurons;
            std::list<ConnectionDescription> connections;
            std::list<LearningRuleDescription> learning_rules;
            std::list<OuterDynamicDescription> outer_dynamics;

            BuildNeuronLayers(cfg, neurons);
            BuildConnections(cfg, connections);
            BuildLearningRules(cfg, learning_rules);
            outer_dynamics.push_back(BuildOuterDynamicDescription(cfg));

            Simulation simulation(
                neurons,
                connections,
                learning_rules,
                outer_dynamics,
                cfg.simulation_steps,
                cfg.timestep_ms,
                cfg.queue_count);

            ReleaseConstructionMemory(neurons, connections, learning_rules, outer_dynamics);

            simulation.AddFileOutputMonitorDriver(monitor_path);
            simulation.AddFileOutputSpikeDriver(spike_path);
            simulation.AddFileOuterDynamicStateDriver(state_path);
            if (round > 0) {
                simulation.LoadWeight(prev_weight_path.c_str());
            }
            simulation.InitSimulation();
            simulation.RunSimulationStep(cfg.simulation_steps);
            simulation.SaveWeightToFile(final_weight_path.c_str());
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "total_model_cerebellum_repro exception: " << ex.what() << std::endl;
        return 1;
    }
}
