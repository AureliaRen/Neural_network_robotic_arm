#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <list>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr int R_STDP_RULE = 0;

struct BenchConfig {
    EventQueueType queue_type = EVENT_QUEUE_TIMING_WHEEL;
    int repeat_count = 5;
    int simulation_steps = 40000;
    int input_count = 12;
    int candidate_count = 192;
    int synapses_per_pair = 2;
    int inhibition_fanout = 24;
    int burst_interval = 6;
    int burst_width = 2;
    int feedback_interval = 48;
    int wheel_size = 64;
    int queue_count = 1;
    float timestep_ms = 0.1f;
    unsigned int random_seed = 20260425u;
};

struct Stats {
    std::vector<double> run_ms;
    std::vector<double> init_ms;
    bench_profile::Snapshot total_snapshot{};
};

ConnectionDescription MakeConnections(
    const std::vector<int>& sources,
    const std::vector<int>& targets,
    const std::vector<int>& types,
    const std::vector<float>& weights,
    const std::vector<float>& max_weights,
    const std::vector<int>& delays,
    const std::vector<int>& synapse_rules,
    const std::vector<int>& trigger_rules)
{
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
    return desc;
}

bench_profile::Snapshot AddSnapshot(const bench_profile::Snapshot& a, const bench_profile::Snapshot& b) {
    bench_profile::Snapshot s{};
    s.event_remove_ns = a.event_remove_ns + b.event_remove_ns;
    s.event_buffer_insert_ns = a.event_buffer_insert_ns + b.event_buffer_insert_ns;
    s.event_buffer_flush_ns = a.event_buffer_flush_ns + b.event_buffer_flush_ns;
    s.wheel_insert_ns = a.wheel_insert_ns + b.wheel_insert_ns;
    s.wheel_remove_ns = a.wheel_remove_ns + b.wheel_remove_ns;
    s.wheel_syn_insert_ns = a.wheel_syn_insert_ns + b.wheel_syn_insert_ns;
    s.wheel_syn_remove_ns = a.wheel_syn_remove_ns + b.wheel_syn_remove_ns;
    s.wheel_first_event_ns = a.wheel_first_event_ns + b.wheel_first_event_ns;
    s.wheel_first_syn_event_ns = a.wheel_first_syn_event_ns + b.wheel_first_syn_event_ns;
    s.wheel_advance_queue_ns = a.wheel_advance_queue_ns + b.wheel_advance_queue_ns;
    s.wheel_advance_syn_ns = a.wheel_advance_syn_ns + b.wheel_advance_syn_ns;
    s.wheel_migrate_queue_ns = a.wheel_migrate_queue_ns + b.wheel_migrate_queue_ns;
    s.wheel_migrate_syn_ns = a.wheel_migrate_syn_ns + b.wheel_migrate_syn_ns;
    s.wheel_pop_ready_ns = a.wheel_pop_ready_ns + b.wheel_pop_ready_ns;
    s.sync_ns = a.sync_ns + b.sync_ns;
    s.time_event_total_ns = a.time_event_total_ns + b.time_event_total_ns;
    s.time_event_update_state_ns = a.time_event_update_state_ns + b.time_event_update_state_ns;
    s.time_event_internal_spike_ns = a.time_event_internal_spike_ns + b.time_event_internal_spike_ns;
    s.time_event_reschedule_ns = a.time_event_reschedule_ns + b.time_event_reschedule_ns;
    s.gpu_update_total_ns = a.gpu_update_total_ns + b.gpu_update_total_ns;
    s.gpu_update_memcpy_h2d_ns = a.gpu_update_memcpy_h2d_ns + b.gpu_update_memcpy_h2d_ns;
    s.gpu_update_kernel_ns = a.gpu_update_kernel_ns + b.gpu_update_kernel_ns;
    s.gpu_update_d2h_ns = a.gpu_update_d2h_ns + b.gpu_update_d2h_ns;
    s.gpu_update_sync_ns = a.gpu_update_sync_ns + b.gpu_update_sync_ns;
    s.gpu_update_memset_ns = a.gpu_update_memset_ns + b.gpu_update_memset_ns;
    s.gpu_update_event_record_ns = a.gpu_update_event_record_ns + b.gpu_update_event_record_ns;
    s.remove_count = a.remove_count + b.remove_count;
    s.time_event_count = a.time_event_count + b.time_event_count;
    s.gpu_update_count = a.gpu_update_count + b.gpu_update_count;
    s.sync_count = a.sync_count + b.sync_count;
    return s;
}

