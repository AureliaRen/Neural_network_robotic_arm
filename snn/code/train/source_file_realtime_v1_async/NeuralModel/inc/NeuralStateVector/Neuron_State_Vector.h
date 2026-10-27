/*
* 文件名：Neuron_State_Vector.h
* 定义了Neuron_Model中存储全体神经元状态向量的类
*/

#ifndef NEURON_STATE_VECTOR_H
#define NEURON_STATE_VECTOR_H

#include <iostream>
class Neuron_State_Vector {
	public:
		int NumberofStateVariable; //需要存储的状态变量个数

		int NumberofNeuron; //需要存储的神经元个数

		float* Vector_of_StateVariable; //存储状态变量的向量，大小为NumberofStateVariable*NumberofNeuron

		int* LastUpdate; //记录每个神经元上次更新的时间步，大小为NumberofNeuron

		int* LastSpike; //记录每个神经元上次发放脉冲到当前时间的时间步数，大小为NumberofNeuron

		int* PredictSpike; //预测每个神经元下次发放脉冲的时间步，大小为NumberofNeuron，用于事件神经元

		bool TimeDriven; //是否为时间驱动模型

		int* SpikeIndex; //记录当前放电神经元索引的数组

		int NumberofSpike; //当前放电神经元个数

		bool IsMonitored = false; //是否被监视

		bool IsGPU = false; //是否在GPU上运行

		float* init_Vector_of_StateVariable; //存储神经元初始状态的向量




		/*
		* 构造函数，接收状态变量个数和是否为时间驱动模型
		* StateNumber: 状态变量个数
		* isTimeDriven: 是否为时间驱动模型
		*/
		Neuron_State_Vector(int StateNumber, bool isTimeDriven);

		/*
		* 构造函数，接收状态变量个数和是否为时间驱动模型，以及是否在GPU上运行
		*/
		Neuron_State_Vector(int StateNumber, bool isTimeDriven, bool isGPU);

		/*
		* 析构函数
		*/
		~Neuron_State_Vector();


		/*
		* 初始化状态向量，将状态变量初始化为StateVariable
		*/
		void InitNeuronState(int NeuronNumber, float* StateVariable, float* sigma); 

        /*
		* 设置状态变量，将第NeuronIndex个神经元的状态变量StateVariableIndex设置为StateVariable
		*/
		void SetNeuronState(int NeuronIndex, int StateVariableIndex, float StateVariable);

		/*
		* 设置状态变量增量
		*/
		void SetNeuronStateIncrement(int NeuronIndex, int StateVariableIndex, float StateVariableIncrement);

		/*
		* 获取第index个神经元的状态变量
		*/
		float* GetNeuronState(int NeuronIndex);

		/*
		* 获取可写入的状态变量个数
		*/
		int GetNumberOfPrintableValues();

		/*
		* 重置神经元的状态
		* NeuronIndex: 需要重置的神经元索引
		*/
		void ResetNeuronState(int NeuronIndex);

		/*
		* 重置全部神经元状态以及相关运行时计数
		*/
		void ResetAllNeuronStates();

		/*
		* 获取可写入的状态变量
		*/
		float GetPrintableValuesAt(int index, int position);

		/*
		* 获取InternalSpike
		*/
		virtual bool* getInternalSpike() {
			return NULL;
		};


};


#endif
