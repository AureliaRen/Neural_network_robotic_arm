/*
* 文件名：Neuron_State_Vector_on_GPU.cuh
* 定义了 GPU 上的神经元状态向量
*/

#ifndef NEURON_STATE_VECTOR_ON_GPU_CUH
#define NEURON_STATE_VECTOR_ON_GPU_CUH

#include "../source_file_realtime_v1_async/error/cudaerror.h"

class Neuron_State_Vector_on_GPU {
	public:

		//每个神经元的状态变量个数
		int NumberofStateVariable;

		//增量向量
		float* AuxStateGPU;

		//初始状态向量
		float* InitState;
		
		//GPU 上的状态向量（与 Interface 中的 Vector_of_State_VariableGPU 指向同一块内存）
		float* Vector_of_State_VariableGPU;

		//上次更新时间步
		int* LastUpdateGPU;

		//上次放电时间步
		int* LastSpikeGPU;

		//放电指示数组
		bool* InternalSpikeGPU;

		//神经元数量
        int NumberofNeurons;

		/*
		* 构造函数（将 Neuron_State_Vector 同步到 GPU）
		*/
		__device__ Neuron_State_Vector_on_GPU(int NumberofStateVariable, float* initstate, float* AuxStateGPU, float* Vector_of_State_VariableGPU, int* LastUpdateGPU, int* LastSpikeGPU, bool* InternalSpikeGPU, int NumberofNeurons):
			NumberofStateVariable(NumberofStateVariable), AuxStateGPU(AuxStateGPU), Vector_of_State_VariableGPU(Vector_of_State_VariableGPU), LastUpdateGPU(LastUpdateGPU), LastSpikeGPU(LastSpikeGPU), InternalSpikeGPU(InternalSpikeGPU), NumberofNeurons(NumberofNeurons), InitState(initstate){}

		/*
		* 析构函数
		*/
		__device__ ~Neuron_State_Vector_on_GPU() {}

		/*
		* 重置状态
		*/
		__device__ void ResetState(int neuron_index) {
			for (int i = 0; i < this->NumberofStateVariable; i++) {
				this->Vector_of_State_VariableGPU[i * NumberofStateVariable + neuron_index] = this->InitState[i];
			}
		}


};

#endif // !NEURON_STATE_VECTOR_ON_GPU_CUH
