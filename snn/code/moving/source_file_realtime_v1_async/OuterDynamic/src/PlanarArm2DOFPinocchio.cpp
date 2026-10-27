#include "../source_file_realtime_v1_async/OuterDynamic/inc/PlanarArm2DOFPinocchio.h"

#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicSpikeBuffer.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"

#include <boost/any.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
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

Eigen::Vector2d GetVector2OrDefault(const std::map<std::string, boost::any>& params, const std::string& key, const Eigen::Vector2d& default_value) {
    std::map<std::string, boost::any>::const_iterator it = params.find(key);
    if (it == params.end()) {
        return default_value;
    }
    try {
        const std::vector<double> values = boost::any_cast<std::vector<double>>(it->second);
        if (values.size() != 2) {
            return default_value;
        }
        return Eigen::Vector2d(values[0], values[1]);
    } catch (const boost::bad_any_cast&) {
        return default_value;
    }
}

constexpr double kPi = 3.14159265358979323846;

} // namespace

PlanarArm2DOFPinocchio::PlanarArm2DOFPinocchio(int timestep_size)
    : OuterDynamicModel(timestep_size),
      model_(),
      data_aba_(model_),
      data_rnea_(model_),
      data_fk_(model_),
      model_ready_(false),
      end_effector_frame_id_(0),
      q_(Eigen::Vector2d::Zero()),
      v_(Eigen::Vector2d::Zero()),
      qdd_(Eigen::Vector2d::Zero()),
      q_des_(Eigen::Vector2d::Zero()),
      v_des_(Eigen::Vector2d::Zero()),
      qdd_des_(Eigen::Vector2d::Zero()),
      tau_pd_(Eigen::Vector2d::Zero()),
      tau_id_(Eigen::Vector2d::Zero()),
      tau_cereb_(Eigen::Vector2d::Zero()),
      tau_cereb_positive_(Eigen::Vector2d::Zero()),
      tau_cereb_negative_(Eigen::Vector2d::Zero()),
      tau_total_(Eigen::Vector2d::Zero()),
      link_lengths_(1.0, 1.0),
      link_masses_(1.0, 1.0),
      damping_(0.05, 0.05),
      kp_(12.0, 10.0),
      kd_(2.0, 1.8),
      gravity_magnitude_(9.81),
      trajectory_frequency_hz_(0.35),
      circle_center_x_(1.0),
      circle_center_y_(0.2),
      circle_radius_(0.3),
      dcn_torque_gain_(0.0025),
      cf_max_torque_(1.0),
      cf_error_kp_(0.5),
      cf_error_kd_(0.5 / (2.0 * kPi)),
      cf_learning_max_spike_frequency_(5.0),
      cf_learning_base_current_(0.0),
      cf_max_low_torque_factor_(0.2),
      mf_min_position_(-2.14),
      mf_max_position_(1.70),
      mf_min_velocity_(-2.0),
      mf_max_velocity_(2.0),
      mf_max_spike_frequency_(250.0),
      mf_overlapping_factor_(1.05),
      cf_error_max_(0.45),
      cf_min_spike_frequency_(0.0),
      cf_max_spike_frequency_(12.0),
      cf_overlapping_factor_(1.2),
      cf_burst_spacing_steps_(4),
      spike_retention_steps_(256),
      vertical_plane_(true),
      elbow_down_(true),
      enable_inverse_dynamics_(true),
      rng_(7),
      unit_rand_(0.0, 1.0) {}

PlanarArm2DOFPinocchio::~PlanarArm2DOFPinocchio() {}

