#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace {

constexpr int kDof = 2;
constexpr int kMfPerJoint = 28;
constexpr int kCfPerSignPerJoint = 20;
constexpr int kDcnPerSignPerJoint = 8;
constexpr float kBaseTimestepMs = 0.5f;
constexpr int kSimulationSteps = 6000;
constexpr int kQueues = 1;

std::string OutputRoot() {
    return "science_robotics_outputs/outer_dynamics_validation";
}

NeuronLayerDescription MakeInputLayer(int count) {
    NeuronLayerDescription layer;
    layer.ModelName = "InputSpikeNeuronModel";
    layer.numberofneuron = count;
    layer.isMonitored = false;
    layer.isOutput = false;
    return layer;
}

NeuronLayerDescription MakeDcnLayer(int count) {
    NeuronLayerDescription layer;
    layer.ModelName = "TimeDrivenLIF_Exponential_double";
    layer.numberofneuron = count;
    layer.isMonitored = true;
    layer.isOutput = true;
    layer.NeuronParameter["V_rest"] = -70.0f;
    layer.NeuronParameter["V_reset"] = -70.0f;
    layer.NeuronParameter["V_th"] = -45.0f;
    layer.NeuronParameter["tau"] = 8.0f;
    layer.NeuronParameter["R"] = 1.0f;
    layer.NeuronParameter["t_ref"] = 4;
    layer.NeuronParameter["gexc_tau"] = 2.0f;
    layer.NeuronParameter["ginh_tau"] = 6.0f;
    layer.NeuronParameter["Eexc"] = 0.0f;
    layer.NeuronParameter["Einh"] = -80.0f;
    layer.NeuronParameter["random_sigma"] = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
    return layer;
}

void AppendDenseConnections(
    std::list<ConnectionDescription>& connections,
    const std::vector<int>& sources,
    const std::vector<int>& targets,
    float weight,
    int delay) {
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
    desc.SynapseModel.reserve(total);

    for (std::size_t si = 0; si < sources.size(); ++si) {
        for (std::size_t ti = 0; ti < targets.size(); ++ti) {
            desc.SourceNeuron.push_back(sources[si]);
            desc.TargetNeuron.push_back(targets[ti]);
            desc.Type.push_back(0);
            desc.Weight.push_back(weight);
            desc.MaxWeight.push_back(weight);
            desc.Delay.push_back(delay);
            desc.SynapseRule.push_back(-1);
            desc.TriggerRule.push_back(-1);
            desc.SynapseModel.push_back(ModelDescription{});
        }
    }
    connections.push_back(std::move(desc));
}

