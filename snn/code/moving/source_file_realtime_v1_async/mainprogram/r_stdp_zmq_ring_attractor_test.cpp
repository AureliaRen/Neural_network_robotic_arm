/*
* R-STDP + ZMQ ring-attractor test with compile-time configuration.
*
 * Layout:
 *   0..4: phase-switched InputSpike sources
 *   5..(5+N-1): ring 0 LIF neurons
 *   next N neurons: ring 1 LIF neurons
 *   next 10 neurons: reward InputSpike neurons, one per input-group/ring pair
 *   next 10 neurons: punishment InputSpike neurons, one per input-group/ring pair
*
* The LIF ring uses distance-dependent inhibitory recurrent connections:
*   nearby neurons inhibit weakly, far neurons inhibit strongly.
*
*/

#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/communication/inc/DriverType.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <list>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
// 输入的组数
constexpr int INPUT_GROUP_COUNT = 15;
// 吸引子环个数
constexpr int RING_COUNT = 2;
// 输入索引(0开始)
constexpr int INPUT_START = 0;
// LIF神经元的起始索引
constexpr int LIF_START = INPUT_START + INPUT_GROUP_COUNT;
// 连接的随机种子
constexpr unsigned int INPUT_WEIGHT_RANDOM_SEED = 20260425;
// 每个吸引子环的神经元数量
constexpr int CONFIG_LIF_COUNT_PER_RING = 16;
// 目标神经元序列
constexpr std::array<int, INPUT_GROUP_COUNT> RING0_TARGET_SEQUENCE_SLOTS_ONE_BASED = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
constexpr std::array<int, INPUT_GROUP_COUNT> RING1_TARGET_SEQUENCE_SLOTS_ONE_BASED = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
// 每个slot运行的时长
constexpr std::array<int, INPUT_GROUP_COUNT> INPUT_PHASE_STEPS = {5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000 };
constexpr unsigned short CONFIG_ZMQ_PORT = 5565;
// 交互的间隔
constexpr int CONFIG_COMMUNICATION_INTERVAL = 50;

/*
* 计算总步数
*/
constexpr int SumPhaseSteps()
{
    int total = 0;
    for (int phaseSteps : INPUT_PHASE_STEPS) {
        total += phaseSteps;
    }
    return total;
}
constexpr int CONFIG_SIMULATION_STEPS = SumPhaseSteps();

/*
* 计算第ringIndex个环的起始索引
*/
int RingStartNeuron(int ringIndex)
{
    return LIF_START + ringIndex * CONFIG_LIF_COUNT_PER_RING;
}

/*
* reward神经元的起始索引
*/
int RewardStartNeuron()
{
    return LIF_START + RING_COUNT * CONFIG_LIF_COUNT_PER_RING;
}

/*
* punishment神经元的起始索引
*/
int PunishmentStartNeuron()
{
    return RewardStartNeuron() + RING_COUNT * INPUT_GROUP_COUNT;
}

/*
* 学习规则的索引
*/
int RuleIndex(int inputGroup, int ringIndex)
{
    return inputGroup * RING_COUNT + ringIndex;
}

/*
* 奖惩神经元的全局索引
*/
int RewardNeuronIndex(int inputGroup, int ringIndex)
{
    return RewardStartNeuron() + ringIndex * INPUT_GROUP_COUNT + inputGroup;
}

int PunishmentNeuronIndex(int inputGroup, int ringIndex)
{
    return PunishmentStartNeuron() + ringIndex * INPUT_GROUP_COUNT + inputGroup;
}

/*
* 创建连接函数
*/
ConnectionDescription MakeConnections(
    const std::vector<int>& sources,
    const std::vector<int>& targets,
    const std::vector<int>& types,
    const std::vector<float>& weights,
    const std::vector<float>& maxWeights,
    const std::vector<int>& delays,
    const std::vector<int>& synapseRules,
    const std::vector<int>& triggerRules)
{
    ConnectionDescription desc;
    desc.SourceNeuron = sources;
    desc.TargetNeuron = targets;
    desc.Type = types;
    desc.Weight = weights;
    desc.MaxWeight = maxWeights;
    desc.Delay = delays;
    desc.SynapseRule = synapseRules;
    desc.TriggerRule = triggerRules;
    desc.SynapseModel.assign(sources.size(), ModelDescription{});
    return desc;
}

/*
* 计算吸引子环上的距离
*/
int CircularDistance(int a, int b, int count)
{
    const int rawDistance = std::abs(a - b);
    return std::min(rawDistance, count - rawDistance);
}

