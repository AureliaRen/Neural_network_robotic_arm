/*
* 测试小脑学习规则的主程序
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

	//鍒涘缓绐佽Е鍓嶈緭鍏ョ缁忓厓
	NeuronLayerDescription PreInputSpikeLayer;
	PreInputSpikeLayer.ModelName = "InputSpikeNeuronModel";
	PreInputSpikeLayer.numberofneuron = 1;
	PreInputSpikeLayer.isMonitored = false;
	PreInputSpikeLayer.isOutput = true;
    neuDescription.push_back(PreInputSpikeLayer);

	//创建突触后LIF神经元
	NeuronLayerDescription PostLIFNeuron;
	PostLIFNeuron.ModelName = "TimeDrivenLIF_Voltage_jump";
	PostLIFNeuron.numberofneuron = 1;
	PostLIFNeuron.isMonitored = true;
	PostLIFNeuron.isOutput = true;
	neuDescription.push_back(PostLIFNeuron);

	//鍒涘缓绐佽Е鍚庤Е鍙戠缁忓厓
	NeuronLayerDescription PostTriggerNeuron;
	PostTriggerNeuron.ModelName = "InputSpikeNeuronModel";
	PostTriggerNeuron.numberofneuron = 1;
	PostTriggerNeuron.isMonitored = false;
	PostTriggerNeuron.isOutput = true;
	neuDescription.push_back(PostTriggerNeuron);

	//鍒涘缓杈撳叆鍒癓IF绁炵粡鍏冪殑杩炴帴
	ConnectionDescription Input2LIF;
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
	Delay1.push_back(1);
	learning_rule1.push_back(0);
	trigger_rule1.push_back(-1);

	Input2LIF.SourceNeuron = source_neuron_list1;
	Input2LIF.TargetNeuron = target_neuron_list1;
	Input2LIF.Type = Type1;
	Input2LIF.Weight = Weight1;
	Input2LIF.MaxWeight = MaxWeight1;
	Input2LIF.Delay = Delay1;
	Input2LIF.SynapseRule = learning_rule1;
	Input2LIF.TriggerRule = trigger_rule1;
	synDescription.push_back(Input2LIF);

	//创建触发神经元到LIF之间的连接
	ConnectionDescription Trigger2LIF;
	std::vector<int> source_neuron_list2;
	std::vector<int> target_neuron_list2;
	std::vector<int> Type2;
	std::vector<float> Weight2;
	std::vector<float> MaxWeight2;
	std::vector<int> Delay2;
	std::vector<int> learning_rule2;
	std::vector<int> trigger_rule2;

	source_neuron_list2.push_back(2);
	target_neuron_list2.push_back(1);
	Type2.push_back(0);
	Weight2.push_back(0.0);
	MaxWeight2.push_back(10.0);
	Delay2.push_back(1);
	learning_rule2.push_back(-1);
	trigger_rule2.push_back(0);

	Trigger2LIF.SourceNeuron = source_neuron_list2;
	Trigger2LIF.TargetNeuron = target_neuron_list2;
	Trigger2LIF.Type = Type2;
	Trigger2LIF.Weight = Weight2;
	Trigger2LIF.MaxWeight = MaxWeight2;
	Trigger2LIF.Delay = Delay2;
	Trigger2LIF.SynapseRule = learning_rule2;
	Trigger2LIF.TriggerRule = trigger_rule2;
	synDescription.push_back(Trigger2LIF);

	//创建突触可塑性描述
	LearningRuleDescription cerebullar_learning;
	cerebullar_learning.RuleName = "CerebullarLearningRule";
	cerebullar_learning.RuleParameter["fixweightchange"] = 0.2;
	cerebullar_learning.RuleParameter["kernalchange"] = -0.3;
	learnDescription.push_back(cerebullar_learning);

	//鍒涘缓Simulation瀵硅薄
	Simulation* mysim;
	mysim = new Simulation(neuDescription, synDescription, learnDescription, 1000, 0.1, 1);
	std::cout << "Simulation object created" << std::endl;

	std::vector<int> input_spike_time;
	std::vector<int> input_neuron_index;
	for (int i = 0; i < 1000; i += 100) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(0);
	}
	for (int i = 0; i < 1000; i += 300) {
		input_spike_time.push_back(i);
		input_neuron_index.push_back(2);
	}

	mysim->AddExternalSpikeActivity(input_spike_time, input_neuron_index);
	std::cout << "External spike activity added" << std::endl;

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
