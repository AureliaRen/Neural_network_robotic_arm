/*
* 测试 Izhikevich 神经元模型的 AMPA / NMDA / GABA 输入
*/
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"

#include <array>
#include <chrono>
#include <iostream>
#include <list>
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

} // namespace

int main(int argc, char** argv) {
	const int simulation_steps = 10000;
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
	spike_input_layer.numberofneuron = 3;
	spike_input_layer.isMonitored = true;
	spike_input_layer.isOutput = false;
	neu_description.push_back(spike_input_layer);

	NeuronLayerDescription izh_layer;
	izh_layer.ModelName = model_name;
	izh_layer.numberofneuron = 1;
	izh_layer.isMonitored = false;
	izh_layer.isOutput = true;
	izh_layer.NeuronParameter["a"] = 0.02f;
	izh_layer.NeuronParameter["b"] = 0.2f;
	izh_layer.NeuronParameter["c"] = -65.0f;
	izh_layer.NeuronParameter["d"] = 8.0f;
	izh_layer.NeuronParameter["V_rest"] = -65.0f;
	izh_layer.NeuronParameter["V_reset"] = -65.0f;
	izh_layer.NeuronParameter["V_th"] = 30.0f;
	izh_layer.NeuronParameter["R"] = 1.0f;
	izh_layer.NeuronParameter["E_ampa"] = 0.0f;
	izh_layer.NeuronParameter["ampa_tau"] = 5.0f;
	izh_layer.NeuronParameter["E_gaba"] = -80.0f;
	izh_layer.NeuronParameter["gaba_tau"] = 10.0f;
	izh_layer.NeuronParameter["nmda_tau"] = 80.0f;
	izh_layer.NeuronParameter["random_mu"] = std::array<float, 6>{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	izh_layer.NeuronParameter["random_sigma"] = std::array<float, 6>{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	neu_description.push_back(izh_layer);

	NeuronLayerDescription current_input_layer;
	current_input_layer.ModelName = "InputCurrentNeuronModel";
	current_input_layer.numberofneuron = 1;
	current_input_layer.isMonitored = false;
	current_input_layer.isOutput = false;
	neu_description.push_back(current_input_layer);

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{0},
		std::vector<int>{3},
		std::vector<int>{0},
		std::vector<float>{6.0f},
		std::vector<float>{6.0f},
		std::vector<int>{1}
	));

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{1},
		std::vector<int>{3},
		std::vector<int>{1},
		std::vector<float>{5.0f},
		std::vector<float>{5.0f},
		std::vector<int>{1}
	));

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{2},
		std::vector<int>{3},
		std::vector<int>{2},
		std::vector<float>{4.5f},
		std::vector<float>{4.5f},
		std::vector<int>{1}
	));

	syn_description.push_back(MakeConnectionDescription(
		std::vector<int>{4},
		std::vector<int>{3},
		std::vector<int>{3},
		std::vector<float>{1.0f},
		std::vector<float>{1.0f},
		std::vector<int>{1}
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
	for (int t = 20; t < simulation_steps; t += 120) {
		spike_times.push_back(t);
		spike_neurons.push_back(0);
	}
	for (int t = 80; t < simulation_steps; t += 200) {
		spike_times.push_back(t);
		spike_neurons.push_back(2);
	}
	for (int t = 140; t < simulation_steps; t += 240) {
		spike_times.push_back(t);
		spike_neurons.push_back(1);
	}
	simulation->AddExternalSpikeActivity(spike_times, spike_neurons);

	std::vector<int> current_times{ 0 };
	std::vector<int> current_neurons{ 4 };
	std::vector<float> current_values{ 4.5f };
	simulation->AddExternalCurrentActivity(current_times, current_neurons, current_values);

	simulation->AddFileOutputSpikeDriver("izhikevich_ampa_nmda_gaba_spikes.txt");
	simulation->AddFileOutputMonitorDriver("izhikevich_ampa_nmda_gaba_monitor.txt");
	simulation->SetPrintSimulationTime(false);
	simulation->InitSimulation();

	auto start = std::chrono::high_resolution_clock::now();
	simulation->RunSimulationStep(simulation_steps);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> elapsed = end - start;

	std::cout << "Izhikevich AMPA/NMDA/GABA test (" << model_name << ") finished in " << elapsed.count() << " ms." << std::endl;

	delete simulation;
	return 0;
}
