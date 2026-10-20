#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr float kConductanceScale = 1.0f / 6.0f;

struct Config {
    int test_rounds = 100;

    double dt_ms = 0.1;
    double control_interval_ms = 20.0;
    int sample_count = 50;

    double link1 = 0.34;
    double link2 = 0.32;
    double mass1 = 1.8;
    double mass2 = 1.6;

    int gc_repeats_per_control = 5;
    int nn = 15;
    int nv = 10;
    int n_gc = 15 * 15 * 10 * 10;
    int n_pc = 200;
    int n_cf = 200;
    int n_dcn = 200;

    double angle1_min_deg = -30.0;
    double angle1_range_deg = 120.0;
    double angle2_min_deg = 0.0;
    double angle2_range_deg = 150.0;
    double velocity1_min_deg = -400.0;
    double velocity1_range_deg = 800.0;
    double velocity2_min_deg = -400.0;
    double velocity2_range_deg = 800.0;

    float w_gc_dcn = 12.0f * kConductanceScale;
    float w_pc_dcn = 12.0f * kConductanceScale;
    float w_gcpc_init = 11.0f * kConductanceScale;
    float w_gcpc_jitter = 0.1f * kConductanceScale;
    float w_gcpc_max = 12.0f * kConductanceScale;
    float w_gcpc_min = 10.0f * kConductanceScale;
    float alpha = 6.0e-3f * kConductanceScale;
    float beta = -0.5f * kConductanceScale;

    double dcn_torque_gain_1 = 0.2;
    double dcn_torque_gain_2 = 0.05;
    double cf_mix_position = 0.8;
    double spike_cf_max = 15.0;
};

struct LayerIndex {
    int gc1 = 0;
    int gc2 = 0;
    int pc1 = 0;
    int pc2 = 0;
    int pc3 = 0;
    int pc4 = 0;
    int dcn1 = 0;
    int dcn2 = 0;
    int dcn3 = 0;
    int dcn4 = 0;
};

struct ProjectionSlice {
    int gc_begin = 0;
    int gc_count = 0;
    int pc_begin = 0;
    int pc_count = 0;
    int original_connection_begin = 0;
};

struct WeightProjectionRuntime {
    int gc_begin = 0;
    int gc_count = 0;
    int pc_begin = 0;
    int pc_count = 0;
    std::vector<std::vector<Interconnections*> > source_columns;
};

struct StateSample {
    double q1 = 0.0;
    double q2 = 0.0;
    double qd1 = 0.0;
    double qd2 = 0.0;
    double x = 0.0;
    double y = 0.0;
};

struct DcnWindowCounts {
    int dcn1 = 0;
    int dcn2 = 0;
    int dcn3 = 0;
    int dcn4 = 0;
};

struct PopulationWindowCounts {
    int pc1 = 0;
    int pc2 = 0;
    int pc3 = 0;
    int pc4 = 0;
    int dcn1 = 0;
    int dcn2 = 0;
    int dcn3 = 0;
    int dcn4 = 0;
};

struct CfWindowCounts {
    std::vector<int> cf1;
    std::vector<int> cf2;
    std::vector<int> cf3;
    std::vector<int> cf4;
};

std::string OutputRoot() {
    return "total_model_matlab_strict_outputs";
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

double Clamp(double value, double min_value, double max_value) {
    return std::max(min_value, std::min(max_value, value));
}

double RadToDeg(double value) {
    return value * 180.0 / kPi;
}

double DegToRad(double value) {
    return value * kPi / 180.0;
}

NeuronLayerDescription MakeInputLayer(int count, bool is_output) {
    NeuronLayerDescription layer;
    layer.ModelName = "InputSpikeNeuronModel";
    layer.numberofneuron = count;
    layer.isMonitored = false;
    layer.isOutput = false;
    return layer;
}

NeuronLayerDescription MakeLifLayer(int count, bool is_output) {
    NeuronLayerDescription layer;
    layer.ModelName = "TimeDrivenLIF_Exponential_double";
    layer.numberofneuron = count;
    layer.isMonitored = false;
    layer.isOutput = is_output;
    layer.NeuronParameter["V_rest"] = -70.0f;
    layer.NeuronParameter["V_reset"] = -80.0f;
    layer.NeuronParameter["V_th"] = -40.0f;
    layer.NeuronParameter["tau"] = 10.0f / 6.0f;
    layer.NeuronParameter["R"] = 1.0f;
    layer.NeuronParameter["t_ref"] = 10;
    layer.NeuronParameter["gexc_tau"] = 1.0f;
    layer.NeuronParameter["ginh_tau"] = 1.0f;
    layer.NeuronParameter["Eexc"] = 0.0f;
    layer.NeuronParameter["Einh"] = -80.0f;
    layer.NeuronParameter["random_mu"] = std::array<float, 4>{ -10.0f, 0.0f, 0.0f, 0.0f };
    layer.NeuronParameter["random_sigma"] = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f };
    return layer;
}

