#include "../source_file_realtime_v1_async/OuterDynamic/inc/TotalModelPlanarArm2DOF.h"

#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicSpikeBuffer.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"

#include <boost/any.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {

template <typename T>
T GetAnyOrDefault(const std::map<std::string, boost::any>& params, const std::string& key, const T& default_value) {
    std::map<std::string, boost::any>::const_iterator it = params.find(key);
    if (it == params.end()) {
        return default_value;
    }
    try {
        return boost::any_cast<T>(it->second);
    } catch (const boost::bad_any_cast&) {
        return default_value;
    }
}

std::array<double, 2> GetVector2OrDefault(
    const std::map<std::string, boost::any>& params,
    const std::string& key,
    const std::array<double, 2>& default_value) {
    std::map<std::string, boost::any>::const_iterator it = params.find(key);
    if (it == params.end()) {
        return default_value;
    }
    try {
        const std::vector<double> values = boost::any_cast<std::vector<double> >(it->second);
        if (values.size() != 2) {
            return default_value;
        }
        return { values[0], values[1] };
    } catch (const boost::bad_any_cast&) {
        return default_value;
    }
}

std::array<int, 4> GetVector4IntOrDefault(
    const std::map<std::string, boost::any>& params,
    const std::string& key,
    const std::array<int, 4>& default_value) {
    std::map<std::string, boost::any>::const_iterator it = params.find(key);
    if (it == params.end()) {
        return default_value;
    }
    try {
        const std::vector<int> values = boost::any_cast<std::vector<int> >(it->second);
        if (values.size() != 4) {
            return default_value;
        }
        return { values[0], values[1], values[2], values[3] };
    } catch (const boost::bad_any_cast&) {
        return default_value;
    }
}

constexpr double kPi = 3.14159265358979323846;

} // namespace

TotalModelPlanarArm2DOF::TotalModelPlanarArm2DOF(int timestep_size)
    : OuterDynamicModel(timestep_size),
      joint_mappings_(),
      mf_channel_bins_{ 15, 15, 10, 10 },
      link_lengths_{ 0.34, 0.32 },
      link_masses_{ 1.8, 1.6 },
      q_{ 0.0, 0.0 },
      qd_{ 0.0, 0.0 },
      qdd_{ 0.0, 0.0 },
      q_des_{ 0.0, 0.0 },
      qd_des_{ 0.0, 0.0 },
      tau_total_{ 0.0, 0.0 },
      angle_min_deg_{ -30.0, 0.0 },
      angle_max_deg_{ 90.0, 150.0 },
      angle_range_deg_{ 120.0, 150.0 },
      velocity_min_deg_{ -400.0, -400.0 },
      velocity_max_deg_{ 400.0, 400.0 },
      torque_scale_{ 0.2, 0.05 },
      trajectory_center_x_(0.1),
      trajectory_center_y_(0.4),
      trajectory_radius_(0.15),
      desired_period_s_(1.0),
      cf_mix_position_weight_(0.8),
      cf_angle_norm_deg_(60.0),
      cf_elbow_norm_deg_(75.0),
      cf_velocity_norm_deg_s_(400.0),
      cf_spike_scale_(60.0),
      spike_retention_steps_(512),
      rng_(7) {}

TotalModelPlanarArm2DOF::~TotalModelPlanarArm2DOF() {}

