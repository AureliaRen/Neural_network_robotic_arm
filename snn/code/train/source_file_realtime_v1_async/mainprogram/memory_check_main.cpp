/*
* 专用于内存检查的主程序，不修改原始 ZMQcommunication_test.cpp
*/

#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <array>
#include <list>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

int main() {
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::list<NeuronLayerDescription> neuDescription;
	std::list<ConnectionDescription> synDescription;
	std::list<LearningRuleDescription> learnDescription;

	std::cout << "memcheck: building network description" << std::endl;

	NeuronLayerDescription PreInputSpikeLayer;
	PreInputSpikeLayer.ModelName = "InputSpikeNeuronModel";
	PreInputSpikeLayer.numberofneuron = 2;
	PreInputSpikeLayer.isMonitored = true;
	PreInputSpikeLayer.isOutput = false;
	neuDescription.push_back(PreInputSpikeLayer);

	NeuronLayerDescription PostLIFNeuron;
	PostLIFNeuron.ModelName = "TimeDrivenLIF_Exponential_triple_GPU";
	PostLIFNeuron.numberofneuron = 2;
	PostLIFNeuron.isMonitored = true;
	PostLIFNeuron.isOutput = false;
	neuDescription.push_back(PostLIFNeuron);

	NeuronLayerDescription PostTriggerNeuron;
	PostTriggerNeuron.ModelName = "InputSpikeNeuronModel";
	PostTriggerNeuron.numberofneuron = 1;
	PostTriggerNeuron.isMonitored = false;
	PostTriggerNeuron.isOutput = false;
	neuDescription.push_back(PostTriggerNeuron);

	NeuronLayerDescription CurrentNeuron;
	CurrentNeuron.ModelName = "InputCurrentNeuronModel";
	CurrentNeuron.numberofneuron = 1;
	CurrentNeuron.isMonitored = false;
	CurrentNeuron.isOutput = false;
	neuDescription.push_back(CurrentNeuron);

	ConnectionDescription Input2LIF;
	std::vector<int> source_neuron_list1;
	std::vector<int> target_neuron_list1;
	std::vector<int> Type1;
	std::vector<float> Weight1;
	std::vector<float> MaxWeight1;
	std::vector<int> Delay1;
	std::vector<int> learning_rule1;
	std::vector<int> trigger_rule1;
	std::vector<ModelDescription> modelDescription;

	source_neuron_list1.push_back(0);
	source_neuron_list1.push_back(1);
	target_neuron_list1.push_back(2);
	target_neuron_list1.push_back(2);
	Type1.push_back(2);
	Type1.push_back(1);
	Weight1.push_back(1.0f);
	Weight1.push_back(1.0f);
	MaxWeight1.push_back(10.0f);
	MaxWeight1.push_back(10.0f);
	Delay1.push_back(1);
	Delay1.push_back(1);
	learning_rule1.push_back(0);
	learning_rule1.push_back(0);
	trigger_rule1.push_back(-1);
	trigger_rule1.push_back(-1);
	ModelDescription temp{};
	modelDescription.push_back(temp);
	modelDescription.push_back(temp);
	Input2LIF.SourceNeuron = source_neuron_list1;
	Input2LIF.TargetNeuron = target_neuron_list1;
	Input2LIF.Type = Type1;
	Input2LIF.Weight = Weight1;
	Input2LIF.MaxWeight = MaxWeight1;
	Input2LIF.Delay = Delay1;
	Input2LIF.SynapseRule = learning_rule1;
	Input2LIF.TriggerRule = trigger_rule1;
	Input2LIF.SynapseModel = modelDescription;
	synDescription.push_back(Input2LIF);

	ConnectionDescription Trigger2LIF;
	std::vector<int> source_neuron_list2;
	std::vector<int> target_neuron_list2;
	std::vector<int> Type2;
	std::vector<float> Weight2;
	std::vector<float> MaxWeight2;
	std::vector<int> Delay2;
	std::vector<int> learning_rule2;
	std::vector<int> trigger_rule2;
	std::vector<ModelDescription> modelDescription2;

	source_neuron_list2.push_back(4);
	target_neuron_list2.push_back(2);
	Type2.push_back(0);
	Weight2.push_back(0.0f);
	MaxWeight2.push_back(10.0f);
	Delay2.push_back(1);
	learning_rule2.push_back(-1);
	trigger_rule2.push_back(0);
	modelDescription2.push_back(temp);
	Trigger2LIF.SourceNeuron = source_neuron_list2;
	Trigger2LIF.TargetNeuron = target_neuron_list2;
	Trigger2LIF.Type = Type2;
	Trigger2LIF.Weight = Weight2;
	Trigger2LIF.MaxWeight = MaxWeight2;
	Trigger2LIF.Delay = Delay2;
	Trigger2LIF.SynapseRule = learning_rule2;
	Trigger2LIF.TriggerRule = trigger_rule2;
	Trigger2LIF.SynapseModel = modelDescription2;
	synDescription.push_back(Trigger2LIF);

	LearningRuleDescription cerebullar_learning;
	cerebullar_learning.RuleName = "CerebullarLearningRule";
	cerebullar_learning.RuleParameter["fixweightchange"] = 0.2f;
	cerebullar_learning.RuleParameter["kernalchange"] = -0.3f;
	learnDescription.push_back(cerebullar_learning);

	ConnectionDescription Current2LIF;
	std::vector<int> source_neuron_list3;
	std::vector<int> target_neuron_list3;
	std::vector<int> Type3;
	std::vector<float> Weight3;
	std::vector<float> MaxWeight3;
	std::vector<int> Delay3;
	std::vector<int> learning_rule3;
	std::vector<int> trigger_rule3;
	std::vector<ModelDescription> modelDescription3;

	source_neuron_list3.push_back(5);
	target_neuron_list3.push_back(3);
	Type3.push_back(3);
	Weight3.push_back(1.0f);
	MaxWeight3.push_back(10.0f);
	Delay3.push_back(1);
	learning_rule3.push_back(-1);
	trigger_rule3.push_back(-1);
	modelDescription3.push_back(temp);
	Current2LIF.SourceNeuron = source_neuron_list3;
	Current2LIF.TargetNeuron = target_neuron_list3;
	Current2LIF.Type = Type3;
	Current2LIF.Weight = Weight3;
	Current2LIF.MaxWeight = MaxWeight3;
	Current2LIF.Delay = Delay3;
	Current2LIF.SynapseRule = learning_rule3;
	Current2LIF.TriggerRule = trigger_rule3;
	Current2LIF.SynapseModel = modelDescription3;
	synDescription.push_back(Current2LIF);

	Simulation* mysim = nullptr;
	try {
		std::cout << "memcheck: creating Simulation" << std::endl;
		mysim = new Simulation(neuDescription, synDescription, learnDescription, 500, 0.2f, 2);
		std::cout << "Simulation object created" << std::endl;

	std::vector<int> input_spike_time;
	std::vector<int> input_neuron_index;
	for (int i = 15; i < 500; i += 150) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(0);
	}
	for (int i = 10; i < 500; i += 400) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(1);
	}
	for (int i = 0; i < 500; i += 250) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(4);
	}
		std::cout << "memcheck: adding spike input" << std::endl;
		mysim->AddExternalSpikeActivity(input_spike_time, input_neuron_index);

	std::vector<int> input_current_time;
	std::vector<int> current_neuron_index;
	std::vector<float> current_value;
	input_current_time.push_back(0);
	current_neuron_index.push_back(5);
	current_value.push_back(12.8f);
		std::cout << "memcheck: adding current input" << std::endl;
		mysim->AddExternalCurrentActivity(input_current_time, current_neuron_index, current_value);

		std::cout << "memcheck: attaching file outputs" << std::endl;
		mysim->AddFileOutputMonitorDriver("D:/LM_agent_test/codex/bench_output/output_monitor_memcheck.txt");
		mysim->AddFileOutputSpikeDriver("D:/LM_agent_test/codex/bench_output/output_spike_memcheck.txt");

		std::cout << "memcheck: initializing simulation" << std::endl;
		mysim->InitSimulation();
		auto start_time = std::chrono::high_resolution_clock::now();
		std::cout << "memcheck: running simulation" << std::endl;
		mysim->RunSimulationStep(500);
		auto end_time = std::chrono::high_resolution_clock::now();

		std::cout << "memcheck: deleting simulation" << std::endl;
		delete mysim;
		mysim = nullptr;

		std::chrono::duration<float, std::milli> duration_ms = end_time - start_time;
		std::cout << "Simulation finished" << std::endl;
		std::cout << "Elapsed: " << duration_ms.count() << " ms" << std::endl;
	}
	catch (const std::exception& ex) {
		std::cerr << "memcheck: caught std::exception: " << ex.what() << std::endl;
		delete mysim;
		return 2;
	}
	catch (...) {
		std::cerr << "memcheck: caught unknown exception" << std::endl;
		delete mysim;
		return 3;
	}
	return 0;
}