void PlanarArm2DOFPinocchio::Initialize(const OuterDynamicDescription& description, Simulation* simulation) {
    this->setTimestepSize(std::max(1, description.update_timestep));
    this->setCommunicationInterval(std::max(1, description.communication_interval));
    this->setQueueIndex(description.queue_index);

    this->link_lengths_ = GetVector2OrDefault(description.ModelParameter, "link_lengths", this->link_lengths_);
    this->link_masses_ = GetVector2OrDefault(description.ModelParameter, "link_masses", this->link_masses_);
    this->damping_ = GetVector2OrDefault(description.ModelParameter, "joint_damping", this->damping_);
    this->kp_ = GetVector2OrDefault(description.ModelParameter, "pd_kp", this->kp_);
    this->kd_ = GetVector2OrDefault(description.ModelParameter, "pd_kd", this->kd_);

    this->gravity_magnitude_ = GetAnyOrDefault<double>(description.ModelParameter, "gravity", this->gravity_magnitude_);
    this->trajectory_frequency_hz_ = GetAnyOrDefault<double>(description.ModelParameter, "trajectory_frequency_hz", this->trajectory_frequency_hz_);
    this->circle_center_x_ = GetAnyOrDefault<double>(description.ModelParameter, "circle_center_x", this->circle_center_x_);
    this->circle_center_y_ = GetAnyOrDefault<double>(description.ModelParameter, "circle_center_y", this->circle_center_y_);
    this->circle_radius_ = GetAnyOrDefault<double>(description.ModelParameter, "circle_radius", this->circle_radius_);
    this->dcn_torque_gain_ = GetAnyOrDefault<double>(description.ModelParameter, "dcn_torque_gain", this->dcn_torque_gain_);
    this->cf_max_torque_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_max_torque", this->cf_max_torque_);
    this->cf_error_kp_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_error_kp", this->cf_error_kp_);
    this->cf_error_kd_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_error_kd", this->cf_error_kd_);
    this->cf_learning_max_spike_frequency_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_learning_max_spike_frequency", this->cf_learning_max_spike_frequency_);
    this->cf_learning_base_current_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_learning_base_current", this->cf_learning_base_current_);
    this->cf_max_low_torque_factor_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_max_low_torque_factor", this->cf_max_low_torque_factor_);
    this->mf_min_position_ = GetAnyOrDefault<double>(description.ModelParameter, "mf_min_position", this->mf_min_position_);
    this->mf_max_position_ = GetAnyOrDefault<double>(description.ModelParameter, "mf_max_position", this->mf_max_position_);
    this->mf_min_velocity_ = GetAnyOrDefault<double>(description.ModelParameter, "mf_min_velocity", this->mf_min_velocity_);
    this->mf_max_velocity_ = GetAnyOrDefault<double>(description.ModelParameter, "mf_max_velocity", this->mf_max_velocity_);
    this->mf_max_spike_frequency_ = GetAnyOrDefault<double>(description.ModelParameter, "mf_max_spike_frequency", this->mf_max_spike_frequency_);
    this->mf_overlapping_factor_ = GetAnyOrDefault<double>(description.ModelParameter, "mf_overlapping_factor", this->mf_overlapping_factor_);
    this->cf_error_max_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_error_max", this->cf_error_max_);
    this->cf_min_spike_frequency_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_min_spike_frequency", this->cf_min_spike_frequency_);
    this->cf_max_spike_frequency_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_max_spike_frequency", this->cf_max_spike_frequency_);
    this->cf_overlapping_factor_ = GetAnyOrDefault<double>(description.ModelParameter, "cf_overlapping_factor", this->cf_overlapping_factor_);
    this->cf_burst_spacing_steps_ = GetAnyOrDefault<int>(description.ModelParameter, "cf_burst_spacing_steps", this->cf_burst_spacing_steps_);
    this->spike_retention_steps_ = GetAnyOrDefault<int>(description.ModelParameter, "spike_retention_steps", this->spike_retention_steps_);
    this->vertical_plane_ = GetAnyOrDefault<bool>(description.ModelParameter, "vertical_plane", this->vertical_plane_);
    this->elbow_down_ = GetAnyOrDefault<bool>(description.ModelParameter, "elbow_down", this->elbow_down_);
    this->enable_inverse_dynamics_ = GetAnyOrDefault<bool>(description.ModelParameter, "enable_inverse_dynamics", this->enable_inverse_dynamics_);
    this->rng_.seed(static_cast<std::mt19937::result_type>(GetAnyOrDefault<int>(description.ModelParameter, "poisson_seed", 7)));

    const std::size_t dof = description.mf_neuron_indices_by_joint.size();
    if (dof != 2 ||
        description.cf_positive_neuron_indices_by_joint.size() != dof ||
        description.cf_negative_neuron_indices_by_joint.size() != dof ||
        description.dcn_positive_neuron_indices_by_joint.size() != dof ||
        description.dcn_negative_neuron_indices_by_joint.size() != dof) {
        throw std::runtime_error("PlanarArm2DOFPinocchio expects explicit 2DOF MF/CF/DCN mappings.");
    }

    this->joint_mappings_.resize(dof);
    std::vector<int> watched_dcn_neurons;
    for (std::size_t joint = 0; joint < dof; ++joint) {
        JointMapping& mapping = this->joint_mappings_[joint];
        mapping.mf_neurons = description.mf_neuron_indices_by_joint[joint];
        mapping.cf_positive_neurons = description.cf_positive_neuron_indices_by_joint[joint];
        mapping.cf_negative_neurons = description.cf_negative_neuron_indices_by_joint[joint];
        mapping.dcn_positive_neurons = description.dcn_positive_neuron_indices_by_joint[joint];
        mapping.dcn_negative_neurons = description.dcn_negative_neuron_indices_by_joint[joint];

        watched_dcn_neurons.insert(watched_dcn_neurons.end(), mapping.dcn_positive_neurons.begin(), mapping.dcn_positive_neurons.end());
        watched_dcn_neurons.insert(watched_dcn_neurons.end(), mapping.dcn_negative_neurons.begin(), mapping.dcn_negative_neurons.end());
    }

    if (simulation->outer_dynamic_spike_buffer != NULL) {
        simulation->outer_dynamic_spike_buffer->RegisterWatchedNeurons(watched_dcn_neurons);
    }

    this->BuildPlanarModel();
    this->UpdateDesiredState(0.0);
    this->q_ = this->q_des_;
    this->v_ = this->v_des_;
    this->qdd_ = this->qdd_des_;
    this->tau_total_ = this->tau_cereb_;
}

