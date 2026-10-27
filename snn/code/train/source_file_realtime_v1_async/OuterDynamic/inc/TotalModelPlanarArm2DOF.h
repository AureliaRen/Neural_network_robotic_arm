#ifndef TOTAL_MODEL_PLANAR_ARM_2DOF_H
#define TOTAL_MODEL_PLANAR_ARM_2DOF_H

#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicModel.h"

#include <array>
#include <random>
#include <vector>

class Simulation;

class TotalModelPlanarArm2DOF : public OuterDynamicModel {
public:
    explicit TotalModelPlanarArm2DOF(int timestep_size);
    virtual ~TotalModelPlanarArm2DOF();

    virtual void Initialize(const OuterDynamicDescription& description, Simulation* simulation);
    virtual void Update(int time, Simulation* simulation);

private:
    struct JointMapping {
        std::vector<int> gc_neurons;
        std::vector<int> cf_positive_neurons;
        std::vector<int> cf_negative_neurons;
        std::vector<int> dcn_positive_neurons;
        std::vector<int> dcn_negative_neurons;
    };

    std::vector<JointMapping> joint_mappings_;
    std::array<int, 4> mf_channel_bins_;
    std::array<double, 2> link_lengths_;
    std::array<double, 2> link_masses_;
    std::array<double, 2> q_;
    std::array<double, 2> qd_;
    std::array<double, 2> qdd_;
    std::array<double, 2> q_des_;
    std::array<double, 2> qd_des_;
    std::array<double, 2> tau_total_;
    std::array<double, 2> angle_min_deg_;
    std::array<double, 2> angle_max_deg_;
    std::array<double, 2> angle_range_deg_;
    std::array<double, 2> velocity_min_deg_;
    std::array<double, 2> velocity_max_deg_;
    std::array<double, 2> torque_scale_;

    double trajectory_center_x_;
    double trajectory_center_y_;
    double trajectory_radius_;
    double desired_period_s_;
    double cf_mix_position_weight_;
    double cf_angle_norm_deg_;
    double cf_elbow_norm_deg_;
    double cf_velocity_norm_deg_s_;
    double cf_spike_scale_;
    int spike_retention_steps_;

    std::mt19937 rng_;

    void UpdateDesiredState(double time_s);
    void DecodeDcnTorqueWindow(int time, Simulation* simulation);
    void IntegratePlant(double interval_s);
    void EmitGranuleCodeSpikes(int time, Simulation* simulation);
    void EmitClimbingFiberSpikes(int time, Simulation* simulation);
    void EmitSpike(Simulation* simulation, int neuron_id, int time);

    std::array<double, 2> HandTrajectory(double phase) const;
    std::array<double, 2> HandVelocity(double phase) const;
    std::array<double, 2> InverseKinematics(double x, double y) const;
    std::array<double, 2> InverseVelocity(const std::array<double, 2>& q, double xdot, double ydot) const;
    std::array<double, 2> JointAcceleration(const std::array<double, 2>& tau) const;

    static double Clamp(double value, double min_value, double max_value);
    static double RadToDeg(double radians);
};

#endif