void FinalizeConnection(ConnectionDescription& desc, std::list<ConnectionDescription>& connections) {
    desc.SynapseModel.resize(desc.SourceNeuron.size());
    connections.push_back(std::move(desc));
}

void AddDenseProjection(
    std::list<ConnectionDescription>& connections,
    int source_begin,
    int source_count,
    int target_begin,
    int target_count,
    int type,
    float weight,
    float max_weight) {
    ConnectionDescription desc;
    const std::size_t total = static_cast<std::size_t>(source_count) * static_cast<std::size_t>(target_count);
    desc.SourceNeuron.reserve(total);
    desc.TargetNeuron.reserve(total);
    desc.Type.reserve(total);
    desc.Weight.reserve(total);
    desc.MaxWeight.reserve(total);
    desc.Delay.reserve(total);
    desc.SynapseRule.reserve(total);
    desc.TriggerRule.reserve(total);

    for (int target = 0; target < target_count; ++target) {
        for (int source = 0; source < source_count; ++source) {
            desc.SourceNeuron.push_back(source_begin + source);
            desc.TargetNeuron.push_back(target_begin + target);
            desc.Type.push_back(type);
            desc.Weight.push_back(weight);
            desc.MaxWeight.push_back(max_weight);
            desc.Delay.push_back(1);
            desc.SynapseRule.push_back(-1);
            desc.TriggerRule.push_back(-1);
        }
    }

    FinalizeConnection(desc, connections);
}

void AddOneToOneProjection(
    std::list<ConnectionDescription>& connections,
    int source_begin,
    int target_begin,
    int count,
    int type,
    float weight) {
    ConnectionDescription desc;
    desc.SourceNeuron.reserve(count);
    desc.TargetNeuron.reserve(count);
    desc.Type.reserve(count);
    desc.Weight.reserve(count);
    desc.MaxWeight.reserve(count);
    desc.Delay.reserve(count);
    desc.SynapseRule.reserve(count);
    desc.TriggerRule.reserve(count);

    for (int i = 0; i < count; ++i) {
        desc.SourceNeuron.push_back(source_begin + i);
        desc.TargetNeuron.push_back(target_begin + i);
        desc.Type.push_back(type);
        desc.Weight.push_back(weight);
        desc.MaxWeight.push_back(weight);
        desc.Delay.push_back(1);
        desc.SynapseRule.push_back(-1);
        desc.TriggerRule.push_back(-1);
    }

    FinalizeConnection(desc, connections);
}

LayerIndex BuildNeuronLayers(const Config& cfg, std::list<NeuronLayerDescription>& neurons) {
    LayerIndex index;
    int next = 0;

    index.gc1 = next; next += cfg.n_gc;
    neurons.push_back(MakeInputLayer(cfg.n_gc, false));
    index.gc2 = next; next += cfg.n_gc;
    neurons.push_back(MakeInputLayer(cfg.n_gc, false));

    index.pc1 = next; next += cfg.n_pc;
    neurons.push_back(MakeLifLayer(cfg.n_pc, false));
    index.pc2 = next; next += cfg.n_pc;
    neurons.push_back(MakeLifLayer(cfg.n_pc, false));
    index.pc3 = next; next += cfg.n_pc;
    neurons.push_back(MakeLifLayer(cfg.n_pc, false));
    index.pc4 = next; next += cfg.n_pc;
    neurons.push_back(MakeLifLayer(cfg.n_pc, false));

    index.dcn1 = next; next += cfg.n_dcn;
    neurons.push_back(MakeLifLayer(cfg.n_dcn, true));
    index.dcn2 = next; next += cfg.n_dcn;
    neurons.push_back(MakeLifLayer(cfg.n_dcn, true));
    index.dcn3 = next; next += cfg.n_dcn;
    neurons.push_back(MakeLifLayer(cfg.n_dcn, true));
    index.dcn4 = next; next += cfg.n_dcn;
    neurons.push_back(MakeLifLayer(cfg.n_dcn, true));

    return index;
}