void PlanarArm2DOFPinocchio::Update(int time, Simulation* simulation) {
    if (!this->model_ready_) {
        throw std::runtime_error("PlanarArm2DOFPinocchio model not initialized.");
    }

    const double step_seconds = static_cast<double>(simulation->basetimesteps) * 0.001 * static_cast<double>(this->getTimestepSize());
    const double time_seconds = static_cast<double>(time) * static_cast<double>(simulation->basetimesteps) * 0.001;

    this->UpdateDesiredState(time_seconds);
    this->DecodeDcnTorques(time, step_seconds, simulation);

    const Eigen::Vector2d position_error = this->q_des_ - this->q_;
    const Eigen::Vector2d velocity_error = this->v_des_ - this->v_;
    //this->tau_pd_ = this->kp_.cwiseProduct(position_error) + this->kd_.cwiseProduct(velocity_error);
    this->tau_total_ = this->tau_cereb_; //this->tau_pd_ + this->tau_cereb_;
    //if (this->enable_inverse_dynamics_) {
        //this->tau_total_ += this->tau_id_;
    //}

    this->IntegrateDynamics(step_seconds);
    OuterDynamicJointState state;
    state.q = { this->q_(0), this->q_(1) };
    state.qv = { this->v_(0), this->v_(1) };
    state.q_des = { this->q_des_(0), this->q_des_(1) };
    state.qv_des = { this->v_des_(0), this->v_des_(1) };
    state.tau_total = { this->tau_total_(0), this->tau_total_(1) };
    simulation->WriteOuterDynamicState(time, state);
    this->EncodeAndEmitStateSpikes(time, step_seconds, simulation);

    if (simulation->outer_dynamic_spike_buffer != NULL) {
        const int discard_before = std::max(0, time - this->spike_retention_steps_);
        simulation->outer_dynamic_spike_buffer->DiscardOlderThan(discard_before);
    }
}

