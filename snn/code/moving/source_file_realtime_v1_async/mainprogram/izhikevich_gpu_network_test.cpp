/*
* 测试多神经元 Izhikevich 网络中的 AMPA / NMDA / GABA 输入与反馈
*/
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

#include <array>
#include <chrono>
#include <iostream>
#include <list>
#include <string>
#include <vector>

namespace {

ConnectionDescription MakeConnectionDescription(
	const std::vector<int>& source,
	const std::vector<int>& target,
	const std::vector<int>& type,
	const std::vector<float>& weight,
	const std::vector<float>& max_weight,
	const std::vector<int>& delay)
{
	ConnectionDescription desc;
	desc.SourceNeuron = source;
	desc.TargetNeuron = target;
	desc.Type = type;
	desc.Weight = weight;
	desc.MaxWeight = max_weight;
	desc.Delay = delay;
	desc.SynapseRule.assign(source.size(), -1);
	desc.TriggerRule.assign(source.size(), -1);
	desc.SynapseModel.assign(source.size(), ModelDescription{});
	return desc;
}

NeuronLayerDescription MakeIzhLayer(const std::string& model_name, int number_of_neurons, bool monitored, bool output)
{
	NeuronLayerDescription layer;
	layer.ModelName = model_name;
	layer.numberofneuron = number_of_neurons;
	layer.isMonitored = monitored;
	layer.isOutput = output;
	layer.NeuronParameter["a"] = 0.02f;
	layer.NeuronParameter["b"] = 0.2f;
	layer.NeuronParameter["c"] = -65.0f;
	layer.NeuronParameter["d"] = 8.0f;
	layer.NeuronParameter["V_rest"] = -65.0f;
	layer.NeuronParameter["V_reset"] = -65.0f;
	layer.NeuronParameter["V_th"] = 30.0f;
	layer.NeuronParameter["R"] = 1.0f;
	layer.NeuronParameter["E_ampa"] = 0.0f;
	layer.NeuronParameter["ampa_tau"] = 5.0f;
	layer.NeuronParameter["E_gaba"] = -80.0f;
	layer.NeuronParameter["gaba_tau"] = 10.0f;
	layer.NeuronParameter["nmda_tau"] = 80.0f;
	layer.NeuronParameter["random_mu"] = std::array<float, 6>{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	layer.NeuronParameter["random_sigma"] = std::array<float, 6>{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	return layer;
}

} // namespace

int main(int argc, char** argv) {
	const int simulation_steps = 20000;
	const float base_timestep = 0.1f;
	const int queue_count = 1;
	std::string model_name = "TimeDrivenIzhikevic_I_Exponential_Decay";
#if !SNN_CPU_ONLY
	if (argc > 1 && std::string(argv[1]) == "gpu") {
		model_name = "TimeDrivenIzhikevic_I_Exponential_Decay_GPU";
	}
#endif

	std::list<NeuronLayerDescription> neu_description;
	std::list<ConnectionDescription> syn_description;
	std::list<LearningRuleDescription> learn_description;

	NeuronLayerDescription spike_input_layer;
	spike_input_layer.ModelName = "InputSpikeNeuronModel";
	spike_input_layer.numberofneuron = 6;
	spike_input_layer.isMonitored = false;
	spike_input_layer.isOutput = false;
	neu_description.push_back(spike_input_layer);

	neu_description.push_back(MakeIzhLayer(model_name, 4, false, false));
	neu_description.push_back(MakeIzhLayer(model_name, 2, false, true));

	NeuronLayerDescription current_input_layer;
	current_input_layer.ModelName = "InputCurrentNeuronModel";
	current_input_layer.numberofneuron = 2;
	current_input_layer.isMonitored = false;
	current_input_layer.isOutput = false;
	neu_description.push_back(current_input_layer);

	// 输入层编号: 0..5
	// 第一层 Izhikevich: 6..9
	// 第二层 Izhikevich: 10..11
	// 电流输入: 12..13

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{0, 1, 2, 3},
		std::vector<int>{6, 7, 8, 9},
		std::vector<int>{0, 0, 0, 0},
		std::vector<float>{5.5f, 5.5f, 5.0f, 5.0f},
		std::vector<float>{5.5f, 5.5f, 5.0f, 5.0f},
		std::vector<int>{1, 1, 1, 1}
	));

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{4, 5},
		std::vector<int>{8, 9},
		std::vector<int>{1, 1},
		std::vector<float>{4.0f, 4.5f},
		std::vector<float>{4.0f, 4.5f},
		std::vector<int>{1, 1}
	));

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{6, 7, 8, 9},
		std::vector<int>{10, 10, 11, 11},
		std::vector<int>{2, 0, 2, 0},
		std::vector<float>{3.5f, 4.0f, 3.5f, 4.0f},
		std::vector<float>{3.5f, 4.0f, 3.5f, 4.0f},
		std::vector<int>{2, 1, 2, 1}
	));

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{10, 11},
		std::vector<int>{7, 6},
		std::vector<int>{1, 1},
		std::vector<float>{4.0f, 4.0f},
		std::vector<float>{4.0f, 4.0f},
		std::vector<int>{1, 1}
	));

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{12, 13},
		std::vector<int>{6, 10},
		std::vector<int>{3, 3},
		std::vector<float>{3.5f, 3.0f},
		std::vector<float>{3.5f, 3.0f},
		std::vector<int>{1, 1}
	));

	Simulation* simulation = new Simulation(
		neu_description,
		syn_description,
		learn_description,
		simulation_steps,
		base_timestep,
		queue_count,
		EVENT_QUEUE_TIMING_WHEEL,
		64
	);

	std::vector<int> spike_times;
	std::vector<int> spike_neurons;
	for (int t = 20; t < simulation_steps; t += 100) {
		spike_times.push_back(t);
		spike_neurons.push_back(0);
	}
	for (int t = 40; t < simulation_steps; t += 120) {
		spike_times.push_back(t);
		spike_neurons.push_back(1);
	}
	for (int t = 60; t < simulation_steps; t += 160) {
		spike_times.push_back(t);
		spike_neurons.push_back(2);
	}
	for (int t = 80; t < simulation_steps; t += 180) {
		spike_times.push_back(t);
		spike_neurons.push_back(3);
	}
	for (int t = 130; t < simulation_steps; t += 220) {
		spike_times.push_back(t);
		spike_neurons.push_back(4);
	}
	for (int t = 170; t < simulation_steps; t += 260) {
		spike_times.push_back(t);
		spike_neurons.push_back(5);
	}
	simulation->AddExternalSpikeActivity(spike_times, spike_neurons);

	std::vector<int> current_times{ 0, 0 };
	std::vector<int> current_neurons{ 12, 13 };
	std::vector<float> current_values{ 4.0f, 3.0f };
	simulation->AddExternalCurrentActivity(current_times, current_neurons, current_values);

	simulation->AddFileOutputSpikeDriver("izhikevich_gpu_network_spikes.txt");
	simulation->AddFileOutputMonitorDriver("izhikevich_gpu_network_monitor.txt");
	simulation->SetPrintSimulationTime(false);
	simulation->InitSimulation();

	auto start = std::chrono::high_resolution_clock::now();
	simulation->RunSimulationStep(simulation_steps);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> elapsed = end - start;

	std::cout << "Izhikevich network test (" << model_name << ") finished in " << elapsed.count() << " ms." << std::endl;

	delete simulation;
	return 0;
}