/*
* 计算环抑制权重
*/
float ComputeRingInhibitoryWeight(int sourceSlot, int targetSlot, int lifCount)
{
    const int distance = CircularDistance(sourceSlot, targetSlot, lifCount);
    const float maxDistance = static_cast<float>(std::max(1, lifCount / 2));
    const float normalizedDistance = static_cast<float>(distance) / maxDistance;
    const float nearWeight = 1.2f;
    const float farWeight = 5.0f;
    return nearWeight + (farWeight - nearWeight) * normalizedDistance * normalizedDistance;
}

/*
* 计算初始输入权重
*/
float ComputeInitialInputWeight(std::mt19937& rng)
{
    std::uniform_real_distribution<float> jitter(-0.35f, 0.35f);
    return 1.0f + jitter(rng);
}

/*
* 创建输入神经元到LIF的连接
*/
void AddInputToLifConnections(
    std::list<ConnectionDescription>& synDescription,
    int lifCount)
{
    std::mt19937 rng(INPUT_WEIGHT_RANDOM_SEED);

    std::vector<int> sources;
    std::vector<int> targets;
    std::vector<int> types;
    std::vector<float> weights;
    std::vector<float> maxWeights;
    std::vector<int> delays;
    std::vector<int> synapseRules;
    std::vector<int> triggerRules;
    // 按输入神经元循环
    for (int inputGroup = 0; inputGroup < INPUT_GROUP_COUNT; ++inputGroup) {
        // 按吸引子环循环
        for (int ringIndex = 0; ringIndex < RING_COUNT; ++ringIndex) {
            // 按LIF神经元循环
            for (int lifSlot = 0; lifSlot < lifCount; ++lifSlot) {
                sources.push_back(INPUT_START + inputGroup);
                targets.push_back(RingStartNeuron(ringIndex) + lifSlot);
                types.push_back(0);
                weights.push_back(ComputeInitialInputWeight(rng));
                maxWeights.push_back(8.0f);
                delays.push_back(1);
                synapseRules.push_back(RuleIndex(inputGroup, ringIndex));
                triggerRules.push_back(-1);
            }
        }
    }
    synDescription.push_back(MakeConnections(
        sources, targets, types, weights, maxWeights, delays, synapseRules, triggerRules));
}

/*
* 创建LIF到LIF的抑制连接
*/
void AddRingInhibitoryConnections(
    std::list<ConnectionDescription>& synDescription,
    int lifCount)
{
    std::vector<int> sources;
    std::vector<int> targets;
    std::vector<int> types;
    std::vector<float> weights;
    std::vector<float> maxWeights;
    std::vector<int> delays;
    std::vector<int> synapseRules;
    std::vector<int> triggerRules;

    for (int ringIndex = 0; ringIndex < RING_COUNT; ++ringIndex) {
        for (int sourceSlot = 0; sourceSlot < lifCount; ++sourceSlot) {
            for (int targetSlot = 0; targetSlot < lifCount; ++targetSlot) {
                if (sourceSlot == targetSlot) {
                    // 不抑制自己
                    continue;
                }
                // 添加抑制
                const float inhibitoryWeight = ComputeRingInhibitoryWeight(sourceSlot, targetSlot, lifCount);
                sources.push_back(RingStartNeuron(ringIndex) + sourceSlot);
                targets.push_back(RingStartNeuron(ringIndex) + targetSlot);
                types.push_back(1);
                weights.push_back(inhibitoryWeight);
                maxWeights.push_back(inhibitoryWeight);
                delays.push_back(1);
                // 不应用学习
                synapseRules.push_back(-1);
                triggerRules.push_back(-1);
            }
        }
    }

    synDescription.push_back(MakeConnections(
        sources, targets, types, weights, maxWeights, delays, synapseRules, triggerRules));
}