void PlanarArm2DOFPinocchio::BuildPlanarModel() {
    pinocchio::Model temp_model;
    if (this->vertical_plane_) {
        temp_model.gravity.linear() = Eigen::Vector3d(0.0, -this->gravity_magnitude_, 0.0);
    } else {
        temp_model.gravity.linear() = Eigen::Vector3d::Zero();
    }

    const double inertia1 = this->link_masses_(0) * this->link_lengths_(0) * this->link_lengths_(0) / 12.0;
    const double inertia2 = this->link_masses_(1) * this->link_lengths_(1) * this->link_lengths_(1) / 12.0;

    const pinocchio::JointIndex joint1 =
        temp_model.addJoint(
            0,
            pinocchio::JointModelRZ(),
            pinocchio::SE3::Identity(),
            "joint1");

    Eigen::Matrix3d link1_inertia_matrix = Eigen::Matrix3d::Zero();
    link1_inertia_matrix(0, 0) = 1.0e-6;
    link1_inertia_matrix(1, 1) = inertia1;
    link1_inertia_matrix(2, 2) = inertia1;

    temp_model.appendBodyToJoint(
        joint1,
        pinocchio::Inertia(
            this->link_masses_(0),
            Eigen::Vector3d(this->link_lengths_(0) * 0.5, 0.0, 0.0),
            link1_inertia_matrix),
        pinocchio::SE3::Identity());

    const pinocchio::SE3 joint2_placement(
        Eigen::Matrix3d::Identity(),
        Eigen::Vector3d(this->link_lengths_(0), 0.0, 0.0));

    const pinocchio::JointIndex joint2 =
        temp_model.addJoint(
            joint1,
            pinocchio::JointModelRZ(),
            joint2_placement,
            "joint2");

    Eigen::Matrix3d link2_inertia_matrix = Eigen::Matrix3d::Zero();
    link2_inertia_matrix(0, 0) = 1.0e-6;
    link2_inertia_matrix(1, 1) = inertia2;
    link2_inertia_matrix(2, 2) = inertia2;

    temp_model.appendBodyToJoint(
        joint2,
        pinocchio::Inertia(
            this->link_masses_(1),
            Eigen::Vector3d(this->link_lengths_(1) * 0.5, 0.0, 0.0),
            link2_inertia_matrix),
        pinocchio::SE3::Identity());

    temp_model.addFrame(
        pinocchio::Frame(
            "end_effector",
            joint2,
            joint2,
            pinocchio::SE3(
                Eigen::Matrix3d::Identity(),
                Eigen::Vector3d(this->link_lengths_(1), 0.0, 0.0)),
            pinocchio::OP_FRAME));

    this->model_ = std::move(temp_model);
    this->data_aba_ = pinocchio::Data(this->model_);
    this->data_rnea_ = pinocchio::Data(this->model_);
    this->data_fk_ = pinocchio::Data(this->model_);
    this->end_effector_frame_id_ = this->model_.getFrameId("end_effector");
    this->model_ready_ = true;
}

void PlanarArm2DOFPinocchio::UpdateDesiredState(double time_seconds) {
    const double omega = 2.0 * kPi * this->trajectory_frequency_hz_;

    const double xd = this->circle_center_x_ + this->circle_radius_ * std::cos(omega * time_seconds);
    const double yd = this->circle_center_y_ + this->circle_radius_ * std::sin(omega * time_seconds);
    const double xd_dot = -this->circle_radius_ * omega * std::sin(omega * time_seconds);
    const double yd_dot = this->circle_radius_ * omega * std::cos(omega * time_seconds);
    const double xd_ddot = -this->circle_radius_ * omega * omega * std::cos(omega * time_seconds);
    const double yd_ddot = -this->circle_radius_ * omega * omega * std::sin(omega * time_seconds);

    const IKResult ik_result = this->InverseKinematics2R(xd, yd);
    if (!ik_result.reachable) {
        throw std::runtime_error("Desired circular trajectory is unreachable for PlanarArm2DOFPinocchio.");
    }

    this->q_des_ = ik_result.q;

    const Eigen::Vector2d xdot_des(xd_dot, yd_dot);
    const Eigen::Vector2d xddot_des(xd_ddot, yd_ddot);
    const Eigen::Matrix2d jacobian = this->Jacobian2R(this->q_des_);

    this->v_des_ = this->DampedLeastSquaresSolve(jacobian, xdot_des);
    this->qdd_des_ = this->DampedLeastSquaresSolve(
        jacobian,
        xddot_des - this->JacobianDot2R(this->q_des_, this->v_des_) * this->v_des_);
    /*
    if (this->enable_inverse_dynamics_) {
        this->tau_id_ = pinocchio::rnea(
            this->model_,
            this->data_rnea_,
            this->q_des_,
            this->v_des_,
            this->qdd_des_);
    } else {
        this->tau_id_.setZero();
    }
    */
}

