/*
* 测试文件主程序
*/
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Network/inc/NetworkConstructStructure.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include <iostream>

#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <array>
#include <list>

int main() {
	//瀛樻斁缃戠粶绁炵粡鍏冨眰鎻忚堪
	std::list<NeuronLayerDescription> neuDescription;
	//瀛樻斁缃戠粶杩炴帴鎻忚堪
	std::list<ConnectionDescription> synDescription;
	//存放网络连接突触可塑性描述
	std::list<LearningRuleDescription> learnDescription;
	//创建Input层与LIF层
	NeuronLayerDescription InputSpikeLayer;
	InputSpikeLayer.ModelName = "InputSpikeNeuronModel";
	InputSpikeLayer.numberofneuron = 1;
	InputSpikeLayer.isMonitored = false;
	InputSpikeLayer.isOutput = false;
	neuDescription.push_back(InputSpikeLayer);
	NeuronLayerDescription LIFLayer1;
	LIFLayer1.ModelName = "TimeDrivenLIF_Exponential_double";
	LIFLayer1.numberofneuron = 6;
	LIFLayer1.isMonitored = false;
	LIFLayer1.isOutput = false;
	LIFLayer1.NeuronParameter["random_sigma"] = std::array<float, 4>{ 4.0, 0.0, 0.0, 0.0 };
	neuDescription.push_back(LIFLayer1);
	NeuronLayerDescription LIFLayer2;
	LIFLayer2.ModelName = "TimeDrivenLIF_Exponential_double";
	LIFLayer2.numberofneuron = 2;
	LIFLayer2.isMonitored = false;
	LIFLayer2.isOutput = false;
	LIFLayer2.NeuronParameter["random_sigma"] = std::array<float, 4>{ 4.0, 0.0, 0.0, 0.0};
	neuDescription.push_back(LIFLayer2);
	NeuronLayerDescription InputCurrentLayer;
	InputCurrentLayer.ModelName = "InputCurrentNeuronModel";
	InputCurrentLayer.numberofneuron = 1;
	InputCurrentLayer.isMonitored = false;
	InputCurrentLayer.isOutput = false;
	neuDescription.push_back(InputCurrentLayer);


	//鍒涘缓Spike灞備笌LIF灞傜殑杩炴帴
	ConnectionDescription Spike2LIF;
	//存放源神经元与目标神经元的索引
	std::vector<int> source_neuron_list1;
	std::vector<int> target_neuron_list1;
	std::vector<int> Type1;
	std::vector<float> Weight1;
	std::vector<float> MaxWeight1;
	std::vector<int> Delay1;
	std::vector<int> learning_rule1;
	std::vector<int> trigger_rule1;
	std::vector<ModelDescription> modelDescription1;
	ModelDescription temp{};
	for (int i = 0; i < 6; i++) {
		source_neuron_list1.push_back(0);
		target_neuron_list1.push_back(i + 1);
		Type1.push_back(0);
        Weight1.push_back(0.5);
		MaxWeight1.push_back(10.0);
        Delay1.push_back(5);
		learning_rule1.push_back(-1);
		trigger_rule1.push_back(-1);
		modelDescription1.push_back(temp);

	}
	//learning_rule1[0] = 0;
	Spike2LIF.SourceNeuron = source_neuron_list1;
	Spike2LIF.TargetNeuron = target_neuron_list1;
	Spike2LIF.Type = Type1;
	Spike2LIF.Weight = Weight1;
    Spike2LIF.MaxWeight = MaxWeight1;
	Spike2LIF.Delay = Delay1;
	Spike2LIF.SynapseRule = learning_rule1;
	Spike2LIF.TriggerRule = trigger_rule1;
	Spike2LIF.SynapseModel = modelDescription1;
	synDescription.push_back(Spike2LIF);

	//鍒涘缓LIF灞備笌LIF灞傜殑杩炴帴
    ConnectionDescription LIF2LIF;
    std::vector<int> source_neuron_list3;
    std::vector<int> target_neuron_list3;
    std::vector<int> Type3;
    std::vector<float> Weight3;
    std::vector<float> MaxWeight3;
    std::vector<int> Delay3;
    std::vector<int> learning_rule3;
	std::vector<int> trigger_rule3;
	std::vector<ModelDescription> modelDescription3;
	for (int i = 1; i < 3; i++) {
		for (int j = 8; j < 9; j++) {
			source_neuron_list3.push_back(i);
			target_neuron_list3.push_back(j);
			Type3.push_back(0);
			Weight3.push_back(0.5);
			MaxWeight3.push_back(10.0);
			Delay3.push_back(5);
			learning_rule3.push_back(0);
			trigger_rule3.push_back(-1);
            modelDescription3.push_back(temp);
		}
	}
    LIF2LIF.SourceNeuron = source_neuron_list3;
    LIF2LIF.TargetNeuron = target_neuron_list3;
    LIF2LIF.Type = Type3;
    LIF2LIF.Weight = Weight3;
    LIF2LIF.MaxWeight = MaxWeight3;
    LIF2LIF.Delay = Delay3;
    LIF2LIF.SynapseRule = learning_rule3;
	LIF2LIF.TriggerRule = trigger_rule3;
	LIF2LIF.SynapseModel = modelDescription3;
    synDescription.push_back(LIF2LIF);

	//鍒涘缓Current灞備笌LIF灞傜殑杩炴帴
	ConnectionDescription Current2LIF;
	std::vector<int> source_neuron_list2;
	std::vector<int> target_neuron_list2;
	std::vector<int> Type2;
	std::vector<float> Weight2;
	std::vector<float> MaxWeight2;
	std::vector<int> Delay2;
	std::vector<int> learning_rule2;
	std::vector<int> trigger_rule2;
	std::vector<ModelDescription> modelDescription2;
	for (int i = 0; i < 8; i++) {
		source_neuron_list2.push_back(9);
		target_neuron_list2.push_back(i + 1);
		Type2.push_back(3);
		Weight2.push_back(0.8);
		MaxWeight2.push_back(10.0);
		Delay2.push_back(5);
		learning_rule2.push_back(-1);
		trigger_rule2.push_back(-1);
        modelDescription2.push_back(temp);
	}
	Current2LIF.SourceNeuron = source_neuron_list2;
	Current2LIF.TargetNeuron = target_neuron_list2;
	Current2LIF.Type = Type2;
	Current2LIF.Weight = Weight2;
    Current2LIF.MaxWeight = MaxWeight2;
	Current2LIF.Delay = Delay2;
	Current2LIF.SynapseRule = learning_rule2;
    Current2LIF.TriggerRule = trigger_rule2;
	Current2LIF.SynapseModel = modelDescription2;
	synDescription.push_back(Current2LIF);
	//创建突触可塑性描述
	LearningRuleDescription STDP_Rule;
	STDP_Rule.RuleName = "STDP";
	learnDescription.push_back(STDP_Rule);
	

	

	//创建仿真对象
	Simulation* mysim;
	mysim = new Simulation(neuDescription, synDescription, learnDescription, 25000, 0.2, 1,EVENT_QUEUE_TIMING_WHEEL, 15);
	std::cout << "Simulation object created" << std::endl;

	//定义输入脉冲
	std::vector<int> input_spike_time;
	std::vector<int> spike_neuron_index;
	for (int time = 0; time < 25000; time += 500) {
		input_spike_time.push_back(time);
		spike_neuron_index.push_back(0);
	}
	//瀹氫箟澶栫晫鐢垫祦杈撳叆
	std::vector<int> input_current_time;
	std::vector<int> current_neuron_index;
	std::vector<float> current_value;
	input_current_time.push_back(0);
	current_neuron_index.push_back(9);
	current_value.push_back(4.8);
	mysim->SetPrintSimulationTime(true);
	//鎻掑叆鑴夊啿
	mysim->AddExternalSpikeActivity(input_spike_time, spike_neuron_index);
	std::cout << "External spike activity added" << std::endl;
	//鎻掑叆鐢垫祦
	mysim->AddExternalCurrentActivity(input_current_time, current_neuron_index, current_value);
	std::cout << "External current activity added" << std::endl;
	//定义输出文件
	mysim->AddFileOutputMonitorDriver("D:/Programing/My_Program/SNN_simulator/My_project_build/Pyplot/output_monitor.txt");
    std::cout << "File Output monitor driver added" << std::endl;
	mysim->AddFileOutputSpikeDriver("D:/Programing/My_Program/SNN_simulator/My_project_build/Pyplot/output_spike.txt");
    std::cout << "File Output spike driver added" << std::endl;
	//初始化模拟
	mysim->SetPrintSimulationTime(false);
	mysim->InitSimulation();
	std::cout << "Simulation initialized" << std::endl;
	const bool enable_realtime = false;
	auto start = std::chrono::high_resolution_clock::now();
	if (enable_realtime) {
		mysim->EnableRealtime(10, 0.1f, 0.85f, 0.98f, 0.99f); //璁剧疆瀹炴椂浠跨湡鍙傛暟
	}
	if (enable_realtime) {
		mysim->RunSimulationRealtime(2500);
	}
	else {
		mysim->RunSimulationStep(2500);
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> elapsed = end - start;
	std::cout << "Simulation finished in " << elapsed.count() << " ms." << std::endl;
	delete mysim;
	std::cout << "Simulation object deleted" << std::endl;
	return 0;
}