void TotalModelPlanarArm2DOF::Initialize(const OuterDynamicDescription& description, Simulation* simulation) {
    this->setTimestepSize(std::max(1, description.update_timestep));
    this->setCommunicationInterval(std::max(1, description.communication_interval));
    this->setQueueIndex(description.queue_index);

    this->link_lengths_ = GetVector2OrDefault(description.ModelParameter, "link_lengths", this->link_lengths_);
    this->link_masses_ = GetVector2OrDefault(description.ModelParameter, "link_masses", this->link_masses_);
    this->torque_scale_ = GetVector2OrDefault(description.ModelParameter, "torque_scale", this->torque_scale_);
    this->trajectory_center_x_ = GetAnyOrDefault<double>(description.ModelParameter, "trajectory_center_x", this->trajectory_center_x_);
    this->trajectory_center_y_ = GetAnyOrDefault<double>(description.ModelParameter, "trajectory_center_y", this->trajectory_center_y_);
    this->trajectory_radius_ = GetAnyOrDefault<double>(description.ModelParameter, "trajectory_radius", this->trajectory_radius_);
    this->desired_period_s_ = GetAnyOrDefault<double>(description.ModelParameter, "desired_period_s", this->desired_period_s_);
    this->cf_mix_position_weight_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_mix_position_weight", this->cf_mix_position_weight_);
    this->cf_angle_norm_deg_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_angle_norm_deg", this->cf_angle_norm_deg_);
    this->cf_elbow_norm_deg_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_elbow_norm_deg", this->cf_elbow_norm_deg_);
    this->cf_velocity_norm_deg_s_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_velocity_norm_deg_s", this->cf_velocity_norm_deg_s_);
    this->cf_spike_scale_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_spike_scale", this->cf_spike_scale_);
    this->spike_retention_steps_ = GetAnyOrDefault<int>(description.ModelParameter, "spike_retention_steps", this->spike_retention_steps_);
    this->mf_channel_bins_ = GetVector4IntOrDefault(description.ModelParameter, "mf_channel_bins", this->mf_channel_bins_);
    this->rng_.seed(static_cast<std::mt19937::result_type>(GetAnyOrDefault<int>(description.ModelParameter, "poisson_seed", 7)));

    const std::size_t dof = description.gc_neuron_indices_by_joint.size();
    if (dof != 2 ||
        description.cf_positive_neuron_indices_by_joint.size() != dof ||
        description.cf_negative_neuron_indices_by_joint.size() != dof ||
        description.dcn_positive_neuron_indices_by_joint.size() != dof ||
        description.dcn_negative_neuron_indices_by_joint.size() != dof) {
        throw std::runtime_error("TotalModelPlanarArm2DOF expects explicit 2DOF GC/CF/DCN mappings.");
    }

    this->joint_mappings_.resize(dof);
    std::vector<int> watched_neurons;
    for (std::size_t joint = 0; joint < dof; ++joint) {
        JointMapping& mapping = this->joint_mappings_[joint];
        mapping.gc_neurons = description.gc_neuron_indices_by_joint[joint];
        mapping.cf_positive_neurons = description.cf_positive_neuron_indices_by_joint[joint];
        mapping.cf_negative_neurons = description.cf_negative_neuron_indices_by_joint[joint];
        mapping.dcn_positive_neurons = description.dcn_positive_neuron_indices_by_joint[joint];
        mapping.dcn_negative_neurons = description.dcn_negative_neuron_indices_by_joint[joint];

        const int expected_gc =
            this->mf_channel_bins_[0] *
            this->mf_channel_bins_[1] *
            this->mf_channel_bins_[2] *
            this->mf_channel_bins_[3];
        if (static_cast<int>(mapping.gc_neurons.size()) != expected_gc) {
            throw std::runtime_error("TotalModelPlanarArm2DOF GC mapping size does not match GC lattice dimensions.");
        }

        watched_neurons.insert(watched_neurons.end(), mapping.dcn_positive_neurons.begin(), mapping.dcn_positive_neurons.end());
        watched_neurons.insert(watched_neurons.end(), mapping.dcn_negative_neurons.begin(), mapping.dcn_negative_neurons.end());
    }

    if (simulation->outer_dynamic_spike_buffer != NULL) {
        simulation->outer_dynamic_spike_buffer->RegisterWatchedNeurons(watched_neurons);
    }

    this->UpdateDesiredState(0.0);
    this->q_ = this->q_des_;
    this->qd_ = this->qd_des_;
}

