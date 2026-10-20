/*
* 测试STDP功能
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
	//鍒涘缓绐佽Е鍓嶇缁忓厓
	NeuronLayerDescription LIFLayer1;
	LIFLayer1.ModelName = "TimeDrivenLIF_Exponential_Decay";
	LIFLayer1.numberofneuron = 1;
	LIFLayer1.isMonitored = true;
	LIFLayer1.isOutput = true;
	LIFLayer1.NeuronParameter["V_th"] = -55.0;
	neuDescription.push_back(LIFLayer1);
	//鍒涘缓绐佽Е鍚庣缁忓厓
	NeuronLayerDescription LIFLayer2;
	LIFLayer2.ModelName = "TimeDrivenLIF_Exponential_Decay";
    LIFLayer2.numberofneuron = 1;
	LIFLayer2.isMonitored = true;
	LIFLayer2.isOutput = true;
	LIFLayer2.NeuronParameter["V_th"] = -55.0;
    neuDescription.push_back(LIFLayer2);
	NeuronLayerDescription InputCurrentLayer1;
	InputCurrentLayer1.ModelName = "InputCurrentNeuronModel";
	InputCurrentLayer1.numberofneuron = 1;
	InputCurrentLayer1.isMonitored = false;
	InputCurrentLayer1.isOutput = false;
	neuDescription.push_back(InputCurrentLayer1);
	NeuronLayerDescription InputCurrentLayer2;
	InputCurrentLayer2.ModelName = "InputCurrentNeuronModel";
	InputCurrentLayer2.numberofneuron = 1;
	InputCurrentLayer2.isMonitored = false;
	InputCurrentLayer2.isOutput = false;
	neuDescription.push_back(InputCurrentLayer2);

	//鍒涘缓涓や釜LIF灞傜殑杩炴帴
	ConnectionDescription LIFpre2LIFpost;
	//存放源神经元与目标神经元的索引
	std::vector<int> source_neuron_list1;
	std::vector<int> target_neuron_list1;
	std::vector<int> Type1;
	std::vector<float> Weight1;
	std::vector<float> MaxWeight1;
	std::vector<int> Delay1;
	std::vector<int> learning_rule1;
	std::vector<int> trigger_rule1;
	
	source_neuron_list1.push_back(0);
	target_neuron_list1.push_back(1);
	Type1.push_back(0);
	Weight1.push_back(1.0);
	MaxWeight1.push_back(10.0);
	Delay1.push_back(5);
	learning_rule1.push_back(0);
    trigger_rule1.push_back(-1);

	//learning_rule1[0] = 0;
	LIFpre2LIFpost.SourceNeuron = source_neuron_list1;
	LIFpre2LIFpost.TargetNeuron = target_neuron_list1;
	LIFpre2LIFpost.Type = Type1;
	LIFpre2LIFpost.Weight = Weight1;
	LIFpre2LIFpost.MaxWeight = MaxWeight1;
	LIFpre2LIFpost.Delay = Delay1;
	LIFpre2LIFpost.SynapseRule = learning_rule1;
	LIFpre2LIFpost.TriggerRule = trigger_rule1;
	synDescription.push_back(LIFpre2LIFpost);

	//鍒涘缓鐢垫祦杈撳叆灞傚埌LIFpre灞傜殑杩炴帴
	ConnectionDescription Current2LIFpre;
	//存放源神经元与目标神经元的索引
	std::vector<int> source_neuron_list2;
	std::vector<int> target_neuron_list2;
	std::vector<int> Type2;
	std::vector<float> Weight2;
	std::vector<float> MaxWeight2;
	std::vector<int> Delay2;
	std::vector<int> learning_rule2;
	std::vector<int> trigger_rule2;

	source_neuron_list2.push_back(2);
	target_neuron_list2.push_back(0);
	Type2.push_back(3);
	Weight2.push_back(1.0);
	MaxWeight2.push_back(10.0);
	Delay2.push_back(5);
	learning_rule2.push_back(-1);
	trigger_rule2.push_back(-1);

	//learning_rule1[0] = 0;
	Current2LIFpre.SourceNeuron = source_neuron_list2;
	Current2LIFpre.TargetNeuron = target_neuron_list2;
	Current2LIFpre.Type = Type2;
	Current2LIFpre.Weight = Weight2;
	Current2LIFpre.MaxWeight = MaxWeight2;
	Current2LIFpre.Delay = Delay2;
	Current2LIFpre.SynapseRule = learning_rule2;
	Current2LIFpre.TriggerRule = trigger_rule2;
	synDescription.push_back(Current2LIFpre);

	//鍒涘缓鐢垫祦鍒癓IFpost灞傜殑杩炴帴
	ConnectionDescription Current2LIFpost;
	//存放源神经元与目标神经元的索引
	std::vector<int> source_neuron_list3;
	std::vector<int> target_neuron_list3;
	std::vector<int> Type3;
	std::vector<float> Weight3;
	std::vector<float> MaxWeight3;
	std::vector<int> Delay3;
	std::vector<int> learning_rule3;
	std::vector<int> trigger_rule3;

	source_neuron_list3.push_back(3);
	target_neuron_list3.push_back(1);
	Type3.push_back(3);
	Weight3.push_back(1.0);
	MaxWeight3.push_back(10.0);
	Delay3.push_back(5);
	learning_rule3.push_back(-1);
	trigger_rule3.push_back(-1);

	//learning_rule1[0] = 0;
	Current2LIFpost.SourceNeuron = source_neuron_list3;
	Current2LIFpost.TargetNeuron = target_neuron_list3;
	Current2LIFpost.Type = Type3;
	Current2LIFpost.Weight = Weight3;
	Current2LIFpost.MaxWeight = MaxWeight3;
	Current2LIFpost.Delay = Delay3;
	Current2LIFpost.SynapseRule = learning_rule3;
	Current2LIFpost.TriggerRule = trigger_rule3;
	synDescription.push_back(Current2LIFpost);

	//创建突触可塑性描述
	LearningRuleDescription STDP_Rule;
	STDP_Rule.RuleName = "STDP";
	learnDescription.push_back(STDP_Rule);

	//鍒涘缓Simulation瀵硅薄
	Simulation* mysim;
	mysim = new Simulation(neuDescription, synDescription, learnDescription, 1000, 0.1, 1);
	std::cout << "Simulation object created" << std::endl;

	//瀹氫箟澶栫晫鐢垫祦杈撳叆
	std::vector<int> input_current_time;
	std::vector<int> current_neuron_index;
	std::vector<float> current_value;
	input_current_time.push_back(0);
	current_neuron_index.push_back(2);
	current_value.push_back(22.5);
	input_current_time.push_back(80);
	current_neuron_index.push_back(3);
	current_value.push_back(22.5);

	//鎻掑叆鐢垫祦
	mysim->AddExternalCurrentActivity(input_current_time, current_neuron_index, current_value);
	std::cout << "External current activity added" << std::endl;
	//瀹氫箟杈撳嚭鏂囦欢
	mysim->AddFileOutputMonitorDriver("D:/computing_ref/Demo/Python_plot/output_monitor.txt");
	std::cout << "File Output monitor driver added" << std::endl;
	mysim->AddFileOutputSpikeDriver("D:/computing_ref/Demo/Python_plot/output_spike.txt");
	std::cout << "File Output spike driver added" << std::endl;
	mysim->AddFileOutputWeightDriver("D:/computing_ref/Demo/Python_plot/Weight/output_weight.txt", 2);
	std::cout << "File Output weight driver added" << std::endl;
	//初始化模拟
	mysim->InitSimulation();
	std::cout << "Simulation initialized" << std::endl;
	//开始模拟
	std::cout << "Simulation finished" << std::endl;
	delete mysim;
}
