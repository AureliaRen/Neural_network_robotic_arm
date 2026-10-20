/*
* 文件名：TimeDrivenNeuralModelGPU.cuh
* 定义了GPU上时间驱动神经元的基类
*/
#ifndef TIME_DRIVEN_NEURAL_MODEL_GPU_CUH
#define TIME_DRIVEN_NEURAL_MODEL_GPU_CUH

#include <cuda_runtime.h>
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector_on_GPU.cuh"
#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodOnGPU.cuh"

class TimeDrivenNeuralModelGPU {
	public:

		//GPU上神经元状态向量
		Neuron_State_Vector_on_GPU* Neuron_State_Vector;

		//GPU涓婄殑鏃堕棿姝ラ暱鍊嶆暟
		int Timesteps;

		//指数电导常数表
		float* conductance_exp_values;

		//指数电导个数
		int N_conductance;

		//衰减最大步数
		int max_elapsed_times;

		//绉垎鏂规硶
		IntegrationMethodOnGPU* integration_method_on_GPU;

		/*
		* 构造函数
		*/
		__device__ TimeDrivenNeuralModelGPU(int Timesteps, float timestep) :Timesteps(Timesteps) {}

		/*
		* 析构函数
		*/
		__device__ ~TimeDrivenNeuralModelGPU() {
			delete this->Neuron_State_Vector;
			if (this->N_conductance > 0) {
				delete[] this->conductance_exp_values;
			}
		}

		/*
		* 虚函数，用于更新神经元状态
		*/
		__device__ virtual void UpdateState(int time, float timestep) {

		}

		/*
		* 虚函数，用于初始化神经元状态向量
		* Number_of_Neurons：神经元数量
		* AuxStateGPU：辅助状态向量
		* Neuron_State_Vector：神经元状态向量
		* LastUpdateGPU：上次更新时间
		* LastSpikeTimeGPU：上次发放脉冲时间
		* InternalSpikeGPU：内部发放脉冲标志
		* SizeStates：状态向量大小
		*/
		__device__ virtual void InitStateVector(int Number_of_States, float* initstate, float* AuxStateGPU, float* Neuron_State_Vector, int* LastUpdateGPU, int* LastSpikeTimeGPU, bool* InternalSpikeGPU, int NumberofNeuron) {
			this->Neuron_State_Vector = new Neuron_State_Vector_on_GPU(Number_of_States, initstate, AuxStateGPU, Neuron_State_Vector, LastUpdateGPU, LastSpikeTimeGPU, InternalSpikeGPU, NumberofNeuron);
		}

		/*
		* 鍒濆鍖朿onductance_exp_values鍚戦噺,鍒嗛厤鍐呭瓨
		*/
		__device__ void Initialize_conductance_exp_values(int N_conductances, int N_elapsed_times) {
			this->conductance_exp_values = new float[N_conductances * N_elapsed_times]();
			this->N_conductance = N_conductances;
			this->max_elapsed_times = N_elapsed_times;
		}

		/*
		* 设置conductance_exp_values向量中的值
		*/
		__device__ void Set_conductance_exp_values(int elapsed_time_index, int conductance_index, float value) {
			this->conductance_exp_values[elapsed_time_index * this->N_conductance + conductance_index] = value;
		}

		/*
		* 查表函数
		*/
		__device__ float Get_conductance_exponential_values(int elapsed_time_index, int conductance_index) {
			return this->conductance_exp_values[elapsed_time_index * this->N_conductance + conductance_index];
		}

		/*
		* 查表函数
		*/
		__device__ float* Get_conductance_exponential_values(int elapsed_time_index) {
			return this->conductance_exp_values + elapsed_time_index * this->N_conductance;
		}

		/*
		* 判断积分的有效性
		*/
		__device__ void CheckValidIntegeration(int index) {
			if (this->Neuron_State_Vector->Vector_of_State_VariableGPU[index] != this->Neuron_State_Vector->Vector_of_State_VariableGPU[index]) {
				printf("Error: Invalid integration method for neuron %d", index);
				this->Neuron_State_Vector->ResetState(index);
			}
		}

};

#endif