bench_profile::Snapshot DivideSnapshot(const bench_profile::Snapshot& a, long long n) {
    if (n <= 0) {
        return a;
    }
    bench_profile::Snapshot s{};
    s.event_remove_ns = a.event_remove_ns / n;
    s.event_buffer_insert_ns = a.event_buffer_insert_ns / n;
    s.event_buffer_flush_ns = a.event_buffer_flush_ns / n;
    s.wheel_insert_ns = a.wheel_insert_ns / n;
    s.wheel_remove_ns = a.wheel_remove_ns / n;
    s.wheel_syn_insert_ns = a.wheel_syn_insert_ns / n;
    s.wheel_syn_remove_ns = a.wheel_syn_remove_ns / n;
    s.wheel_first_event_ns = a.wheel_first_event_ns / n;
    s.wheel_first_syn_event_ns = a.wheel_first_syn_event_ns / n;
    s.wheel_advance_queue_ns = a.wheel_advance_queue_ns / n;
    s.wheel_advance_syn_ns = a.wheel_advance_syn_ns / n;
    s.wheel_migrate_queue_ns = a.wheel_migrate_queue_ns / n;
    s.wheel_migrate_syn_ns = a.wheel_migrate_syn_ns / n;
    s.wheel_pop_ready_ns = a.wheel_pop_ready_ns / n;
    s.sync_ns = a.sync_ns / n;
    s.time_event_total_ns = a.time_event_total_ns / n;
    s.time_event_update_state_ns = a.time_event_update_state_ns / n;
    s.time_event_internal_spike_ns = a.time_event_internal_spike_ns / n;
    s.time_event_reschedule_ns = a.time_event_reschedule_ns / n;
    s.gpu_update_total_ns = a.gpu_update_total_ns / n;
    s.gpu_update_memcpy_h2d_ns = a.gpu_update_memcpy_h2d_ns / n;
    s.gpu_update_kernel_ns = a.gpu_update_kernel_ns / n;
    s.gpu_update_d2h_ns = a.gpu_update_d2h_ns / n;
    s.gpu_update_sync_ns = a.gpu_update_sync_ns / n;
    s.gpu_update_memset_ns = a.gpu_update_memset_ns / n;
    s.gpu_update_event_record_ns = a.gpu_update_event_record_ns / n;
    s.remove_count = a.remove_count / n;
    s.time_event_count = a.time_event_count / n;
    s.gpu_update_count = a.gpu_update_count / n;
    s.sync_count = a.sync_count / n;
    return s;
}