void PlanarArm2DOFPinocchio::DecodeDcnTorques(int time, double step_seconds, Simulation* simulation) {
    this->tau_cereb_.setZero();
    this->tau_cereb_positive_.setZero();
    this->tau_cereb_negative_.setZero();
    if (simulation->outer_dynamic_spike_buffer == NULL) {
        return;
    }

    const int window_steps = std::max(1, this->getCommunicationInterval());
    const int window_start = std::max(0, time - window_steps);
    const double window_seconds = std::max(step_seconds, static_cast<double>(window_steps) * static_cast<double>(simulation->basetimesteps) * 0.001);

    for (int joint = 0; joint < 2; ++joint) {
        const JointMapping& mapping = this->joint_mappings_[joint];
        const int pos_count = simulation->outer_dynamic_spike_buffer->CountSpikesInWindow(window_start, time, mapping.dcn_positive_neurons);
        const int neg_count = simulation->outer_dynamic_spike_buffer->CountSpikesInWindow(window_start, time, mapping.dcn_negative_neurons);
        const double positive_rate = static_cast<double>(pos_count) / window_seconds;
        const double negative_rate = static_cast<double>(neg_count) / window_seconds;
        this->tau_cereb_positive_(joint) = this->dcn_torque_gain_ * positive_rate;
        this->tau_cereb_negative_(joint) = this->dcn_torque_gain_ * negative_rate;
        this->tau_cereb_(joint) = this->tau_cereb_positive_(joint) - this->tau_cereb_negative_(joint);
    }
}

void PlanarArm2DOFPinocchio::IntegrateDynamics(double step_seconds) {
    const Eigen::Vector2d tau_input = this->tau_total_ - this->damping_.cwiseProduct(this->v_);
    this->qdd_ = pinocchio::aba(this->model_, this->data_aba_, this->q_, this->v_, tau_input);
    this->v_ += this->qdd_ * step_seconds;
    this->q_ += this->v_ * step_seconds;
}

void PlanarArm2DOFPinocchio::EncodeAndEmitStateSpikes(int time, double step_seconds, Simulation* simulation) {
    const int injection_base_time = time + this->getTimestepSize();
    for (int joint = 0; joint < 2; ++joint) {
        const JointMapping& mapping = this->joint_mappings_[joint];
        if (mapping.mf_neurons.empty()) {
            continue;
        }

        const int bins_per_channel = static_cast<int>(mapping.mf_neurons.size() / 4);
        if (bins_per_channel <= 0 || static_cast<std::size_t>(bins_per_channel * 4) != mapping.mf_neurons.size()) {
            throw std::runtime_error("MF mapping must contain 4 equally-sized channels per joint.");
        }

        const std::vector<double> mf_values = { this->q_des_(joint), this->q_(joint), this->v_des_(joint), this->v_(joint) };
        const std::vector<std::pair<double, double> > ranges = {
            std::make_pair(this->mf_min_position_, this->mf_max_position_),
            std::make_pair(this->mf_min_position_, this->mf_max_position_),
            std::make_pair(this->mf_min_velocity_, this->mf_max_velocity_),
            std::make_pair(this->mf_min_velocity_, this->mf_max_velocity_)
        };

        for (int channel = 0; channel < 4; ++channel) {
            const double value = ClampToRange(mf_values[channel], ranges[channel].first, ranges[channel].second);
            const double width = UniformWidth(bins_per_channel, ranges[channel].first, ranges[channel].second, this->mf_overlapping_factor_);
            for (int bin = 0; bin < bins_per_channel; ++bin) {
                const int neuron_id = mapping.mf_neurons[channel * bins_per_channel + bin];
                const double center = UniformCenter(bin, bins_per_channel, ranges[channel].first, ranges[channel].second);
                if (!SquareRbfActive(value, center, width)) {
                    continue;
                }
                int& next_step = this->next_mf_spike_step_[neuron_id];
                if (next_step <= 0 || next_step < injection_base_time) {
                    next_step = injection_base_time;
                }
                const int period_steps = FrequencyToPeriodSteps(this->mf_max_spike_frequency_, step_seconds);
                if (next_step == injection_base_time) {
                    this->EmitSpike(simulation, neuron_id, next_step);
                    next_step += period_steps;
                }
            }
        }

        if (mapping.cf_positive_neurons.empty() || mapping.cf_negative_neurons.size() != mapping.cf_positive_neurons.size()) {
            continue;
        }

        const double error_value =
            this->cf_error_kp_ * (this->q_(joint) - this->q_des_(joint)) +
            this->cf_error_kd_ * (this->v_(joint) - this->v_des_(joint));

        double positive_current = this->cf_learning_base_current_;
        double negative_current = this->cf_learning_base_current_;

        if (error_value <= 0.0) {
            positive_current = ErrorSigmoid(-error_value, this->cf_max_torque_);
            if (this->tau_cereb_negative_(joint) > this->cf_max_torque_ * this->cf_max_low_torque_factor_) {
                negative_current = 0.0;
            }
        } else {
            negative_current = ErrorSigmoid(error_value, this->cf_max_torque_);
            if (this->tau_cereb_positive_(joint) > this->cf_max_torque_ * this->cf_max_low_torque_factor_) {
                positive_current = 0.0;
            }
        }

        this->EmitEdlutLearningGroup(simulation, mapping.cf_positive_neurons, positive_current, injection_base_time, step_seconds);
        this->EmitEdlutLearningGroup(simulation, mapping.cf_negative_neurons, negative_current, injection_base_time, step_seconds);
    }
}