void TotalModelPlanarArm2DOF::Update(int time, Simulation* simulation) {
    const double interval_s =
        static_cast<double>(simulation->basetimesteps) * 0.001 * static_cast<double>(this->getTimestepSize());
    const double time_s =
        static_cast<double>(time) * static_cast<double>(simulation->basetimesteps) * 0.001;

    this->UpdateDesiredState(time_s);
    this->DecodeDcnTorqueWindow(time, simulation);
    this->IntegratePlant(interval_s);

    OuterDynamicJointState state;
    state.q = { this->q_[0], this->q_[1] };
    state.qv = { this->qd_[0], this->qd_[1] };
    state.q_des = { this->q_des_[0], this->q_des_[1] };
    state.qv_des = { this->qd_des_[0], this->qd_des_[1] };
    state.tau_total = { this->tau_total_[0], this->tau_total_[1] };
    simulation->WriteOuterDynamicState(time, state);

    this->EmitGranuleCodeSpikes(time, simulation);
    this->EmitClimbingFiberSpikes(time, simulation);

    if (simulation->outer_dynamic_spike_buffer != NULL) {
        simulation->outer_dynamic_spike_buffer->DiscardOlderThan(std::max(0, time - this->spike_retention_steps_));
    }
}

void TotalModelPlanarArm2DOF::UpdateDesiredState(double time_s) {
    const double phase = std::fmod(time_s, this->desired_period_s_) / this->desired_period_s_;
    const std::array<double, 2> xy = this->HandTrajectory(phase);
    const std::array<double, 2> xy_dot = this->HandVelocity(phase);
    this->q_des_ = this->InverseKinematics(xy[0], xy[1]);
    this->qd_des_ = this->InverseVelocity(this->q_des_, xy_dot[0], xy_dot[1]);
}

void TotalModelPlanarArm2DOF::DecodeDcnTorqueWindow(int time, Simulation* simulation) {
    this->tau_total_[0] = 0.0;
    this->tau_total_[1] = 0.0;
    if (simulation->outer_dynamic_spike_buffer == NULL) {
        return;
    }

    const int window_steps = std::max(1, this->getCommunicationInterval());
    const int window_start = std::max(0, time - window_steps);
    for (int joint = 0; joint < 2; ++joint) {
        const JointMapping& mapping = this->joint_mappings_[joint];
        const int positive_count =
            simulation->outer_dynamic_spike_buffer->CountSpikesInWindow(window_start, time, mapping.dcn_positive_neurons);
        const int negative_count =
            simulation->outer_dynamic_spike_buffer->CountSpikesInWindow(window_start, time, mapping.dcn_negative_neurons);
        this->tau_total_[joint] =
            this->torque_scale_[joint] * static_cast<double>(positive_count - negative_count);
    }
}

void TotalModelPlanarArm2DOF::IntegratePlant(double interval_s) {
    this->qdd_ = this->JointAcceleration(this->tau_total_);
    const std::array<double, 2> old_qd = this->qd_;
    this->qd_[0] += this->qdd_[0] * interval_s;
    this->qd_[1] += this->qdd_[1] * interval_s;
    this->q_[0] += old_qd[0] * interval_s;
    this->q_[1] += old_qd[1] * interval_s;

    this->q_[0] = Clamp(this->q_[0], -kPi / 6.0, kPi / 2.0);
    this->q_[1] = Clamp(this->q_[1], 0.1, 5.0 * kPi / 6.0);
}

