#ifndef PLANAR_ARM_2DOF_PINOCCHIO_H
#define PLANAR_ARM_2DOF_PINOCCHIO_H

#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicModel.h"

#include <random>
#include <unordered_map>
#include <vector>

#include <Eigen/Dense>
#include <pinocchio/algorithm/aba.hpp>
#include <pinocchio/algorithm/rnea.hpp>
#include <pinocchio/algorithm/kinematics.hpp>
#include <pinocchio/algorithm/frames.hpp>
#include <pinocchio/multibody/data.hpp>
#include <pinocchio/multibody/frame.hpp>
#include <pinocchio/multibody/model.hpp>
#include <pinocchio/multibody/joint/joint-revolute.hpp>
#include <pinocchio/spatial/se3.hpp>

class PlanarArm2DOFPinocchio : public OuterDynamicModel {
public:
    explicit PlanarArm2DOFPinocchio(int timestep_size);
    virtual ~PlanarArm2DOFPinocchio();

    virtual void Initialize(const OuterDynamicDescription& description, Simulation* simulation);
    virtual void Update(int time, Simulation* simulation);

private:
    struct JointMapping {
        std::vector<int> mf_neurons;
        std::vector<int> cf_positive_neurons;
        std::vector<int> cf_negative_neurons;
        std::vector<int> dcn_positive_neurons;
        std::vector<int> dcn_negative_neurons;
    };

    pinocchio::Model model_;
    pinocchio::Data data_aba_;
    pinocchio::Data data_rnea_;
    pinocchio::Data data_fk_;
    bool model_ready_;
    pinocchio::FrameIndex end_effector_frame_id_;
    std::vector<JointMapping> joint_mappings_;

    Eigen::Vector2d q_;
    Eigen::Vector2d v_;
    Eigen::Vector2d qdd_;
    Eigen::Vector2d q_des_;
    Eigen::Vector2d v_des_;
    Eigen::Vector2d qdd_des_;
    Eigen::Vector2d tau_pd_;
    Eigen::Vector2d tau_id_;
    Eigen::Vector2d tau_cereb_;
    Eigen::Vector2d tau_cereb_positive_;
    Eigen::Vector2d tau_cereb_negative_;
    Eigen::Vector2d tau_total_;

    Eigen::Vector2d link_lengths_;
    Eigen::Vector2d link_masses_;
    Eigen::Vector2d damping_;
    Eigen::Vector2d kp_;
    Eigen::Vector2d kd_;

    double gravity_magnitude_;
    double trajectory_frequency_hz_;
    double circle_center_x_;
    double circle_center_y_;
    double circle_radius_;
    double dcn_torque_gain_;
    double cf_max_torque_;
    double cf_error_kp_;
    double cf_error_kd_;
    double cf_learning_max_spike_frequency_;
    double cf_learning_base_current_;
    double cf_max_low_torque_factor_;
    double mf_min_position_;
    double mf_max_position_;
    double mf_min_velocity_;
    double mf_max_velocity_;
    double mf_max_spike_frequency_;
    double mf_overlapping_factor_;
    double cf_error_max_;
    double cf_min_spike_frequency_;
    double cf_max_spike_frequency_;
    double cf_overlapping_factor_;
    int cf_burst_spacing_steps_;
    int spike_retention_steps_;
    bool vertical_plane_;
    bool elbow_down_;
    bool enable_inverse_dynamics_;

    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_rand_;
    std::unordered_map<int, int> next_mf_spike_step_;
    std::unordered_map<int, int> last_cf_spike_step_;

    struct IKResult {
        Eigen::Vector2d q;
        bool reachable;
    };

    void BuildPlanarModel();
    void UpdateDesiredState(double time_seconds);
    void DecodeDcnTorques(int time, double step_seconds, Simulation* simulation);
    void IntegrateDynamics(double step_seconds);
    void EncodeAndEmitStateSpikes(int time, double step_seconds, Simulation* simulation);
    void EmitSpike(Simulation* simulation, int neuron_id, int time);
    IKResult InverseKinematics2R(double x, double y) const;
    Eigen::Matrix2d Jacobian2R(const Eigen::Vector2d& q) const;
    Eigen::Matrix2d JacobianDot2R(const Eigen::Vector2d& q, const Eigen::Vector2d& qdot) const;
    Eigen::Vector2d DampedLeastSquaresSolve(const Eigen::Matrix2d& jacobian, const Eigen::Vector2d& rhs) const;
    void EmitEdlutLearningGroup(
        Simulation* simulation,
        const std::vector<int>& neurons,
        double input_current,
        int time,
        double step_seconds);
    static double ErrorSigmoid(double error_torque, double max_error_torque);

    static double ClampToRange(double value, double min_value, double max_value);
    static double UniformCenter(int index, int count, double min_value, double max_value);
    static double UniformWidth(int count, double min_value, double max_value, double overlap);
    static bool SquareRbfActive(double value, double center, double width);
    static double TanhRateActivation(double value, double center, double width);
    static int FrequencyToPeriodSteps(double frequency_hz, double timestep_s);
};

#endif