void PlanarArm2DOFPinocchio::EmitSpike(Simulation* simulation, int neuron_id, int time) {
    if (neuron_id < 0 || neuron_id >= simulation->network->neuronsNum) {
        return;
    }
    simulation->QueueOuterDynamicInputSpike(time, neuron_id);
}

PlanarArm2DOFPinocchio::IKResult PlanarArm2DOFPinocchio::InverseKinematics2R(double x, double y) const {
    IKResult result;
    result.q.setZero();
    result.reachable = true;

    const double l1 = this->link_lengths_(0);
    const double l2 = this->link_lengths_(1);
    const double radius_squared = x * x + y * y;
    const double c2_raw = (radius_squared - l1 * l1 - l2 * l2) / (2.0 * l1 * l2);
    if (c2_raw < -1.0 || c2_raw > 1.0) {
        result.reachable = false;
        return result;
    }

    const double c2 = std::clamp(c2_raw, -1.0, 1.0);
    double s2 = std::sqrt(std::max(0.0, 1.0 - c2 * c2));
    if (!this->elbow_down_) {
        s2 = -s2;
    }

    const double q2 = std::atan2(s2, c2);
    const double k1 = l1 + l2 * c2;
    const double k2 = l2 * s2;
    const double q1 = std::atan2(y, x) - std::atan2(k2, k1);

    result.q << q1, q2;
    return result;
}

Eigen::Matrix2d PlanarArm2DOFPinocchio::Jacobian2R(const Eigen::Vector2d& q) const {
    const double q1 = q(0);
    const double q2 = q(1);
    const double s1 = std::sin(q1);
    const double c1 = std::cos(q1);
    const double s12 = std::sin(q1 + q2);
    const double c12 = std::cos(q1 + q2);

    Eigen::Matrix2d jacobian;
    jacobian(0, 0) = -this->link_lengths_(0) * s1 - this->link_lengths_(1) * s12;
    jacobian(0, 1) = -this->link_lengths_(1) * s12;
    jacobian(1, 0) = this->link_lengths_(0) * c1 + this->link_lengths_(1) * c12;
    jacobian(1, 1) = this->link_lengths_(1) * c12;
    return jacobian;
}