void BuildValidationNetwork(
    std::list<NeuronLayerDescription>& neurons,
    std::list<ConnectionDescription>& connections,
    std::list<OuterDynamicDescription>& outer_dynamics) {
    int next_neuron = 0;
    OuterDynamicDescription dynamic_desc;
    dynamic_desc.ModelName = "PlanarArm2DOFPinocchio";
    dynamic_desc.update_timestep = 1;
    dynamic_desc.communication_interval = 10;
    dynamic_desc.queue_index = 0;
    dynamic_desc.ModelParameter["link_lengths"] = std::vector<double>{0.40, 0.30};
    dynamic_desc.ModelParameter["link_masses"] = std::vector<double>{1.20, 0.80};
    dynamic_desc.ModelParameter["link_inertias"] = std::vector<double>{0.03, 0.02};
    dynamic_desc.ModelParameter["joint_damping"] = std::vector<double>{0.05, 0.04};
    dynamic_desc.ModelParameter["pd_kp"] = std::vector<double>{14.0, 11.0};
    dynamic_desc.ModelParameter["pd_kd"] = std::vector<double>{2.5, 2.0};
    dynamic_desc.ModelParameter["trajectory_amplitudes"] = std::vector<double>{0.70, 0.78};
    dynamic_desc.ModelParameter["trajectory_phase_offsets"] = std::vector<double>{0.0, 0.45};
    dynamic_desc.ModelParameter["trajectory_frequency_hz"] = 0.35;
    dynamic_desc.ModelParameter["dcn_torque_gain"] = 0.0015;
    dynamic_desc.ModelParameter["cf_error_max"] = 0.6;
    dynamic_desc.ModelParameter["cf_max_spike_frequency"] = 18.0;
    dynamic_desc.ModelParameter["poisson_seed"] = 7;
    dynamic_desc.ModelParameter["spike_retention_steps"] = 512;

    for (int joint = 0; joint < kDof; ++joint) {
        std::vector<int> mf;
        std::vector<int> cf_pos;
        std::vector<int> cf_neg;
        std::vector<int> dcn_pos;
        std::vector<int> dcn_neg;

        neurons.push_back(MakeInputLayer(kMfPerJoint));
        for (int i = 0; i < kMfPerJoint; ++i) {
            mf.push_back(next_neuron++);
        }

        neurons.push_back(MakeInputLayer(kCfPerSignPerJoint));
        for (int i = 0; i < kCfPerSignPerJoint; ++i) {
            cf_pos.push_back(next_neuron++);
        }

        neurons.push_back(MakeInputLayer(kCfPerSignPerJoint));
        for (int i = 0; i < kCfPerSignPerJoint; ++i) {
            cf_neg.push_back(next_neuron++);
        }

        neurons.push_back(MakeDcnLayer(kDcnPerSignPerJoint));
        for (int i = 0; i < kDcnPerSignPerJoint; ++i) {
            dcn_pos.push_back(next_neuron++);
        }

        neurons.push_back(MakeDcnLayer(kDcnPerSignPerJoint));
        for (int i = 0; i < kDcnPerSignPerJoint; ++i) {
            dcn_neg.push_back(next_neuron++);
        }

        dynamic_desc.mf_neuron_indices_by_joint.push_back(mf);
        dynamic_desc.cf_positive_neuron_indices_by_joint.push_back(cf_pos);
        dynamic_desc.cf_negative_neuron_indices_by_joint.push_back(cf_neg);
        dynamic_desc.dcn_positive_neuron_indices_by_joint.push_back(dcn_pos);
        dynamic_desc.dcn_negative_neuron_indices_by_joint.push_back(dcn_neg);

        AppendDenseConnections(connections, cf_pos, dcn_pos, 0.35f, 1);
        AppendDenseConnections(connections, cf_neg, dcn_neg, 0.35f, 1);

        const std::vector<int> desired_mf(mf.begin(), mf.begin() + kMfPerJoint / 2);
        const std::vector<int> actual_mf(mf.begin() + kMfPerJoint / 2, mf.end());
        AppendDenseConnections(connections, desired_mf, dcn_pos, 0.05f, 1);
        AppendDenseConnections(connections, actual_mf, dcn_neg, 0.05f, 1);
    }

    outer_dynamics.push_back(std::move(dynamic_desc));
}

} // namespace

int main() {
#if !SNN_WITH_PINOCCHIO
    std::cerr << "Pinocchio support is disabled in this build." << std::endl;
    return 1;
#else
    try {
        std::filesystem::create_directories(OutputRoot());

        std::list<NeuronLayerDescription> neurons;
        std::list<ConnectionDescription> connections;
        std::list<LearningRuleDescription> learning_rules;
        std::list<OuterDynamicDescription> outer_dynamics;
        BuildValidationNetwork(neurons, connections, outer_dynamics);

        Simulation simulation(
            neurons,
            connections,
            learning_rules,
            outer_dynamics,
            kSimulationSteps,
            kBaseTimestepMs,
            kQueues);

        simulation.AddFileOutputMonitorDriver(OutputRoot() + "/monitor.txt");
        simulation.AddFileOutputSpikeDriver(OutputRoot() + "/spikes.txt");
        simulation.InitSimulation();
        simulation.RunSimulationStep(kSimulationSteps);

        std::cout
            << "outer_dynamics_validation finished\n"
            << "  outputs: " << OutputRoot() << "\n"
            << "  steps: " << kSimulationSteps << "\n"
            << "  base timestep ms: " << kBaseTimestepMs << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "outer_dynamics_validation exception: " << ex.what() << std::endl;
        return 1;
    }
#endif
}