void TotalModelPlanarArm2DOF::EmitGranuleCodeSpikes(int time, Simulation* simulation) {
    const int inject_time = time + this->getTimestepSize();
    for (int joint = 0; joint < 2; ++joint) {
        const JointMapping& mapping = this->joint_mappings_[joint];
        const std::array<double, 4> channel_values = {
            RadToDeg(this->q_des_[joint]),
            RadToDeg(this->q_[joint]),
            RadToDeg(this->qd_des_[joint]),
            RadToDeg(this->qd_[joint])
        };
        const std::array<double, 4> min_values = {
            this->angle_min_deg_[joint],
            this->angle_min_deg_[joint],
            this->velocity_min_deg_[joint],
            this->velocity_min_deg_[joint]
        };
        const std::array<double, 4> ranges = {
            this->angle_range_deg_[joint],
            this->angle_range_deg_[joint],
            this->velocity_max_deg_[joint] - this->velocity_min_deg_[joint],
            this->velocity_max_deg_[joint] - this->velocity_min_deg_[joint]
        };

        std::array<int, 4> indices = { 0, 0, 0, 0 };
        for (int channel = 0; channel < 4; ++channel) {
            const int bins = this->mf_channel_bins_[channel];
            const double step = ranges[channel] / static_cast<double>(bins);
            int index = static_cast<int>(std::ceil((channel_values[channel] - min_values[channel]) / step));
            index = std::max(1, std::min(bins, index));
            indices[channel] = index - 1;
        }

        const int gc_index =
            ((indices[0] * this->mf_channel_bins_[1] + indices[1]) * this->mf_channel_bins_[2] + indices[2]) *
                this->mf_channel_bins_[3] +
            indices[3];
        const int neuron_id = mapping.gc_neurons[gc_index];
        this->EmitSpike(simulation, neuron_id, inject_time);
    }
}

void TotalModelPlanarArm2DOF::EmitClimbingFiberSpikes(int time, Simulation* simulation) {
    const int window_steps = std::max(1, this->getCommunicationInterval());
    std::uniform_int_distribution<int> time_offset_dist(0, window_steps - 1);

    for (int joint = 0; joint < 2; ++joint) {
        const double desired_angle_deg = RadToDeg(this->q_des_[joint]);
        const double actual_angle_deg = RadToDeg(this->q_[joint]);
        const double desired_velocity_deg = RadToDeg(this->qd_des_[joint]);
        const double actual_velocity_deg = RadToDeg(this->qd_[joint]);
        const double angle_error = desired_angle_deg - actual_angle_deg;
        const double velocity_error = desired_velocity_deg - actual_velocity_deg;

        const double angle_norm = joint == 0 ? this->cf_angle_norm_deg_ : this->cf_elbow_norm_deg_;
        const double normalized_angle = Clamp(std::fabs(angle_error) / angle_norm, 0.0, 1.0);
        const double normalized_velocity = Clamp(std::fabs(velocity_error) / this->cf_velocity_norm_deg_s_, 0.0, 1.0);
        const double signed_error =
            (this->cf_mix_position_weight_ * normalized_angle +
             (1.0 - this->cf_mix_position_weight_) * normalized_velocity) *
            (angle_error >= 0.0 ? 1.0 : -1.0);

        const int seed = std::max(0, static_cast<int>(std::llround(this->cf_spike_scale_ * std::fabs(signed_error))));
        const std::vector<int>& neurons =
            signed_error >= 0.0 ? this->joint_mappings_[joint].cf_positive_neurons : this->joint_mappings_[joint].cf_negative_neurons;

        if (neurons.empty() || seed <= 0) {
            continue;
        }

        std::uniform_int_distribution<int> neuron_dist(0, static_cast<int>(neurons.size()) - 1);
        for (int spike = 0; spike < seed; ++spike) {
            const int neuron_id = neurons[neuron_dist(this->rng_)];
            const int spike_time = time + 1 + time_offset_dist(this->rng_);
            this->EmitSpike(simulation, neuron_id, spike_time);
        }
    }
}

void TotalModelPlanarArm2DOF::EmitSpike(Simulation* simulation, int neuron_id, int time) {
    if (neuron_id < 0 || neuron_id >= simulation->network->neuronsNum) {
        return;
    }
    simulation->QueueOuterDynamicInputSpike(time, neuron_id);
}

std::array<double, 2> TotalModelPlanarArm2DOF::HandTrajectory(double phase) const {
    const double angle = 2.0 * kPi * phase;
    return {
        this->trajectory_radius_ * std::cos(angle) + this->trajectory_center_x_,
        this->trajectory_radius_ * std::sin(angle) + this->trajectory_center_y_
    };
}