std::array<ProjectionSlice, 4> BuildConnections(
    const Config& cfg,
    const LayerIndex& index,
    std::list<ConnectionDescription>& connections,
    std::mt19937& rng) {
    std::array<ProjectionSlice, 4> slices;
    const int gc_pc_projection_size = cfg.n_gc * cfg.n_pc;
    slices[0] = ProjectionSlice{ index.gc1, cfg.n_gc, index.pc1, cfg.n_pc, 0 * gc_pc_projection_size };
    slices[1] = ProjectionSlice{ index.gc1, cfg.n_gc, index.pc2, cfg.n_pc, 1 * gc_pc_projection_size };
    slices[2] = ProjectionSlice{ index.gc2, cfg.n_gc, index.pc3, cfg.n_pc, 2 * gc_pc_projection_size };
    slices[3] = ProjectionSlice{ index.gc2, cfg.n_gc, index.pc4, cfg.n_pc, 3 * gc_pc_projection_size };

    std::uniform_real_distribution<float> jitter(0.0f, cfg.w_gcpc_jitter);
    auto add_gc_pc = [&](int gc_begin, int pc_begin) {
        ConnectionDescription desc;
        const std::size_t total = static_cast<std::size_t>(cfg.n_gc) * static_cast<std::size_t>(cfg.n_pc);
        desc.SourceNeuron.reserve(total);
        desc.TargetNeuron.reserve(total);
        desc.Type.reserve(total);
        desc.Weight.reserve(total);
        desc.MaxWeight.reserve(total);
        desc.Delay.reserve(total);
        desc.SynapseRule.reserve(total);
        desc.TriggerRule.reserve(total);
        for (int target = 0; target < cfg.n_pc; ++target) {
            for (int source = 0; source < cfg.n_gc; ++source) {
                desc.SourceNeuron.push_back(gc_begin + source);
                desc.TargetNeuron.push_back(pc_begin + target);
                desc.Type.push_back(0);
                desc.Weight.push_back(cfg.w_gcpc_init + jitter(rng));
                desc.MaxWeight.push_back(cfg.w_gcpc_max);
                desc.Delay.push_back(1);
                desc.SynapseRule.push_back(-1);
                desc.TriggerRule.push_back(-1);
            }
        }
        FinalizeConnection(desc, connections);
    };

    add_gc_pc(index.gc1, index.pc1);
    add_gc_pc(index.gc1, index.pc2);
    add_gc_pc(index.gc2, index.pc3);
    add_gc_pc(index.gc2, index.pc4);

    AddDenseProjection(connections, index.gc1, cfg.n_gc, index.dcn1, cfg.n_dcn, 0, cfg.w_gc_dcn, cfg.w_gc_dcn);
    AddDenseProjection(connections, index.gc1, cfg.n_gc, index.dcn2, cfg.n_dcn, 0, cfg.w_gc_dcn, cfg.w_gc_dcn);
    AddDenseProjection(connections, index.gc2, cfg.n_gc, index.dcn3, cfg.n_dcn, 0, cfg.w_gc_dcn, cfg.w_gc_dcn);
    AddDenseProjection(connections, index.gc2, cfg.n_gc, index.dcn4, cfg.n_dcn, 0, cfg.w_gc_dcn, cfg.w_gc_dcn);

    AddOneToOneProjection(connections, index.pc1, index.dcn1, cfg.n_pc, 1, cfg.w_pc_dcn);
    AddOneToOneProjection(connections, index.pc2, index.dcn2, cfg.n_pc, 1, cfg.w_pc_dcn);
    AddOneToOneProjection(connections, index.pc3, index.dcn3, cfg.n_pc, 1, cfg.w_pc_dcn);
    AddOneToOneProjection(connections, index.pc4, index.dcn4, cfg.n_pc, 1, cfg.w_pc_dcn);

    return slices;
}

std::vector<StateSample> BuildDesiredTrajectory(const Config& cfg) {
    std::vector<StateSample> desired(static_cast<std::size_t>(cfg.sample_count + 1));
    const double total_ms = cfg.control_interval_ms * static_cast<double>(cfg.sample_count);

    for (int i = 0; i <= cfg.sample_count; ++i) {
        const double phase = (static_cast<double>(i) * cfg.control_interval_ms) / total_ms;
        const double x = 0.15 * std::cos(2.0 * kPi * phase) + 0.1;
        const double y = 0.15 * std::sin(2.0 * kPi * phase) + 0.4;
        const double c2 = Clamp(
            (x * x + y * y - cfg.link1 * cfg.link1 - cfg.link2 * cfg.link2) / (2.0 * cfg.link1 * cfg.link2),
            -1.0,
            1.0);
        const double q2 = std::acos(c2);
        const double phi = std::acos(
            Clamp(
                (cfg.link2 * cfg.link2 - cfg.link1 * cfg.link1 - x * x - y * y) /
                    (-2.0 * cfg.link1 * std::sqrt(x * x + y * y)),
                -1.0,
                1.0));
        const double q1 = std::atan2(y, x) - phi;
        desired[static_cast<std::size_t>(i)].q1 = q1;
        desired[static_cast<std::size_t>(i)].q2 = q2;
        desired[static_cast<std::size_t>(i)].x = x;
        desired[static_cast<std::size_t>(i)].y = y;
    }

    const double dt_s = cfg.control_interval_ms * 0.001;
    for (int i = 0; i < cfg.sample_count; ++i) {
        desired[static_cast<std::size_t>(i)].qd1 =
            (desired[static_cast<std::size_t>(i + 1)].q1 - desired[static_cast<std::size_t>(i)].q1) / dt_s;
        desired[static_cast<std::size_t>(i)].qd2 =
            (desired[static_cast<std::size_t>(i + 1)].q2 - desired[static_cast<std::size_t>(i)].q2) / dt_s;
    }
    desired.back().qd1 = desired[desired.size() - 2].qd1;
    desired.back().qd2 = desired[desired.size() - 2].qd2;

    return desired;
}