double Average(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

void PrintStats(const char* name, const std::vector<double>& values) {
    if (values.empty()) {
        std::cout << name << ": no data" << std::endl;
        return;
    }
    const auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
    std::cout << name << ": avg=" << Average(values)
              << " ms, min=" << *min_it
              << " ms, max=" << *max_it << " ms" << std::endl;
}

BenchConfig ParseArgs(int argc, char** argv) {
    BenchConfig cfg;
    if (argc > 1) {
        const std::string queue_name = argv[1];
        if (queue_name == "heap") {
            cfg.queue_type = EVENT_QUEUE_HEAP;
        }
        else if (queue_name == "wheel") {
            cfg.queue_type = EVENT_QUEUE_TIMING_WHEEL;
        }
    }
    if (argc > 2) cfg.repeat_count = std::max(1, std::atoi(argv[2]));
    if (argc > 3) cfg.simulation_steps = std::max(100, std::atoi(argv[3]));
    if (argc > 4) cfg.input_count = std::max(1, std::atoi(argv[4]));
    if (argc > 5) cfg.candidate_count = std::max(8, std::atoi(argv[5]));
    if (argc > 6) cfg.synapses_per_pair = std::max(1, std::atoi(argv[6]));
    if (argc > 7) cfg.inhibition_fanout = std::max(1, std::atoi(argv[7]));
    if (argc > 8) cfg.burst_interval = std::max(1, std::atoi(argv[8]));
    if (argc > 9) cfg.burst_width = std::max(1, std::atoi(argv[9]));
    if (argc > 10) cfg.feedback_interval = std::max(2, std::atoi(argv[10]));
    if (argc > 11) cfg.wheel_size = std::max(8, std::atoi(argv[11]));
    if (argc > 12) cfg.queue_count = std::max(1, std::atoi(argv[12]));
    return cfg;
}

void BuildNetwork(
    const BenchConfig& cfg,
    std::list<NeuronLayerDescription>& neu_description,
    std::list<ConnectionDescription>& syn_description,
    std::list<LearningRuleDescription>& learn_description,
    int& input_start,
    int& candidate_start,
    int& reward_neuron,
    int& punishment_neuron)
{
    input_start = 0;
    candidate_start = cfg.input_count;
    reward_neuron = candidate_start + cfg.candidate_count;
    punishment_neuron = reward_neuron + 1;

    NeuronLayerDescription input_layer;
    input_layer.ModelName = "InputSpikeNeuronModel";
    input_layer.numberofneuron = cfg.input_count;
    input_layer.isMonitored = false;
    input_layer.isOutput = false;
    neu_description.push_back(input_layer);

    NeuronLayerDescription candidate_layer;
    candidate_layer.ModelName = "TimeDrivenLIF_Exponential_double";
    candidate_layer.numberofneuron = cfg.candidate_count;
    candidate_layer.isMonitored = false;
    candidate_layer.isOutput = false;
    candidate_layer.NeuronParameter["V_rest"] = -60.0f;
    candidate_layer.NeuronParameter["V_reset"] = -60.0f;
    candidate_layer.NeuronParameter["V_th"] = -45.0f;
    candidate_layer.NeuronParameter["tau"] = 12.0f;
    candidate_layer.NeuronParameter["R"] = 1.0f;
    candidate_layer.NeuronParameter["t_ref"] = 12;
    candidate_layer.NeuronParameter["gexc_tau"] = 5.0f;
    candidate_layer.NeuronParameter["ginh_tau"] = 8.0f;
    candidate_layer.NeuronParameter["Eexc"] = 0.0f;
    candidate_layer.NeuronParameter["Einh"] = -80.0f;
    candidate_layer.NeuronParameter["random_sigma"] = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
    neu_description.push_back(candidate_layer);

    NeuronLayerDescription trigger_layer;
    trigger_layer.ModelName = "InputSpikeNeuronModel";
    trigger_layer.numberofneuron = 2;
    trigger_layer.isMonitored = false;
    trigger_layer.isOutput = false;
    neu_description.push_back(trigger_layer);

    LearningRuleDescription rstdp_rule;
    rstdp_rule.RuleName = "R-STDP";
    rstdp_rule.RuleParameter["Max_LTP"] = 0.05f;
    rstdp_rule.RuleParameter["Max_LTD"] = 0.03f;
    rstdp_rule.RuleParameter["LTP_tau"] = 20.0f;
    rstdp_rule.RuleParameter["LTD_tau"] = 30.0f;
    rstdp_rule.RuleParameter["RewardFactor"] = 1.0f;
    rstdp_rule.RuleParameter["PunishmentFactor"] = -1.0f;
    rstdp_rule.RuleParameter["ClearEligibilityAfterTrigger"] = true;
    learn_description.push_back(rstdp_rule);

    std::mt19937 rng(cfg.random_seed);
    std::uniform_real_distribution<float> excit_weight(1.05f, 1.35f);

    {
        std::vector<int> sources;
        std::vector<int> targets;
        std::vector<int> types;
        std::vector<float> weights;
        std::vector<float> max_weights;
        std::vector<int> delays;
        std::vector<int> synapse_rules;
        std::vector<int> trigger_rules;

        sources.reserve(cfg.input_count * cfg.candidate_count * cfg.synapses_per_pair);
        targets.reserve(cfg.input_count * cfg.candidate_count * cfg.synapses_per_pair);

        for (int input = 0; input < cfg.input_count; ++input) {
            for (int candidate = 0; candidate < cfg.candidate_count; ++candidate) {
                for (int copy = 0; copy < cfg.synapses_per_pair; ++copy) {
                    sources.push_back(input_start + input);
                    targets.push_back(candidate_start + candidate);
                    types.push_back(0);
                    weights.push_back(excit_weight(rng));
                    max_weights.push_back(4.0f);
                    delays.push_back(1);
                    synapse_rules.push_back(R_STDP_RULE);
                    trigger_rules.push_back(-1);
                }
            }
        }

        syn_description.push_back(MakeConnections(
            sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules));
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

        const int fanout = std::min(cfg.inhibition_fanout, cfg.candidate_count - 1);
        sources.reserve(cfg.candidate_count * fanout);
        targets.reserve(cfg.candidate_count * fanout);

        for (int source = 0; source < cfg.candidate_count; ++source) {
            for (int offset = 1; offset <= fanout; ++offset) {
                const int target = (source + offset) % cfg.candidate_count;
                sources.push_back(candidate_start + source);
                targets.push_back(candidate_start + target);
                types.push_back(1);
                weights.push_back(3.5f);
                max_weights.push_back(3.5f);
                delays.push_back(1);
                synapse_rules.push_back(-1);
                trigger_rules.push_back(-1);
            }
        }

        syn_description.push_back(MakeConnections(
            sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules));
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

        sources.reserve(cfg.candidate_count * 2);
        targets.reserve(cfg.candidate_count * 2);

        for (int candidate = 0; candidate < cfg.candidate_count; ++candidate) {
            sources.push_back(reward_neuron);
            targets.push_back(candidate_start + candidate);
            types.push_back(0);
            weights.push_back(0.0f);
            max_weights.push_back(0.0f);
            delays.push_back(1);
            synapse_rules.push_back(-1);
            trigger_rules.push_back(R_STDP_RULE);

            sources.push_back(punishment_neuron);
            targets.push_back(candidate_start + candidate);
            types.push_back(1);
            weights.push_back(0.0f);
            max_weights.push_back(0.0f);
            delays.push_back(1);
            synapse_rules.push_back(-1);
            trigger_rules.push_back(R_STDP_RULE);
        }

        syn_description.push_back(MakeConnections(
            sources, targets, types, weights, max_weights, delays, synapse_rules, trigger_rules));
    }
}

void BuildExternalSpikes(
    const BenchConfig& cfg,
    int input_start,
    int reward_neuron,
    int punishment_neuron,
    std::vector<int>& spike_times,
    std::vector<int>& spike_neurons)
{
    for (int t = 10000; t < 10003; t += 1) {
        for (int width = cfg.simulation_steps; t + width < 10 * cfg.simulation_steps; ++width) {
            const int spike_time = t + width;
            for (int input = 0; input < cfg.input_count; ++input) {
                spike_times.push_back(spike_time);
                spike_neurons.push_back(input_start + input);
            }
        }
    }

    for (int t = cfg.feedback_interval; t < cfg.simulation_steps; t += cfg.feedback_interval) {
        spike_times.push_back(t);
        spike_neurons.push_back(reward_neuron);
        if (t + cfg.feedback_interval / 2 < cfg.simulation_steps) {
            spike_times.push_back(t + cfg.feedback_interval / 2);
            spike_neurons.push_back(punishment_neuron);
        }
    }
}

Stats RunBench(const BenchConfig& cfg) {
    Stats stats;

    std::list<NeuronLayerDescription> neu_description;
    std::list<ConnectionDescription> syn_description;
    std::list<LearningRuleDescription> learn_description;
    int input_start = 0;
    int candidate_start = 0;
    int reward_neuron = 0;
    int punishment_neuron = 0;

    BuildNetwork(
        cfg,
        neu_description,
        syn_description,
        learn_description,
        input_start,
        candidate_start,
        reward_neuron,
        punishment_neuron);

    std::vector<int> spike_times;
    std::vector<int> spike_neurons;
    BuildExternalSpikes(cfg, input_start, reward_neuron, punishment_neuron, spike_times, spike_neurons);

    std::cout << "queue_type=" << (cfg.queue_type == EVENT_QUEUE_TIMING_WHEEL ? "wheel" : "heap")
              << ", repeat_count=" << cfg.repeat_count
              << ", simulation_steps=" << cfg.simulation_steps
              << ", input_count=" << cfg.input_count
              << ", candidate_count=" << cfg.candidate_count
              << ", synapses_per_pair=" << cfg.synapses_per_pair
              << ", inhibition_fanout=" << cfg.inhibition_fanout
              << ", burst_interval=" << cfg.burst_interval
              << ", burst_width=" << cfg.burst_width
              << ", feedback_interval=" << cfg.feedback_interval
              << ", wheel_size=" << cfg.wheel_size
              << ", queue_count=" << cfg.queue_count
              << ", external_spikes=" << spike_times.size() << std::endl;

    for (int run = 1; run <= cfg.repeat_count; ++run) {
        auto total_start = std::chrono::high_resolution_clock::now();

        Simulation* sim = new Simulation(
            neu_description,
            syn_description,
            learn_description,
            cfg.simulation_steps,
            cfg.timestep_ms,
            cfg.queue_count);
            //cfg.queue_type,
            //cfg.wheel_size);

        auto init_start = std::chrono::high_resolution_clock::now();
        sim->AddExternalSpikeActivity(spike_times, spike_neurons);
        sim->InitSimulation();
        sim->SetPrintSimulationTime(false);
        auto init_end = std::chrono::high_resolution_clock::now();

        bench_profile::reset();
        auto run_start = std::chrono::high_resolution_clock::now();
        sim->RunSimulationStep(cfg.simulation_steps);
        const bench_profile::Snapshot snap = bench_profile::snapshot();
        auto run_end = std::chrono::high_resolution_clock::now();
        auto total_end = std::chrono::high_resolution_clock::now();

        const double init_ms = std::chrono::duration<double, std::milli>(init_end - init_start).count();
        const double run_ms = std::chrono::duration<double, std::milli>(run_end - run_start).count();
        const double total_ms = std::chrono::duration<double, std::milli>(total_end - total_start).count();

        stats.init_ms.push_back(init_ms);
        stats.run_ms.push_back(run_ms);
        stats.total_snapshot = AddSnapshot(stats.total_snapshot, snap);

        std::cout << "run " << run
                  << ": init=" << init_ms
                  << " ms, run=" << run_ms
                  << " ms, total=" << total_ms << " ms" << std::endl;
        bench_profile::print_breakdown(snap, run_ms);

        delete sim;
    }

    return stats;
}

}  // namespace

int main(int argc, char** argv) {
    const BenchConfig cfg = ParseArgs(argc, argv);
    const Stats stats = RunBench(cfg);
    const double avg_run = Average(stats.run_ms);
    const bench_profile::Snapshot avg_snapshot = DivideSnapshot(stats.total_snapshot, cfg.repeat_count);

    PrintStats("init", stats.init_ms);
    PrintStats("run", stats.run_ms);
    std::cout << "average_profiling_breakdown" << std::endl;
    bench_profile::print_breakdown(avg_snapshot, avg_run);
    return 0;
}