Eigen::Matrix2d PlanarArm2DOFPinocchio::JacobianDot2R(const Eigen::Vector2d& q, const Eigen::Vector2d& qdot) const {
    const double q1 = q(0);
    const double q2 = q(1);
    const double q1dot = qdot(0);
    const double q2dot = qdot(1);
    const double q12 = q1 + q2;
    const double q12dot = q1dot + q2dot;
    const double s1 = std::sin(q1);
    const double c1 = std::cos(q1);
    const double s12 = std::sin(q12);
    const double c12 = std::cos(q12);

    Eigen::Matrix2d jacobian_dot;
    jacobian_dot(0, 0) = -this->link_lengths_(0) * c1 * q1dot - this->link_lengths_(1) * c12 * q12dot;
    jacobian_dot(0, 1) = -this->link_lengths_(1) * c12 * q12dot;
    jacobian_dot(1, 0) = -this->link_lengths_(0) * s1 * q1dot - this->link_lengths_(1) * s12 * q12dot;
    jacobian_dot(1, 1) = -this->link_lengths_(1) * s12 * q12dot;
    return jacobian_dot;
}

Eigen::Vector2d PlanarArm2DOFPinocchio::DampedLeastSquaresSolve(const Eigen::Matrix2d& jacobian, const Eigen::Vector2d& rhs) const {
    const double lambda = 1.0e-8;
    const Eigen::Matrix2d matrix =
        jacobian * jacobian.transpose() +
        lambda * lambda * Eigen::Matrix2d::Identity();
    return jacobian.transpose() * matrix.ldlt().solve(rhs);
}

void PlanarArm2DOFPinocchio::EmitEdlutLearningGroup(
    Simulation* simulation,
    const std::vector<int>& neurons,
    double input_current,
    int time,
    double step_seconds) {
    if (neurons.empty() || input_current <= 0.0) {
        return;
    }

    int num_spikes = 1;
    if (input_current >= 0.75) {
        num_spikes = 3;
    } else if (input_current > 0.50) {
        num_spikes = 3;
    } else if (input_current > 0.25) {
        num_spikes = 2;
    }

    for (std::size_t index = 0; index < neurons.size(); ++index) {
        const int neuron_id = neurons[index];
        int& last_step = this->last_cf_spike_step_[neuron_id];
        double factor_time_since_spike = static_cast<double>(time - last_step) * step_seconds * this->cf_learning_max_spike_frequency_;
        if (factor_time_since_spike > 1.0) {
            factor_time_since_spike = 1.0;
        }

        if (factor_time_since_spike * input_current * step_seconds * this->cf_learning_max_spike_frequency_ > this->unit_rand_(this->rng_)) {
            int spike_time = time;
            for (int spike = 0; spike < num_spikes; ++spike) {
                this->EmitSpike(simulation, neuron_id, spike_time);
                spike_time += this->getTimestepSize();
            }
            last_step = spike_time;
        }
    }
}

double PlanarArm2DOFPinocchio::ErrorSigmoid(double error_torque, double max_error_torque) {
    if (max_error_torque <= 1.0e-9) {
        return 0.0;
    }
    return 0.2 + 0.8 * (1.0 - std::exp(-error_torque * 90.0 / max_error_torque));
}

double PlanarArm2DOFPinocchio::ClampToRange(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(max_value, value));
}

double PlanarArm2DOFPinocchio::UniformCenter(int index, int count, double min_value, double max_value) {
    if (count <= 1) {
        return 0.5 * (min_value + max_value);
    }
    const double step = (max_value - min_value) / static_cast<double>(count - 1);
    return min_value + step * static_cast<double>(index);
}

double PlanarArm2DOFPinocchio::UniformWidth(int count, double min_value, double max_value, double overlap) {
    if (count <= 1) {
        return std::max(1.0e-6, (max_value - min_value) * overlap);
    }
    const double step = (max_value - min_value) / static_cast<double>(count - 1);
    return std::max(1.0e-6, step * overlap);
}

bool PlanarArm2DOFPinocchio::SquareRbfActive(double value, double center, double width) {
    return std::fabs((value - center) / width) < 1.0;
}

double PlanarArm2DOFPinocchio::TanhRateActivation(double value, double center, double width) {
    return (std::tanh((value - center) / width) + 1.0) * 0.5;
}

int PlanarArm2DOFPinocchio::FrequencyToPeriodSteps(double frequency_hz, double timestep_s) {
    if (frequency_hz <= 0.0) {
        return std::numeric_limits<int>::max();
    }
    return std::max(1, static_cast<int>(std::llround(1.0 / (frequency_hz * timestep_s))));
}
