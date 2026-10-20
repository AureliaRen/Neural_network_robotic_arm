/*
* 文件名：NeuronPropogationStructure.h
* 定义了NeuronPropogationStructure类，记录每个神经元的传出结构
*/

#ifndef NEURONPROPAGATIONSTRUCTURE_H
#define NEURONPROPAGATIONSTRUCTURE_H

class Interconnections;
class Neuron;
class NeuronModelPropogationStructure;

class NeuronPropogationStructure {
	public:

		int* NDifferentdelays; //每个目标线程不同延迟的个数

		Interconnections*** interconnections; //二维数组，数组内存放的是Interconnections类的指针

		int** NInterconnections; //每个目标线程,每个延迟的连接个数

		int** SynapseDelay; //每个目标线程,每个延迟的延迟数值

		int** SynapseDelayIndex; //每个目标线程,每个延迟的延迟数值在NeuronModel中SynapseDelay中的索引

		/*
		* 构造函数
		* neuron:传出的神经元指针
		*/
		NeuronPropogationStructure(Neuron* neuron);

        /*
		* 析构函数
		*/
		~NeuronPropogationStructure();


		/*
		* 计算每个延迟的延迟数值在NeuronModel中SynapseDelay中的索引,即计算SynapseDelayIndex
		* neuronModelPropogationStructure:NeuronModel的传出结构
		*/
        void CalculateSynapseDelayIndex(NeuronModelPropogationStructure* neuronModelPropogationStructure);


};

#endif
