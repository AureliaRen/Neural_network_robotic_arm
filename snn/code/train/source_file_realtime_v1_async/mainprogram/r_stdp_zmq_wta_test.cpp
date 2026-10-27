/*
* R-STDP + ZMQ winner-take-all test.
*
* Neuron index layout:
*   0: fixed-rate InputSpike source
*   1: LIF candidate 0
*   2: LIF candidate 1, expected target winner
*   3: LIF candidate 2
*   4: reward InputSpike neuron, driven by the ZMQ server
*   5: punishment InputSpike neuron, driven by the ZMQ server
*
* ZMQ protocol matches ZMQInputOutputSpikeDriver and SpikeOutputServer:
*   client sends [uint32 time_step][uint32 output_count][OutputSpikeIO...]
*   server replies [uint32 input_count] and optional [OutputSpikeIO...]
*
* The server should count spikes from neurons 1, 2, and 3 during each
* communication window. If neuron 2 has the largest count, return a spike to
* neuron 4. Otherwise return a spike to neuron 5. This file does not connect to
* CoppeliaSim.
*/

#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/communication/inc/DriverType.h"

#include <array>
#include <chrono>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <vector>
#include <zmq.hpp>

namespace {

constexpr unsigned short ZMQ_PORT = 5565;
constexpr int INPUT_NEURON = 0;
constexpr int LIF0 = 1;
constexpr int LIF1_TARGET = 2;
constexpr int LIF2 = 3;
constexpr int REWARD_NEURON = 4;
constexpr int PUNISHMENT_NEURON = 5;
constexpr int R_STDP_RULE = 0;
constexpr unsigned int INPUT_WEIGHT_RANDOM_SEED = 20260421;

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

void AddInputToLifConnections(std::list<ConnectionDescription>& synDescription)
{
	std::mt19937 rng(INPUT_WEIGHT_RANDOM_SEED);
	std::uniform_real_distribution<float> inputWeightDistribution(0.7f, 1.3f);

	std::vector<int> sources;
	std::vector<int> targets;
	std::vector<int> types;
	std::vector<float> weights;
	std::vector<float> maxWeights;
	std::vector<int> delays;
	std::vector<int> synapseRules;
	std::vector<int> triggerRules;

	for (int lif = LIF0; lif <= LIF2; ++lif) {
		sources.push_back(INPUT_NEURON);
		targets.push_back(lif);
		types.push_back(0);
		weights.push_back(inputWeightDistribution(rng));
		maxWeights.push_back(8.0f);
		delays.push_back(1);
		synapseRules.push_back(R_STDP_RULE);
		triggerRules.push_back(-1);
	}

	synDescription.push_back(MakeConnections(
		sources, targets, types, weights, maxWeights, delays, synapseRules, triggerRules));
}

void AddWinnerTakeAllConnections(std::list<ConnectionDescription>& synDescription)
{
	std::vector<int> sources;
	std::vector<int> targets;
	std::vector<int> types;
	std::vector<float> weights;
	std::vector<float> maxWeights;
	std::vector<int> delays;
	std::vector<int> synapseRules;
	std::vector<int> triggerRules;

	for (int source = LIF0; source <= LIF2; ++source) {
		for (int target = LIF0; target <= LIF2; ++target) {
			if (source == target) {
				continue;
			}
			sources.push_back(source);
			targets.push_back(target);
			types.push_back(1);
			weights.push_back(5.0f);
			maxWeights.push_back(5.0f);
			delays.push_back(1);
			synapseRules.push_back(-1);
			triggerRules.push_back(-1);
		}
	}

	synDescription.push_back(MakeConnections(
		sources, targets, types, weights, maxWeights, delays, synapseRules, triggerRules));
}

void AddRewardPunishmentConnections(std::list<ConnectionDescription>& synDescription)
{
	std::vector<int> sources;
	std::vector<int> targets;
	std::vector<int> types;
	std::vector<float> weights;
	std::vector<float> maxWeights;
	std::vector<int> delays;
	std::vector<int> synapseRules;
	std::vector<int> triggerRules;

	for (int lif = LIF0; lif <= LIF2; ++lif) {
		sources.push_back(REWARD_NEURON);
		targets.push_back(lif);
		types.push_back(0);
		weights.push_back(0.0f);
		maxWeights.push_back(0.0f);
		delays.push_back(1);
		synapseRules.push_back(-1);
		triggerRules.push_back(R_STDP_RULE);

		sources.push_back(PUNISHMENT_NEURON);
		targets.push_back(lif);
		types.push_back(1);
		weights.push_back(0.0f);
		maxWeights.push_back(0.0f);
		delays.push_back(1);
		synapseRules.push_back(-1);
		triggerRules.push_back(R_STDP_RULE);
	}

	synDescription.push_back(MakeConnections(
		sources, targets, types, weights, maxWeights, delays, synapseRules, triggerRules));
}

} // namespace