std::array<double, 2> TotalModelPlanarArm2DOF::HandVelocity(double phase) const {
    const double angle = 2.0 * kPi * phase;
    const double omega = 2.0 * kPi / this->desired_period_s_;
    return {
        -this->trajectory_radius_ * omega * std::sin(angle),
        this->trajectory_radius_ * omega * std::cos(angle)
    };
}

std::array<double, 2> TotalModelPlanarArm2DOF::InverseKinematics(double x, double y) const {
    const double l1 = this->link_lengths_[0];
    const double l2 = this->link_lengths_[1];
    const double c2 = Clamp((x * x + y * y - l1 * l1 - l2 * l2) / (2.0 * l1 * l2), -1.0, 1.0);
    const double q2 = std::acos(c2);
    const double phi = std::acos(Clamp((l2 * l2 - l1 * l1 - x * x - y * y) / (-2.0 * l1 * std::sqrt(x * x + y * y)), -1.0, 1.0));
    const double q1 = std::atan2(y, x) - phi;
    return { q1, q2 };
}

std::array<double, 2> TotalModelPlanarArm2DOF::InverseVelocity(
    const std::array<double, 2>& q,
    double xdot,
    double ydot) const {
    const double q1 = q[0];
    const double q2 = q[1];
    const double l1 = this->link_lengths_[0];
    const double l2 = this->link_lengths_[1];
    const double s1 = std::sin(q1);
    const double c1 = std::cos(q1);
    const double s12 = std::sin(q1 + q2);
    const double c12 = std::cos(q1 + q2);

    const double j11 = -l1 * s1 - l2 * s12;
    const double j12 = -l2 * s12;
    const double j21 = l1 * c1 + l2 * c12;
    const double j22 = l2 * c12;
    const double det = j11 * j22 - j12 * j21;
    if (std::fabs(det) < 1.0e-9) {
        return { 0.0, 0.0 };
    }

    return {
        (j22 * xdot - j12 * ydot) / det,
        (-j21 * xdot + j11 * ydot) / det
    };
}

std::array<double, 2> TotalModelPlanarArm2DOF::JointAcceleration(const std::array<double, 2>& tau) const {
    const double theta2 = this->q_[1];
    const double d_theta1 = this->qd_[0];
    const double d_theta2 = this->qd_[1];
    const double l1 = this->link_lengths_[0];
    const double l2 = this->link_lengths_[1];
    const double m1 = this->link_masses_[0];
    const double m2 = this->link_masses_[1];
    const double d1 = l1 * 0.5;
    const double d2 = l2 * 0.5;
    const double i1 = (m1 * l1 * l1) / 3.0;
    const double i2 = (m2 * l2 * l2) / 12.0;
    const double c2 = std::cos(theta2);
    const double s2 = std::sin(theta2);

    const double m11 = i1 + i2 + 2.0 * m2 * l1 * d2 * c2 + m1 * d1 * d1 + m2 * (d2 * d2 + l1 * l1);
    const double m12 = i2 + m2 * l1 * d2 * c2 + m2 * d2 * d2;
    const double m21 = m12;
    const double m22 = i2 + m2 * d2 * d2;

    const double b1 = -2.0 * m2 * l1 * d2 * s2 * d_theta1 * d_theta2;
    const double b2 = 0.0;
    const double c1 = -m2 * l1 * d2 * s2 * d_theta2 * d_theta2;
    const double c2_term = m2 * l1 * d2 * s2 * d_theta1 * d_theta1;

    const double rhs1 = tau[0] - b1 - c1;
    const double rhs2 = tau[1] - b2 - c2_term;
    const double det = m11 * m22 - m12 * m21;
    if (std::fabs(det) < 1.0e-12) {
        return { 0.0, 0.0 };
    }

    return {
        (rhs1 * m22 - rhs2 * m12) / det,
        (m11 * rhs2 - m21 * rhs1) / det
    };
}

double TotalModelPlanarArm2DOF::Clamp(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(max_value, value));
}

double TotalModelPlanarArm2DOF::RadToDeg(double radians) {
    return radians * 180.0 / kPi;
}