/*
* 创建奖励惩罚连接
*/
void AddRewardPunishmentConnections(
    std::list<ConnectionDescription>& synDescription,
    int lifCount)
{
    std::vector<int> sources;
    std::vector<int> targets;
    std::vector<int> types;
    std::vector<float> weights;
    std::vector<float> maxWeights;
    std::vector<int> delays;
    std::vector<int> synapseRules;
    std::vector<int> triggerRules;

    for (int inputGroup = 0; inputGroup < INPUT_GROUP_COUNT; ++inputGroup) {
        for (int ringIndex = 0; ringIndex < RING_COUNT; ++ringIndex) {
            const int rewardNeuron = RewardNeuronIndex(inputGroup, ringIndex);
            const int punishmentNeuron = PunishmentNeuronIndex(inputGroup, ringIndex);
            const int triggerRule = RuleIndex(inputGroup, ringIndex);

            for (int lifSlot = 0; lifSlot < lifCount; ++lifSlot) {
                const int lifNeuron = RingStartNeuron(ringIndex) + lifSlot;

                sources.push_back(rewardNeuron);
                targets.push_back(lifNeuron);
                types.push_back(0);
                weights.push_back(0.0f);
                maxWeights.push_back(0.0f);
                delays.push_back(1);
                synapseRules.push_back(-1);
                triggerRules.push_back(triggerRule);

                sources.push_back(punishmentNeuron);
                targets.push_back(lifNeuron);
                types.push_back(1);
                weights.push_back(0.0f);
                maxWeights.push_back(0.0f);
                delays.push_back(1);
                synapseRules.push_back(-1);
                triggerRules.push_back(triggerRule);
            }
        }
    }

    synDescription.push_back(MakeConnections(
        sources, targets, types, weights, maxWeights, delays, synapseRules, triggerRules));
}

} // namespace

