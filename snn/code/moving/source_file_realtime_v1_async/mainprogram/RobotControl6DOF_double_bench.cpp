#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <iostream>
#include <thread>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <array>
#include <list>
#include <random>
#include <cstdlib>
#include <numeric>
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"

constexpr double PI = 3.14159265358979323846;

static void print_stats(const char* name, const std::vector<double>& values) {
    double sum = 0.0;
    double min_v = values.empty() ? 0.0 : values[0];
    double max_v = values.empty() ? 0.0 : values[0];
    for (double v : values) {
        sum += v;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }
    const double avg = values.empty() ? 0.0 : (sum / static_cast<double>(values.size()));
    std::cout << name << ": avg=" << avg << " ms, min=" << min_v << " ms, max=" << max_v << " ms" << std::endl;
}

static bench_profile::Snapshot add_snapshot(const bench_profile::Snapshot& a, const bench_profile::Snapshot& b) {
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

static bench_profile::Snapshot div_snapshot(const bench_profile::Snapshot& a, long long n) {
    if (n <= 0) return a;
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

int main(int argc, char** argv) {
    const int thread_count = (argc > 1) ? std::atoi(argv[1]) : 1;
    const int repeat_count = (argc > 2) ? std::atoi(argv[2]) : 3;
    const int simulation_steps = 10000;
    const double simulated_time_ms = simulation_steps * 0.1;
    std::vector<double> total_ms;
    std::vector<double> init_ms;
    std::vector<double> run_ms;
    bench_profile::Snapshot total_profile{};

    const int DOF = 4;
    const int MF_PER = 28;
    const int CF_PER = 100;
    const int GC_PER = 2401;
    const int PC_PER = 100;
    const int DCN_PER = 100;

    std::list<NeuronLayerDescription> neuDescription;
    std::list<ConnectionDescription> synDescription;
    std::list<LearningRuleDescription> learnDescription;

    for (int joint = 0; joint < DOF; ++joint) {
        NeuronLayerDescription MF;
        MF.ModelName = "InputSpikeNeuronModel";
        MF.numberofneuron = MF_PER;
        MF.isMonitored = false;
        MF.isOutput = false;
        neuDescription.push_back(MF);

        NeuronLayerDescription CF;
        CF.ModelName = "InputSpikeNeuronModel";
        CF.numberofneuron = CF_PER;
        CF.isMonitored = false;
        CF.isOutput = false;
        neuDescription.push_back(CF);

        NeuronLayerDescription GC;
        GC.ModelName = "TimeDrivenLIF_Exponential_double";
        GC.numberofneuron = GC_PER;
        GC.isMonitored = false;
        GC.isOutput = false;
        GC.NeuronParameter["tau"] = 2.0f;
        GC.NeuronParameter["V_rest"] = -65.0f;
        GC.NeuronParameter["V_reset"] = -65.0f;
        GC.NeuronParameter["Eexc"] = 0.0f;
        GC.NeuronParameter["gexc_tau"] = 1.0f;
        GC.NeuronParameter["V_th"] = -50.0f;
        GC.NeuronParameter["t_ref"] = 10;
        GC.NeuronParameter["random_sigma"] = std::array<float, 4>{ 1.5f, 0.0f, 0.0f, 0.0f };
        neuDescription.push_back(GC);

        NeuronLayerDescription PC;
        PC.ModelName = "TimeDrivenLIF_Exponential_double";
        PC.numberofneuron = PC_PER;
        PC.isMonitored = false;
        PC.isOutput = false;
        PC.NeuronParameter["tau"] = 100.0f / 6.0f;
        PC.NeuronParameter["V_rest"] = -70.0f;
        PC.NeuronParameter["V_reset"] = -70.0f;
        PC.NeuronParameter["Eexc"] = 0.0f;
        PC.NeuronParameter["gexc_tau"] = 1.2f;
        PC.NeuronParameter["V_th"] = -52.0f;
        PC.NeuronParameter["t_ref"] = 20;
        PC.NeuronParameter["random_sigma"] = std::array<float, 4>{ 5.0f, 0.0f, 0.0f, 0.0f };
        neuDescription.push_back(PC);

        NeuronLayerDescription DCN;
        DCN.ModelName = "TimeDrivenLIF_Exponential_double";
        DCN.numberofneuron = DCN_PER;
        DCN.isMonitored = false;
        DCN.isOutput = true;
        DCN.NeuronParameter["tau"] = 10.0f;
        DCN.NeuronParameter["V_rest"] = -70.0f;
        DCN.NeuronParameter["V_reset"] = -70.0f;
        DCN.NeuronParameter["Eexc"] = 0.0f;
        DCN.NeuronParameter["Einh"] = -80.0f;
        DCN.NeuronParameter["gexc_tau"] = 1.2f;
        DCN.NeuronParameter["ginh_tau"] = 10.0f;
        DCN.NeuronParameter["V_th"] = -40.0f;
        DCN.NeuronParameter["t_ref"] = 10;
        DCN.NeuronParameter["random_sigma"] = std::array<float, 4>{ 10.0f, 0.0f, 0.0f, 0.0f };
        neuDescription.push_back(DCN);
    }

    const int JOINT_STRIDE = MF_PER + CF_PER + GC_PER + PC_PER + DCN_PER;
    auto MF0 = [&](int joint) { return joint * JOINT_STRIDE + 0; };
    auto CF0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER; };
    auto GC0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER + CF_PER; };
    auto PC0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER + CF_PER + GC_PER; };
    auto DCN0 = [&](int joint) { return joint * JOINT_STRIDE + MF_PER + CF_PER + GC_PER + PC_PER; };

    const int MF_TOTAL = DOF * MF_PER;
    const int GC_TOTAL = DOF * GC_PER;
    const int PC_TOTAL = DOF * PC_PER;
    const int DCN_TOTAL = DOF * DCN_PER;

    for (int joint = 0; joint < DOF; ++joint) {
        const int mfStart = MF0(joint);
        const int cfStart = CF0(joint);
        const int gcStart = GC0(joint);
        const int pcStart = PC0(joint);
        const int dcnStart = DCN0(joint);

        {
            ConnectionDescription MF2GC;
            std::vector<int> source_neuron_list, target_neuron_list, Type, Delay, learning_rule, trigger_rule;
            std::vector<float> Weight, MaxWeight;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};
            int i = gcStart;
            for (int j1 = 0; j1 < 7; j1++) {
                for (int j2 = 7; j2 < 14; j2++) {
                    for (int j3 = 14; j3 < 21; j3++) {
                        for (int j4 = 21; j4 < 28; j4++) {
                            int pres[4] = { mfStart + j1, mfStart + j2, mfStart + j3, mfStart + j4 };
                            for (int k = 0; k < 4; ++k) {
                                source_neuron_list.push_back(pres[k]);
                                target_neuron_list.push_back(i);
                                Type.push_back(0);
                                Weight.push_back(0.18f);
                                MaxWeight.push_back(0.18f);
                                Delay.push_back(1);
                                learning_rule.push_back(-1);
                                trigger_rule.push_back(-1);
                                modelDescription.push_back(temp);
                            }
                            i++;
                        }
                    }
                }
            }
            MF2GC.SourceNeuron = source_neuron_list;
            MF2GC.TargetNeuron = target_neuron_list;
            MF2GC.Type = Type;
            MF2GC.Weight = Weight;
            MF2GC.MaxWeight = MaxWeight;
            MF2GC.Delay = Delay;
            MF2GC.SynapseRule = learning_rule;
            MF2GC.TriggerRule = trigger_rule;
            MF2GC.SynapseModel = modelDescription;
            synDescription.push_back(MF2GC);
        }

        {
            ConnectionDescription PC2DCN;
            std::vector<int> source_neuron_list, target_neuron_list, Type, Delay, learning_rule, trigger_rule;
            std::vector<float> Weight, MaxWeight;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};
            for (int k = 0; k < PC_PER; ++k) {
                source_neuron_list.push_back(pcStart + k);
                target_neuron_list.push_back(dcnStart + k);
                Type.push_back(1);
                Weight.push_back(1.0f);
                MaxWeight.push_back(1.0f);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }
            PC2DCN.SourceNeuron = source_neuron_list;
            PC2DCN.TargetNeuron = target_neuron_list;
            PC2DCN.Type = Type;
            PC2DCN.Weight = Weight;
            PC2DCN.MaxWeight = MaxWeight;
            PC2DCN.Delay = Delay;
            PC2DCN.SynapseRule = learning_rule;
            PC2DCN.TriggerRule = trigger_rule;
            PC2DCN.SynapseModel = modelDescription;
            synDescription.push_back(PC2DCN);
        }

        {
            ConnectionDescription CF2PC;
            std::vector<int> source_neuron_list, target_neuron_list, Type, Delay, learning_rule, trigger_rule;
            std::vector<float> Weight, MaxWeight;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};
            for (int k = 0; k < 100; ++k) {
                source_neuron_list.push_back(cfStart + k);
                target_neuron_list.push_back(pcStart + k);
                Type.push_back(0);
                Weight.push_back(0.0f);
                MaxWeight.push_back(0.0f);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(0);
                modelDescription.push_back(temp);
            }
            CF2PC.SourceNeuron = source_neuron_list;
            CF2PC.TargetNeuron = target_neuron_list;
            CF2PC.Type = Type;
            CF2PC.Weight = Weight;
            CF2PC.MaxWeight = MaxWeight;
            CF2PC.Delay = Delay;
            CF2PC.SynapseRule = learning_rule;
            CF2PC.TriggerRule = trigger_rule;
            CF2PC.SynapseModel = modelDescription;
            synDescription.push_back(CF2PC);
        }

        {
            ConnectionDescription CF2DCN_EXC;
            std::vector<int> source_neuron_list, target_neuron_list, Type, Delay, learning_rule, trigger_rule;
            std::vector<float> Weight, MaxWeight;
            std::vector<ModelDescription> modelDescription;
            ModelDescription temp{};
            for (int k = 0; k < CF_PER; ++k) {
                source_neuron_list.push_back(cfStart + k);
                target_neuron_list.push_back(dcnStart + k);
                Type.push_back(0);
                Weight.push_back(0.75f);
                MaxWeight.push_back(0.75f);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }
            CF2DCN_EXC.SourceNeuron = source_neuron_list;
            CF2DCN_EXC.TargetNeuron = target_neuron_list;
            CF2DCN_EXC.Type = Type;
            CF2DCN_EXC.Weight = Weight;
            CF2DCN_EXC.MaxWeight = MaxWeight;
            CF2DCN_EXC.Delay = Delay;
            CF2DCN_EXC.SynapseRule = learning_rule;
            CF2DCN_EXC.TriggerRule = trigger_rule;
            CF2DCN_EXC.SynapseModel = modelDescription;
            synDescription.push_back(CF2DCN_EXC);
        }
    }

    {
        ConnectionDescription MF2DCN;
        std::vector<int> source_neuron_list, target_neuron_list, Type, Delay, learning_rule, trigger_rule;
        std::vector<float> Weight, MaxWeight;
        std::vector<ModelDescription> modelDescription;
        ModelDescription temp{};

        std::vector<int> allMF;
        allMF.reserve(MF_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < MF_PER; ++k) {
                allMF.push_back(MF0(joint) + k);
            }
        }

        std::vector<int> allDCN;
        allDCN.reserve(DCN_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < DCN_PER; ++k) {
                allDCN.push_back(DCN0(joint) + k);
            }
        }

        for (int dcn : allDCN) {
            for (int mf : allMF) {
                source_neuron_list.push_back(mf);
                target_neuron_list.push_back(dcn);
                Type.push_back(0);
                Weight.push_back(0.1f);
                MaxWeight.push_back(0.1f);
                Delay.push_back(1);
                learning_rule.push_back(-1);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }
        }
        MF2DCN.SourceNeuron = source_neuron_list;
        MF2DCN.TargetNeuron = target_neuron_list;
        MF2DCN.Type = Type;
        MF2DCN.Weight = Weight;
        MF2DCN.MaxWeight = MaxWeight;
        MF2DCN.Delay = Delay;
        MF2DCN.SynapseRule = learning_rule;
        MF2DCN.TriggerRule = trigger_rule;
        MF2DCN.SynapseModel = modelDescription;
        synDescription.push_back(MF2DCN);
    }

    {
        ConnectionDescription GC2PC;
        std::vector<int> source_neuron_list, target_neuron_list, Type, Delay, learning_rule, trigger_rule;
        std::vector<float> Weight, MaxWeight;
        std::vector<ModelDescription> modelDescription;
        ModelDescription temp{};

        std::vector<int> allGC;
        allGC.reserve(GC_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < GC_PER; ++k) {
                allGC.push_back(GC0(joint) + k);
            }
        }

        std::vector<int> allPC;
        allPC.reserve(PC_TOTAL);
        for (int joint = 0; joint < DOF; ++joint) {
            for (int k = 0; k < PC_PER; ++k) {
                allPC.push_back(PC0(joint) + k);
            }
        }

        for (int pc : allPC) {
            for (int gc : allGC) {
                source_neuron_list.push_back(gc);
                target_neuron_list.push_back(pc);
                Type.push_back(0);
                Weight.push_back(1.6f);
                MaxWeight.push_back(5.0f);
                Delay.push_back(1);
                learning_rule.push_back(0);
                trigger_rule.push_back(-1);
                modelDescription.push_back(temp);
            }
        }
        GC2PC.SourceNeuron = source_neuron_list;
        GC2PC.TargetNeuron = target_neuron_list;
        GC2PC.Type = Type;
        GC2PC.Weight = Weight;
        GC2PC.MaxWeight = MaxWeight;
        GC2PC.Delay = Delay;
        GC2PC.SynapseRule = learning_rule;
        GC2PC.TriggerRule = trigger_rule;
        GC2PC.SynapseModel = modelDescription;
        synDescription.push_back(GC2PC);
    }

    LearningRuleDescription cerebullar_learning;
    cerebullar_learning.RuleName = "CerebullarLearningRule";
    cerebullar_learning.RuleParameter["fixweightchange"] = 0.002f;
    cerebullar_learning.RuleParameter["kernalchange"] = -0.0008f;
    learnDescription.push_back(cerebullar_learning);

    std::cout << "thread_count=" << thread_count << ", repeat_count=" << repeat_count
              << ", simulation_steps=" << simulation_steps << ", simulated_time_ms=" << simulated_time_ms << std::endl;
    for (int i = 1; i <= repeat_count; i++) {
        auto total_start = std::chrono::high_resolution_clock::now();
        Simulation* mysim = new Simulation(neuDescription, synDescription, learnDescription, simulation_steps, 0.1, 2, EVENT_QUEUE_TIMING_WHEEL, 10);
        std::cout << "Simulation " << i << " object created" << std::endl;

        auto init_start = std::chrono::high_resolution_clock::now();
        mysim->InitSimulation();
        auto init_end = std::chrono::high_resolution_clock::now();
        std::cout << "Simulation initialized" << std::endl;

        bench_profile::reset();
        auto run_start = std::chrono::high_resolution_clock::now();
        mysim->RunSimulationStep(simulation_steps);
        bench_profile::Snapshot run_profile = bench_profile::snapshot();
        auto run_end = std::chrono::high_resolution_clock::now();
        std::cout << i << " Simulation finished" << std::endl;

        auto total_end = std::chrono::high_resolution_clock::now();
        const double init_elapsed = std::chrono::duration<double, std::milli>(init_end - init_start).count();
        const double run_elapsed = std::chrono::duration<double, std::milli>(run_end - run_start).count();
        const double total_elapsed = std::chrono::duration<double, std::milli>(total_end - total_start).count();
        init_ms.push_back(init_elapsed);
        run_ms.push_back(run_elapsed);
        total_ms.push_back(total_elapsed);
        std::cout << "Run " << i << ": init=" << init_elapsed << " ms, run=" << run_elapsed << " ms, total=" << total_elapsed << " ms" << std::endl;
        bench_profile::print_breakdown(run_profile, run_elapsed);
        total_profile = add_snapshot(total_profile, run_profile);
        delete mysim;
    }

    print_stats("init", init_ms);
    print_stats("run", run_ms);
    print_stats("total", total_ms);
    const double avg_run = run_ms.empty() ? 0.0 : (std::accumulate(run_ms.begin(), run_ms.end(), 0.0) / static_cast<double>(run_ms.size()));
    std::cout << "realtime_ratio=" << (simulated_time_ms > 0.0 ? (avg_run / simulated_time_ms) : 0.0) << std::endl;
    std::cout << "realtime_capable=" << ((avg_run <= simulated_time_ms) ? "yes" : "no") << std::endl;
    bench_profile::Snapshot avg_profile = div_snapshot(total_profile, repeat_count);
    std::cout << "average_profiling_breakdown" << std::endl;
    bench_profile::print_breakdown(avg_profile, avg_run);
    return 0;
}