std::array<double, 2> HandPosition(const Config& cfg, double q1, double q2) {
    return {
        cfg.link1 * std::cos(q1) + cfg.link2 * std::cos(q1 + q2),
        cfg.link1 * std::sin(q1) + cfg.link2 * std::sin(q1 + q2)
    };
}

std::array<double, 2> JointAcceleration(
    const Config& cfg,
    double theta2,
    double d_theta1,
    double d_theta2,
    double tau1,
    double tau2) {
    const double d1 = cfg.link1 * 0.5;
    const double d2 = cfg.link2 * 0.5;
    const double i1 = (cfg.mass1 * cfg.link1 * cfg.link1) / 3.0;
    const double i2 = (cfg.mass2 * cfg.link2 * cfg.link2) / 12.0;
    const double c2 = std::cos(theta2);
    const double s2 = std::sin(theta2);

    const double m11 = i1 + i2 + 2.0 * cfg.mass2 * cfg.link1 * d2 * c2 + cfg.mass1 * d1 * d1 + cfg.mass2 * (d2 * d2 + cfg.link1 * cfg.link1);
    const double m12 = i2 + cfg.mass2 * cfg.link1 * d2 * c2 + cfg.mass2 * d2 * d2;
    const double m21 = m12;
    const double m22 = i2 + cfg.mass2 * d2 * d2;

    const double rhs1 = tau1 + 2.0 * cfg.mass2 * cfg.link1 * d2 * s2 * d_theta1 * d_theta2 + cfg.mass2 * cfg.link1 * d2 * s2 * d_theta2 * d_theta2;
    const double rhs2 = tau2 - cfg.mass2 * cfg.link1 * d2 * s2 * d_theta1 * d_theta1;

    const double det = m11 * m22 - m12 * m21;
    if (std::fabs(det) < 1.0e-12) {
        return { 0.0, 0.0 };
    }

    return {
        (rhs1 * m22 - rhs2 * m12) / det,
        (m11 * rhs2 - m21 * rhs1) / det
    };
}

int QuantizeIndex(double value, double min_value, double range, int bins) {
    const double step = range / static_cast<double>(bins);
    int index = static_cast<int>(std::ceil((value - min_value) / step));
    index = std::max(1, std::min(bins, index));
    return index - 1;
}

int GranuleIndexShoulder(const Config& cfg, const StateSample& desired, const StateSample& actual) {
    const int i1 = QuantizeIndex(RadToDeg(desired.q1), cfg.angle1_min_deg, cfg.angle1_range_deg, cfg.nn);
    const int i2 = QuantizeIndex(RadToDeg(actual.q1), cfg.angle1_min_deg, cfg.angle1_range_deg, cfg.nn);
    const int i3 = QuantizeIndex(RadToDeg(desired.qd1), cfg.velocity1_min_deg, cfg.velocity1_range_deg, cfg.nv);
    const int i4 = QuantizeIndex(RadToDeg(actual.qd1), cfg.velocity1_min_deg, cfg.velocity1_range_deg, cfg.nv);
    return ((i1 * cfg.nn + i2) * cfg.nv + i3) * cfg.nv + i4;
}

int GranuleIndexElbow(const Config& cfg, const StateSample& desired, const StateSample& actual) {
    const int i1 = QuantizeIndex(RadToDeg(desired.q2), cfg.angle2_min_deg, cfg.angle2_range_deg, cfg.nn);
    const int i2 = QuantizeIndex(RadToDeg(actual.q2), cfg.angle2_min_deg, cfg.angle2_range_deg, cfg.nn);
    const int i3 = QuantizeIndex(RadToDeg(desired.qd2), cfg.velocity2_min_deg, cfg.velocity2_range_deg, cfg.nv);
    const int i4 = QuantizeIndex(RadToDeg(actual.qd2), cfg.velocity2_min_deg, cfg.velocity2_range_deg, cfg.nv);
    return ((i1 * cfg.nn + i2) * cfg.nv + i3) * cfg.nv + i4;
}

double LinearSaturating(double scale, double value) {
    return value <= scale ? value / scale : 1.0;
}

std::vector<int> GenerateBinaryCfCounts(int seed, int n_cf, int kk, std::mt19937& rng) {
    std::vector<std::uint8_t> bitmap(static_cast<std::size_t>(n_cf * kk), 0);
    std::uniform_int_distribution<int> neuron_dist(0, n_cf - 1);
    std::uniform_int_distribution<int> time_dist(0, kk - 1);
    for (int i = 0; i < seed; ++i) {
        const int neuron = neuron_dist(rng);
        const int time = time_dist(rng);
        bitmap[static_cast<std::size_t>(neuron * kk + time)] = 1;
    }
    std::vector<int> counts(static_cast<std::size_t>(n_cf), 0);
    for (int neuron = 0; neuron < n_cf; ++neuron) {
        int total = 0;
        for (int t = 0; t < kk; ++t) {
            total += bitmap[static_cast<std::size_t>(neuron * kk + t)];
        }
        counts[static_cast<std::size_t>(neuron)] = total;
    }
    return counts;
}