int main()
{
    const int lifCountPerRing = CONFIG_LIF_COUNT_PER_RING;
    const int totalLifNeuronCount = RING_COUNT * lifCountPerRing;
    const unsigned short zmqPort = CONFIG_ZMQ_PORT;
    const int communicationInterval = CONFIG_COMMUNICATION_INTERVAL;
    const int simulationSteps = CONFIG_SIMULATION_STEPS;

    if (lifCountPerRing < 3) {
        throw std::invalid_argument("lif_count must be at least 3 for the ring-attractor test");
    }
    for (const int targetSlotOneBased : RING0_TARGET_SEQUENCE_SLOTS_ONE_BASED) {
        if (targetSlotOneBased < 1 || targetSlotOneBased > lifCountPerRing) {
            throw std::invalid_argument("RING0_TARGET_SEQUENCE_SLOTS_ONE_BASED values must be in [1, lif_count]");
        }
    }
    for (const int targetSlotOneBased : RING1_TARGET_SEQUENCE_SLOTS_ONE_BASED) {
        if (targetSlotOneBased < 1 || targetSlotOneBased > lifCountPerRing) {
            throw std::invalid_argument("RING1_TARGET_SEQUENCE_SLOTS_ONE_BASED values must be in [1, lif_count]");
        }
    }
    for (int phaseSteps : INPUT_PHASE_STEPS) {
        if (phaseSteps <= 0) {
            throw std::invalid_argument("INPUT_PHASE_STEPS values must be positive");
        }
    }
    if (communicationInterval <= 0) {
        throw std::invalid_argument("communication_interval must be positive");
    }
    if (simulationSteps <= 0) {
        throw std::invalid_argument("simulation_steps must be positive");
    }

    const float baseTimestep = 0.1f;
    const int numberOfQueues = 1;
    const int rewardStartNeuron = RewardStartNeuron();
    const int punishmentStartNeuron = PunishmentStartNeuron();

    std::list<NeuronLayerDescription> neuDescription;
    std::list<ConnectionDescription> synDescription;
    std::list<LearningRuleDescription> learnDescription;

    NeuronLayerDescription inputLayer;
    inputLayer.ModelName = "InputSpikeNeuronModel";
    inputLayer.numberofneuron = INPUT_GROUP_COUNT;
    inputLayer.isMonitored = false;
    inputLayer.isOutput = false;
    neuDescription.push_back(inputLayer);

    NeuronLayerDescription lifLayer;
    lifLayer.ModelName = "TimeDrivenLIF_Exponential_double";
    lifLayer.numberofneuron = totalLifNeuronCount;
    lifLayer.isMonitored = true;
    lifLayer.isOutput = true;
    lifLayer.NeuronParameter["V_rest"] = -60.0f;
    lifLayer.NeuronParameter["V_reset"] = -60.0f;
    lifLayer.NeuronParameter["V_th"] = -40.0f;
    lifLayer.NeuronParameter["tau"] = 15.0f;
    lifLayer.NeuronParameter["R"] = 1.0f;
    lifLayer.NeuronParameter["t_ref"] = 60;
    lifLayer.NeuronParameter["gexc_tau"] = 8.0f;
    lifLayer.NeuronParameter["ginh_tau"] = 15.0f;
    lifLayer.NeuronParameter["Eexc"] = 0.0f;
    lifLayer.NeuronParameter["Einh"] = -80.0f;
    lifLayer.NeuronParameter["random_sigma"] = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
    neuDescription.push_back(lifLayer);

    NeuronLayerDescription triggerLayer;
    triggerLayer.ModelName = "InputSpikeNeuronModel";
    triggerLayer.numberofneuron = RING_COUNT * INPUT_GROUP_COUNT * 2;
    triggerLayer.isMonitored = false;
    triggerLayer.isOutput = false;
    neuDescription.push_back(triggerLayer);

    for (int inputGroup = 0; inputGroup < INPUT_GROUP_COUNT; ++inputGroup) {
        for (int ringIndex = 0; ringIndex < RING_COUNT; ++ringIndex) {
            LearningRuleDescription rstdpRule;
            rstdpRule.RuleName = "R-STDP";
            rstdpRule.RuleParameter["Max_LTP"] = 0.08f;
            rstdpRule.RuleParameter["Max_LTD"] = 0.05f;
            rstdpRule.RuleParameter["LTP_tau"] = 20.0f;
            rstdpRule.RuleParameter["LTD_tau"] = 30.0f;
            rstdpRule.RuleParameter["RewardFactor"] = 1.0f;
            rstdpRule.RuleParameter["PunishmentFactor"] = -1.0f;
            rstdpRule.RuleParameter["ClearEligibilityAfterTrigger"] = true;
            learnDescription.push_back(rstdpRule);
        }
    }

    AddInputToLifConnections(synDescription, lifCountPerRing);
    AddRingInhibitoryConnections(synDescription, lifCountPerRing);
    AddRewardPunishmentConnections(synDescription, lifCountPerRing);

    Simulation* simulation = new Simulation(
        neuDescription,
        synDescription,
        learnDescription,
        simulationSteps,
        baseTimestep,
        numberOfQueues,
        EVENT_QUEUE_TIMING_WHEEL,
        128
    );

    std::vector<int> inputSpikeTimes;
    std::vector<int> inputSpikeNeurons;
    int phaseStart = 0;
    for (int inputGroup = 0; inputGroup < INPUT_GROUP_COUNT; ++inputGroup) {
        const int phaseEnd = std::min(simulationSteps, phaseStart + INPUT_PHASE_STEPS[inputGroup]);
        for (int t = phaseStart + 10; t < phaseEnd; t += 100) {
            inputSpikeTimes.push_back(t);
            inputSpikeNeurons.push_back(INPUT_START + inputGroup);
        }
        phaseStart = phaseEnd;
    }
    simulation->AddExternalSpikeActivity(inputSpikeTimes, inputSpikeNeurons);

    simulation->AddZMQInputOutputSpikeDriver(CLIENT, "localhost", zmqPort, communicationInterval);
    simulation->AddFileOutputSpikeDriver("r_stdp_zmq_ring_attractor_spikes.txt");
    simulation->AddFileOutputMonitorDriver("r_stdp_zmq_ring_attractor_monitor.txt");
    simulation->AddFileOutputWeightDriver("r_stdp_zmq_ring_attractor_output_weight.txt", 1000);

    std::cout << "R-STDP ZMQ ring-attractor test configured." << std::endl;
    std::cout << "rings=" << RING_COUNT
              << " lif_count_per_ring=" << lifCountPerRing
              << " total_lif_neurons=" << totalLifNeuronCount
              << " input_groups=" << INPUT_GROUP_COUNT
              << " learning_rules=" << (INPUT_GROUP_COUNT * RING_COUNT)
              << " reward_start_neuron=" << rewardStartNeuron
              << " punishment_start_neuron=" << punishmentStartNeuron << std::endl;
    std::cout << "port=" << zmqPort
        << " communication_interval=" << communicationInterval
        << " simulation_steps=" << simulationSteps << std::endl;
    
    std::cout << "Start the matching ring server before InitSimulation()." << std::endl;
    simulation->InitSimulation();
    simulation->SetPrintSimulationTime(false);
    simulation->LoadWeight("r_stdp_zmq_ring_attractor_output_weight750000.txt");
    const auto start = std::chrono::high_resolution_clock::now();
    simulation->RunSimulationStep(simulationSteps);
    const auto end = std::chrono::high_resolution_clock::now();

    const std::chrono::duration<float, std::milli> elapsed = end - start;
    std::cout << "Simulation finished in " << elapsed.count() << " ms." << std::endl;

    delete simulation;
    return 0;
}