int main()
{
	const int simulationSteps = 50000;
	const float baseTimestep = 0.1f;
	const int numberOfQueues = 1;
	const int communicationInterval = 50;

	std::list<NeuronLayerDescription> neuDescription;
	std::list<ConnectionDescription> synDescription;
	std::list<LearningRuleDescription> learnDescription;

	NeuronLayerDescription inputLayer;
	inputLayer.ModelName = "InputSpikeNeuronModel";
	inputLayer.numberofneuron = 1;
	inputLayer.isMonitored = true;
	inputLayer.isOutput = false;
	neuDescription.push_back(inputLayer);

	NeuronLayerDescription lifLayer;
	lifLayer.ModelName = "TimeDrivenLIF_Exponential_double";
	lifLayer.numberofneuron = 3;
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
	triggerLayer.numberofneuron = 2;
	triggerLayer.isMonitored = true;
	triggerLayer.isOutput = false;
	neuDescription.push_back(triggerLayer);

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

	AddInputToLifConnections(synDescription);
	AddWinnerTakeAllConnections(synDescription);
	AddRewardPunishmentConnections(synDescription);
	/*
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
	*/
	//*
	Simulation* simulation = new Simulation(
		neuDescription,
		synDescription,
		learnDescription,
		simulationSteps,
		baseTimestep,
		numberOfQueues
	);
	//*/

	std::vector<int> inputSpikeTimes;
	std::vector<int> inputSpikeNeurons;
	for (int t = 10; t < simulationSteps; t += 100) {
		inputSpikeTimes.push_back(t);
		inputSpikeNeurons.push_back(INPUT_NEURON);
	}
	simulation->AddExternalSpikeActivity(inputSpikeTimes, inputSpikeNeurons);

	simulation->AddZMQInputOutputSpikeDriver(CLIENT, "localhost", ZMQ_PORT, communicationInterval);
	simulation->AddFileOutputSpikeDriver("r_stdp_zmq_wta_spikes.txt");
	simulation->AddFileOutputMonitorDriver("r_stdp_zmq_wta_monitor.txt");

	std::cout << "R-STDP ZMQ WTA test configured." << std::endl;
	std::cout << "Expected selected neuron: " << LIF1_TARGET << std::endl;
	std::cout << "Reward neuron: " << REWARD_NEURON << ", punishment neuron: " << PUNISHMENT_NEURON << std::endl;
	std::cout << "Start the ZMQ server on port " << ZMQ_PORT << " before InitSimulation()." << std::endl;

	simulation->InitSimulation();
	simulation->SetPrintSimulationTime(false);
	auto start = std::chrono::high_resolution_clock::now();
	simulation->RunSimulationStep(simulationSteps);
	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float, std::milli> elapsed = end - start;
	std::cout << "Simulation finished in " << elapsed.count() << " ms." << std::endl;

	delete simulation;
	return 0;
}