CfWindowCounts GenerateCfCounts(
    const Config& cfg,
    const StateSample& desired,
    const StateSample& actual,
    std::mt19937& rng) {
    CfWindowCounts counts;
    counts.cf1.assign(cfg.n_cf, 0);
    counts.cf2.assign(cfg.n_cf, 0);
    counts.cf3.assign(cfg.n_cf, 0);
    counts.cf4.assign(cfg.n_cf, 0);

    const double sh1 = RadToDeg(desired.q1);
    const double sh2 = RadToDeg(actual.q1);
    const double el1 = RadToDeg(desired.q2);
    const double el2 = RadToDeg(actual.q2);
    const double shv1 = RadToDeg(desired.qd1);
    const double shv2 = RadToDeg(actual.qd1);
    const double elv1 = RadToDeg(desired.qd2);
    const double elv2 = RadToDeg(actual.qd2);

    double na_err1 = LinearSaturating(60.0, std::fabs(sh1 - sh2));
    double na_err2 = LinearSaturating(75.0, std::fabs(el1 - el2));
    double nv_err1 = LinearSaturating(400.0, std::fabs(shv1 - shv2));
    double nv_err2 = LinearSaturating(400.0, std::fabs(elv1 - elv2));
    if (sh1 - sh2 < 0.0) na_err1 = -na_err1;
    if (el1 - el2 < 0.0) na_err2 = -na_err2;
    if (shv1 - shv2 < 0.0) nv_err1 = -nv_err1;
    if (elv1 - elv2 < 0.0) nv_err2 = -nv_err2;

    const double err1 = cfg.cf_mix_position * na_err1 + (1.0 - cfg.cf_mix_position) * nv_err1;
    const double err2 = cfg.cf_mix_position * na_err2 + (1.0 - cfg.cf_mix_position) * nv_err2;
    const double se_co = cfg.spike_cf_max * static_cast<double>(cfg.n_cf) / static_cast<double>(cfg.sample_count);
    const int seed1 = std::abs(static_cast<int>(std::llround(se_co * err1)));
    const int seed2 = std::abs(static_cast<int>(std::llround(se_co * err2)));
    const int kk = static_cast<int>(std::llround(cfg.control_interval_ms / cfg.dt_ms));

    if (err1 > 0.0) {
        counts.cf1 = GenerateBinaryCfCounts(seed1, cfg.n_cf, kk, rng);
    } else {
        counts.cf2 = GenerateBinaryCfCounts(seed1, cfg.n_cf, kk, rng);
    }
    if (err2 > 0.0) {
        counts.cf3 = GenerateBinaryCfCounts(seed2, cfg.n_cf, kk, rng);
    } else {
        counts.cf4 = GenerateBinaryCfCounts(seed2, cfg.n_cf, kk, rng);
    }

    return counts;
}

void DrainSpikeBuffer(ArrayOutputSpikeDriver* driver, std::vector<std::pair<int, int> >& spikes) {
    spikes.clear();
    int* times = NULL;
    int* cells = NULL;
    const int count = driver->GetBufferedSpikes(times, cells);
    spikes.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        spikes.push_back(std::make_pair(times[i], cells[i]));
    }
    delete[] times;
    delete[] cells;
}

DcnWindowCounts CountDcnSpikes(const LayerIndex& index, int n_dcn, const std::vector<std::pair<int, int> >& spikes) {
    DcnWindowCounts counts;
    for (std::size_t i = 0; i < spikes.size(); ++i) {
        const int neuron = spikes[i].second;
        if (neuron >= index.dcn1 && neuron < index.dcn1 + n_dcn) {
            ++counts.dcn1;
        } else if (neuron >= index.dcn2 && neuron < index.dcn2 + n_dcn) {
            ++counts.dcn2;
        } else if (neuron >= index.dcn3 && neuron < index.dcn3 + n_dcn) {
            ++counts.dcn3;
        } else if (neuron >= index.dcn4 && neuron < index.dcn4 + n_dcn) {
            ++counts.dcn4;
        }
    }
    return counts;
}

PopulationWindowCounts CountPopulationSpikes(
    const LayerIndex& index,
    const Config& cfg,
    const std::vector<std::pair<int, int> >& spikes) {
    PopulationWindowCounts counts;
    for (std::size_t i = 0; i < spikes.size(); ++i) {
        const int neuron = spikes[i].second;
        if (neuron >= index.pc1 && neuron < index.pc1 + cfg.n_pc) {
            ++counts.pc1;
        } else if (neuron >= index.pc2 && neuron < index.pc2 + cfg.n_pc) {
            ++counts.pc2;
        } else if (neuron >= index.pc3 && neuron < index.pc3 + cfg.n_pc) {
            ++counts.pc3;
        } else if (neuron >= index.pc4 && neuron < index.pc4 + cfg.n_pc) {
            ++counts.pc4;
        } else if (neuron >= index.dcn1 && neuron < index.dcn1 + cfg.n_dcn) {
            ++counts.dcn1;
        } else if (neuron >= index.dcn2 && neuron < index.dcn2 + cfg.n_dcn) {
            ++counts.dcn2;
        } else if (neuron >= index.dcn3 && neuron < index.dcn3 + cfg.n_dcn) {
            ++counts.dcn3;
        } else if (neuron >= index.dcn4 && neuron < index.dcn4 + cfg.n_dcn) {
            ++counts.dcn4;
        }
    }
    return counts;
}

