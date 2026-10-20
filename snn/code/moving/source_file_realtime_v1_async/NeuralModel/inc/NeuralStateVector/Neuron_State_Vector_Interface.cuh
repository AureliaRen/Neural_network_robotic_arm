/*
* 文件名：Neuron_State_Vector_Interface.cuh
* 该文件定义了神经元状态向量的接口类，该接口用于与 GPU 上的状态向量进行同步交互，该类继承自 Neuron_State_Vector
*/

#ifndef NEURON_STATE_VECTOR_INTERFACE_CUH
#define NEURON_STATE_VECTOR_INTERFACE_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include "cuda_runtime.h"

class Neuron_State_Vector_Interface : public Neuron_State_Vector {
	public:

		//CPU 上的增量向量
		float* AuxStateCPU;

		//GPU 上的初始向量
        float* InitialStateGPU;

		//CPU 上的初始向量
		float* InitialStateCPU;

		//GPU 上的增量向量
		float* AuxStateGPU;

		//GPU 上的状态向量
		float* Vector_of_StateVariableGPU;

		//GPU 上的上次更新时间步
		int* LastUpdateGPU;

		//GPU 上的上次放电时间步
		int* LastSpikingGPU;

		//GPU 上的放电指示数组
		bool* InternalSpikeGPU;

		//CPU 上的放电指示数组
		bool* InternalSpikeCPU;

		cudaDeviceProp deviceProp;

		/*
		* 构造函数
		* @StateNumber：状态变量个数
		*/
		Neuron_State_Vector_Interface(int StateNumber);

		/*
		* 析构函数
		*/
		~Neuron_State_Vector_Interface();

		/*
		* GPU 向量初始化函数
		* @NeuronNumber：神经元数量
		* @StateVariable：状态向量
		* @N_AuxState：增量向量长度
		* @DeviceProp：GPU 设备属性
		*/
		void InitNeuronStateGPU(int NeuronNumber, float* StateVariable,float* sigma, int N_AuxState, cudaDeviceProp DeviceProp);

		/*
		* 获取 InternalSpikeCPU 向量
		*/
		bool* getInternalSpike();


};

#endif
