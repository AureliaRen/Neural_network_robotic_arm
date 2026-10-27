/*
* 文件名：Neuron.h
* 定义了Neuron类，用于表征网络中的具体一个神经元
*/

#ifndef NEURON_H
#define NEURON_H

class NeuronModel;
class Neuron_State_Vector;
class Interconnections;
#include "../source_file_realtime_v1_async/Neuron/inc/NeuronPropogationStructure.h"
#include <vector>

class Neuron {
	public:

		int Neuron_index; //神经元在全局队列中的索引

		int index_in_NeuronModel; //神经元在NeuronModel中的索引

		int Queue_index; //神经元所在队列的索引

        NeuronModel* neuron_model; //神经元所属的模型

        Neuron_State_Vector* neuron_state_vector; //神经元的状态向量，与NeuronModel中的一致

		int* Output_Synaps_Number; //每个目标队列的输出突触的个数

		Interconnections*** Output_Synaps; //每个目标队列的输出突触,二维指针数组

		NeuronPropogationStructure* PropogationStructure; //神经元传播结构

		bool IsMonitor = false; //是否监视神经元电位

		bool IsOutput = false; //是否输出神经元电位

		//std::vector<double> V_monitor; //存储神经元电位

		//std::vector<bool> spike_monitor; //存储神经元是否发放脉冲

		Interconnections*** PostSynapticLearning; //后突触学习连接

		Interconnections*** TriggerSynapticLearning; //前突触学习连接

		Interconnections*** TriggerAndPostSynapticLearning; //前后突触学习连接

		int* PostSynapticLearning_Number; //后突触学习连接个数

		int* TriggerSynapticLearning_Number; //前突触学习连接个数

		int* TriggerAndPostSynapticLearning_Number; //前后突触学习连接个数

		int*** IndexOfInputLearningIndex; //输入突触学习连接在SynapseState中的索引 (学习类型：后/前/前后)*(学习规则)*(学习规则连接个数)

		int NumberOfRule; //学习规则个数

		int* NumberOfTriggerConnectionPerRule; //每个学习规则的触发规则突触连接个数
		Interconnections*** TriggerConnectionPerRule; //触发规则突触连接

		bool TriggerConnectionOwned; //鏄惁宸茬粡鍒濆鍖栬Е鍙戣鍒欒繛鎺ユ墍鏈夋潈缁撴瀯

		Neuron();

		~Neuron();

		/*
		* 鐢ㄤ簬鍒濆鍖栫缁忓厓
		*/
		void InitNeuron(int NeuronIndex, NeuronModel* NeuronModel, int index_in_NeuronModel, int QueueIndex, bool monitor, bool output);

		/*
		* 鏍规嵁OutputSynapse璁＄畻浼犲嚭寤惰繜缁撴瀯
		*/
		void CaculateOutputDelayStructure();

		/*
		* 设置后突触学习规则
		* Connections：学习连接
		* ConnectionsNumPerRule：每个学习规则连接个数
		* NumberOfRules：学习规则个数
		*/
		void SetPostSynapticLearningRule(Interconnections*** Connections, int* ConnectionsNumPerRule, int NumberOfRules);

		/*
		* 设置前突触学习规则
		*/
		void SetTriggerSynapticLearningRule(Interconnections*** Connections, int* ConnectionsNumPerRule, int NumberOfRules);

		/*
		* 璁剧疆鍓嶅悗绐佽Е瀛︿範瑙勫垯
		*/
		void SetTriggerAndPostSynapticLearningRule(Interconnections*** Connections, int* ConnectionsNumPerRule, int NumberOfRules);

		/*
		* 初始化学习连接
		*/
		void InitLearningConnections();


	
};




#endif