void WriteLabeledSpikes(
    std::ofstream& out,
    const LayerIndex& index,
    const Config& cfg,
    const std::vector<std::pair<int, int> >& spikes) {
    for (std::size_t i = 0; i < spikes.size(); ++i) {
        const int time = spikes[i].first;
        const int neuron = spikes[i].second;
        if (neuron >= index.pc1 && neuron < index.pc1 + cfg.n_pc) {
            out << time << "\tPC1\t" << (neuron - index.pc1) << '\n';
        } else if (neuron >= index.pc2 && neuron < index.pc2 + cfg.n_pc) {
            out << time << "\tPC2\t" << (neuron - index.pc2) << '\n';
        } else if (neuron >= index.pc3 && neuron < index.pc3 + cfg.n_pc) {
            out << time << "\tPC3\t" << (neuron - index.pc3) << '\n';
        } else if (neuron >= index.pc4 && neuron < index.pc4 + cfg.n_pc) {
            out << time << "\tPC4\t" << (neuron - index.pc4) << '\n';
        } else if (neuron >= index.dcn1 && neuron < index.dcn1 + cfg.n_dcn) {
            out << time << "\tDCN1\t" << (neuron - index.dcn1) << '\n';
        } else if (neuron >= index.dcn2 && neuron < index.dcn2 + cfg.n_dcn) {
            out << time << "\tDCN2\t" << (neuron - index.dcn2) << '\n';
        } else if (neuron >= index.dcn3 && neuron < index.dcn3 + cfg.n_dcn) {
            out << time << "\tDCN3\t" << (neuron - index.dcn3) << '\n';
        } else if (neuron >= index.dcn4 && neuron < index.dcn4 + cfg.n_dcn) {
            out << time << "\tDCN4\t" << (neuron - index.dcn4) << '\n';
        }
    }
}

WeightProjectionRuntime BuildProjectionRuntime(Network* network, const ProjectionSlice& slice) {
    WeightProjectionRuntime runtime;
    runtime.gc_begin = slice.gc_begin;
    runtime.gc_count = slice.gc_count;
    runtime.pc_begin = slice.pc_begin;
    runtime.pc_count = slice.pc_count;
    runtime.source_columns.assign(static_cast<std::size_t>(slice.gc_count), std::vector<Interconnections*>());

    for (int local_source = 0; local_source < slice.gc_count; ++local_source) {
        std::vector<Interconnections*>& column = runtime.source_columns[static_cast<std::size_t>(local_source)];
        column.reserve(static_cast<std::size_t>(slice.pc_count));
        for (int row = 0; row < slice.pc_count; ++row) {
            const int original_index = slice.original_connection_begin + row * slice.gc_count + local_source;
            Interconnections* connection = network->wordination[original_index];
            if (connection == NULL) {
                continue;
            }
            column.push_back(connection);
        }
    }

    return runtime;
}

void ApplyMatlabWeightUpdate(
    Network* network,
    const WeightProjectionRuntime& runtime,
    int gc_index,
    const std::vector<int>& cf_counts,
    float alpha,
    float beta,
    float min_weight,
    std::mt19937& rng) {
    if (gc_index < 0 || gc_index >= runtime.gc_count) {
        return;
    }
    const std::vector<Interconnections*>& column = runtime.source_columns[static_cast<std::size_t>(gc_index)];
    if (column.empty()) {
        return;
    }

    std::uniform_real_distribution<float> unit_rand(0.0f, 1.0f);
    for (std::size_t i = 0; i < column.size(); ++i) {
        Interconnections* connection = column[i];
        if (connection == NULL) {
            continue;
        }
        const int row = connection->TargetNeuron->Neuron_index - runtime.pc_begin;
        if (row < 0 || row >= runtime.pc_count) {
            continue;
        }
        connection->weight += beta * static_cast<float>(cf_counts[static_cast<std::size_t>(row)]);
        if (cf_counts[static_cast<std::size_t>(row)] == 0) {
            connection->weight += alpha * unit_rand(rng);
        }
        if (connection->weight > connection->maximum_weight) {
            connection->weight = connection->maximum_weight;
        } else if (connection->weight < min_weight) {
            connection->weight = min_weight;
        }
    }
}

void WriteStateHeader(std::ofstream& out) {
    out << "time_s\tq1\tq2\tqd1\tqd2\tqdes1\tqdes2\tqdd1\tqdd2\ttau1\ttau2\tx\ty\n";
}

void WriteStateRow(
    std::ofstream& out,
    double time_s,
    const StateSample& actual,
    const StateSample& desired,
    const std::array<double, 2>& qdd,
    double tau1,
    double tau2) {
    out
        << time_s << '\t'
        << actual.q1 << '\t' << actual.q2 << '\t'
        << actual.qd1 << '\t' << actual.qd2 << '\t'
        << desired.q1 << '\t' << desired.q2 << '\t'
        << qdd[0] << '\t' << qdd[1] << '\t'
        << tau1 << '\t' << tau2 << '\t'
        << actual.x << '\t' << actual.y << '\n';
}

void IntegrateAngleLikeMatlab(
    const Config& cfg,
    double dda1,
    double dda2,
    StateSample& actual) {
    const double dt_s = cfg.control_interval_ms * 0.001;
    const double old_qd1 = actual.qd1;
    const double old_qd2 = actual.qd2;
    actual.qd1 += dda1 * dt_s;
    actual.qd2 += dda2 * dt_s;
    actual.q1 += old_qd1 * dt_s;
    actual.q2 += old_qd2 * dt_s;
    actual.q1 = Clamp(actual.q1, DegToRad(-30.0), DegToRad(90.0));
    actual.q2 = Clamp(actual.q2, DegToRad(0.1), DegToRad(150.0));
}

void ReleaseConstructionMemory(
    std::list<NeuronLayerDescription>& neurons,
    std::list<ConnectionDescription>& connections,
    std::list<LearningRuleDescription>& learning_rules) {
    std::list<NeuronLayerDescription>().swap(neurons);
    std::list<ConnectionDescription>().swap(connections);
    std::list<LearningRuleDescription>().swap(learning_rules);
}

} // namespace

int main() {
    try {
        Config cfg;
        cfg.test_rounds = std::max(1, EnvIntOrDefault("TOTAL_MODEL_TEST_ROUNDS", cfg.test_rounds));

        const int steps_per_control = static_cast<int>(std::llround(cfg.control_interval_ms / cfg.dt_ms));
        const int total_steps = steps_per_control * cfg.sample_count;
        const int gc_period_steps = std::max(1, static_cast<int>(std::llround(4.0 / cfg.dt_ms)));

        const std::vector<StateSample> desired = BuildDesiredTrajectory(cfg);
        std::filesystem::create_directories(OutputRoot());

        std::list<NeuronLayerDescription> neurons;
        std::list<ConnectionDescription> connections;
        std::list<LearningRuleDescription> learning_rules;
        std::mt19937 init_rng(7u);
        const LayerIndex index = BuildNeuronLayers(cfg, neurons);
        const std::array<ProjectionSlice, 4> slices = BuildConnections(cfg, index, connections, init_rng);

        Simulation simulation(neurons, connections, learning_rules, total_steps, static_cast<float>(cfg.dt_ms), 1);
        ReleaseConstructionMemory(neurons, connections, learning_rules);

        const std::array<WeightProjectionRuntime, 4> runtimes = {
            BuildProjectionRuntime(simulation.network, slices[0]),
            BuildProjectionRuntime(simulation.network, slices[1]),
            BuildProjectionRuntime(simulation.network, slices[2]),
            BuildProjectionRuntime(simulation.network, slices[3])
        };

        for (int round = 0; round < cfg.test_rounds; ++round) {
            const std::string round_dir = (std::filesystem::path(OutputRoot()) / ("round_" + std::to_string(round))).string();
            const std::string spike_path = (std::filesystem::path(round_dir) / "spikes.txt").string();
            const std::string monitor_path = (std::filesystem::path(round_dir) / "monitor.txt").string();
            const std::string state_path = (std::filesystem::path(round_dir) / "joint_state.tsv").string();
            const std::string weight_path = (std::filesystem::path(round_dir) / "weights_final.txt").string();
            const std::string labeled_spike_path = (std::filesystem::path(round_dir) / "population_spikes.tsv").string();
            const std::string window_count_path = (std::filesystem::path(round_dir) / "window_counts.tsv").string();
            std::filesystem::create_directories(round_dir);

            simulation.AddFileOutputSpikeDriver(spike_path);
            simulation.AddFileOutputMonitorDriver(monitor_path);
            if (round > 0) {
                simulation.ResetForNextRound(true);
            } else {
                simulation.InitSimulation();
            }

            std::ofstream state_out(state_path);
            std::ofstream labeled_spike_out(labeled_spike_path);
            std::ofstream window_count_out(window_count_path);
            WriteStateHeader(state_out);
            labeled_spike_out << "time_step\tpopulation\tlocal_index\n";
            window_count_out << "sample\ttime_s\tgc1_index\tgc2_index\tgc_injections\tpc1\tpc2\tpc3\tpc4\tdcn1\tdcn2\tdcn3\tdcn4\ttau1\ttau2\tq1\tq2\tqd1\tqd2\n";

            std::vector<std::pair<int, int> > buffered_spikes;
            std::mt19937 rng(static_cast<unsigned int>(17 + round));

            StateSample actual = desired[1];
            actual.x = HandPosition(cfg, actual.q1, actual.q2)[0];
            actual.y = HandPosition(cfg, actual.q1, actual.q2)[1];
            int gc1_prev_index = GranuleIndexShoulder(cfg, desired[0], desired[0]);
            int gc2_prev_index = GranuleIndexElbow(cfg, desired[0], desired[0]);

            for (int sample = 2; sample < cfg.sample_count; ++sample) {
                const StateSample& desired_sample = desired[static_cast<std::size_t>(sample)];

                std::vector<int> event_times;
                std::vector<int> event_neurons;
                event_times.reserve(static_cast<std::size_t>(cfg.gc_repeats_per_control * 2));
                event_neurons.reserve(static_cast<std::size_t>(cfg.gc_repeats_per_control * 2));
                for (int rep = 0; rep < cfg.gc_repeats_per_control; ++rep) {
                    const int event_time = (sample - 2) * steps_per_control + rep * gc_period_steps + 1;
                    event_times.push_back(event_time);
                    event_neurons.push_back(index.gc1 + gc1_prev_index);
                    event_times.push_back(event_time);
                    event_neurons.push_back(index.gc2 + gc2_prev_index);
                }

                simulation.AddExternalSpikeActivity(event_times, event_neurons);
                simulation.RunSimulationStep(steps_per_control);
                DrainSpikeBuffer(simulation.output_spike_driver, buffered_spikes);
                WriteLabeledSpikes(labeled_spike_out, index, cfg, buffered_spikes);

                const DcnWindowCounts dcn_counts = CountDcnSpikes(index, cfg.n_dcn, buffered_spikes);
                const PopulationWindowCounts pop_counts = CountPopulationSpikes(index, cfg, buffered_spikes);
                const double tau1 = cfg.dcn_torque_gain_1 * static_cast<double>(dcn_counts.dcn1 - dcn_counts.dcn2);
                const double tau2 = cfg.dcn_torque_gain_2 * static_cast<double>(dcn_counts.dcn3 - dcn_counts.dcn4);
                const std::array<double, 2> qdd = JointAcceleration(cfg, actual.q2, actual.qd1, actual.qd2, tau1, tau2);

                IntegrateAngleLikeMatlab(cfg, qdd[0], qdd[1], actual);
                const std::array<double, 2> xy = HandPosition(cfg, actual.q1, actual.q2);
                actual.x = xy[0];
                actual.y = xy[1];

                const CfWindowCounts cf = GenerateCfCounts(cfg, desired_sample, actual, rng);
                ApplyMatlabWeightUpdate(simulation.network, runtimes[0], gc1_prev_index, cf.cf1, cfg.alpha, cfg.beta, cfg.w_gcpc_min, rng);
                ApplyMatlabWeightUpdate(simulation.network, runtimes[1], gc1_prev_index, cf.cf2, cfg.alpha, cfg.beta, cfg.w_gcpc_min, rng);
                ApplyMatlabWeightUpdate(simulation.network, runtimes[2], gc2_prev_index, cf.cf3, cfg.alpha, cfg.beta, cfg.w_gcpc_min, rng);
                ApplyMatlabWeightUpdate(simulation.network, runtimes[3], gc2_prev_index, cf.cf4, cfg.alpha, cfg.beta, cfg.w_gcpc_min, rng);

                gc1_prev_index = GranuleIndexShoulder(cfg, desired_sample, actual);
                gc2_prev_index = GranuleIndexElbow(cfg, desired_sample, actual);

                window_count_out
                    << sample << '\t'
                    << static_cast<double>(sample) * cfg.control_interval_ms * 0.001 << '\t'
                    << gc1_prev_index << '\t'
                    << gc2_prev_index << '\t'
                    << cfg.gc_repeats_per_control << '\t'
                    << pop_counts.pc1 << '\t'
                    << pop_counts.pc2 << '\t'
                    << pop_counts.pc3 << '\t'
                    << pop_counts.pc4 << '\t'
                    << pop_counts.dcn1 << '\t'
                    << pop_counts.dcn2 << '\t'
                    << pop_counts.dcn3 << '\t'
                    << pop_counts.dcn4 << '\t'
                    << tau1 << '\t'
                    << tau2 << '\t'
                    << actual.q1 << '\t'
                    << actual.q2 << '\t'
                    << actual.qd1 << '\t'
                    << actual.qd2 << '\n';

                WriteStateRow(
                    state_out,
                    static_cast<double>(sample) * cfg.control_interval_ms * 0.001,
                    actual,
                    desired_sample,
                    qdd,
                    tau1,
                    tau2);
            }

            //simulation.SaveWeightToFile(weight_path.c_str());
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "total_model_matlab_strict_repro exception: " << ex.what() << std::endl;
        return 1;
    }
}
