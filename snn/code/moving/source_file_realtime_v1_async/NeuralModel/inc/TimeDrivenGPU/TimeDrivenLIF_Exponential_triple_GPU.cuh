/*
* 文件名: TimeDrivenLIF_Exponential_triple_GPU.cuh
* 
*/

#ifndef TIME_DRIVEN_LIF_EXPONENTIAL_TRIPLE_GPU_CUH
#define TIME_DRIVEN_LIF_EXPONENTIAL_TRIPLE_GPU_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuralModelGPU.cuh"
#include "../source_file_realtime_v1_async/ModelFactory/Integration_method_GPU_Factory.cuh"
#include <iostream>
#include <array>

class TimeDrivenLIF_Exponential_triple_GPU : public TimeDrivenNeuralModelGPU {
public:
	bool I_EXT = true;

	bool AMPA = false;

	bool NMDA = false;

	bool GABA = false;

	float V_rest = -60.;

	float tau = 20.;

	float V_th = -50.;

	float R = 1.;

	float V_reset = -60.;

	int t_ref = 50;

	float E_ampa = 0.;

	float ampa_tau = 10.;

	float E_gaba = -80.;

	float gaba_tau = 10.;

	float nmda_tau = 10.;

	const int N_NeuronStateVariables = 5;

	const int N_TimedependentInput = 4; //时变输入变量个数：ampa, gaba, nmda, I_ext

	const int N_DifferentialStates = 1;

	const int index_V = 0;

	const int TimeDependentInputSize = 4;

	const int index_ampa = 1;

	const int index_nmda = 3;

	const int index_gaba = 2;

	const int I_EXT_index = 4;


	__device__ virtual void SetEnabledSynapsis(bool new_ampa, bool new_gaba, bool new_nmda, bool new_I_EXT) {
		AMPA = new_ampa;
		GABA = new_gaba;
		NMDA = new_nmda;
		I_EXT = new_I_EXT;
	}

	__device__ TimeDrivenLIF_Exponential_triple_GPU(char* int_Name, float newV_rest, float newtau, float newV_th, float newR, float newV_reset, int newt_ref,
		float new_E_ampa, float new_ampa_tau, float new_E_gaba, float new_gaba_tau, float new_nmda_tau, int Timestep, float timesteps, void** new_d_parm, int new_N_neurons) :TimeDrivenNeuralModelGPU(Timestep, timesteps), 
		V_rest(newV_rest), tau(newtau), V_th(newV_th), R(newR), V_reset(newV_reset), t_ref(newt_ref), E_ampa(new_E_ampa), ampa_tau(new_ampa_tau), E_gaba(new_E_gaba), gaba_tau(new_gaba_tau), nmda_tau(new_nmda_tau) {

		this->integration_method_on_GPU = Integration_method_GPU_Factory<TimeDrivenLIF_Exponential_triple_GPU>::create_Integration_method_GPU(int_Name, new_d_parm, this);
		this->integration_method_on_GPU->Calculate_conductance_exp_values();
	}


	__device__ ~TimeDrivenLIF_Exponential_triple_GPU() {
		delete this->integration_method_on_GPU;
	}

	/*
		* 更新状态
		* time: 当前时间步
		* timestep: 时间步长
		*/
	__device__ virtual void UpdateState(int time, float timestep) {
		// 遍历当前线程负责的神经元状态
		int index = blockIdx.x * blockDim.x + threadIdx.x;
		while (index < this->Neuron_State_Vector->NumberofNeurons)
		{
			// 更新 AMPA 电导输入
			if (this->AMPA) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[(this->N_DifferentialStates * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index];
			}
			// 更新 GABA 电导输入
			if (this->GABA) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 1) * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index + this->Neuron_State_Vector->NumberofNeurons];
			}
			if (this->NMDA) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 2) * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index + this->Neuron_State_Vector->NumberofNeurons * 2];
			}
			// 更新外部电流输入
			if (this->I_EXT) {
				this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 3) * this->Neuron_State_Vector->NumberofNeurons) + index] = this->Neuron_State_Vector->AuxStateGPU[index + (this->Neuron_State_Vector->NumberofNeurons * 3)];
			}

			// 记录本次更新时间
			this->Neuron_State_Vector->LastUpdateGPU[index] = time;
			// 继续处理该线程覆盖的后续神经元
			index += blockDim.x * gridDim.x;
		}

		// 计算积分增量
		this->integration_method_on_GPU->CaculateIncreament(this->Neuron_State_Vector->NumberofNeurons, this->Neuron_State_Vector->Vector_of_State_VariableGPU);

	}


	/*
		* 初始化 NeuronStateVector
		* NumberofState: 状态变量个数
		* AuxStateGPU: 辅助状态数组
		* VectorNeuronStateGPU: 状态向量
		* LastUpdate: 上次更新时间
		* LastSpike: 上次放电时间
		* InternalSpikeGPU: 是否产生内部脉冲
		* NumberofNeuron: 神经元数量
		*/
	__device__ void InitializeNeuronStateVector(int NumberofState, float* initState, float* AuxStateGPU, float* VectorNeuronStateGPU, int* LastUpdate, int* LastSpike, bool* InternalSpikeGPU, int NumberofNeuron) {
		this->Neuron_State_Vector = new Neuron_State_Vector_on_GPU(NumberofState, initState, AuxStateGPU, VectorNeuronStateGPU, LastUpdate, LastSpike, InternalSpikeGPU, NumberofNeuron);
	}


	/*
	* 计算电导指数项
	* index: 指数项下标（为 0）
	* elapsed_time: 时间步长
	*/
	__device__ void Caculate_Conductance(int index, float elapsed_time) {
		// AMPA 电导
		this->Set_conductance_exp_values(index, 0, exp(-elapsed_time / this->ampa_tau));
		// GABA 电导
		this->Set_conductance_exp_values(index, 1, exp(-elapsed_time / this->gaba_tau));
		// NMDA 电导
		this->Set_conductance_exp_values(index, 2, exp(-elapsed_time / this->nmda_tau));
	}


	/*
		* 计算微分方程
		* index: 神经元下标
		* NumberofNeuron: 神经元数量
		* NeuronState: 神经元状态数组
		* AuxNeuronState: 辅助状态数组
		* elapsetime: 时间步长
		*/
	__device__ void CaculateDifferentialEquation(int index, int NumberofNeuron, float* NeuronState, float* AuxNeuronState, float elapsetime) {
		float V = NeuronState[index];
		float gAMPA = NeuronState[index + NumberofNeuron];
		float gGABA = NeuronState[index + (NumberofNeuron * 2)];
		float gNMDA = NeuronState[index + (NumberofNeuron * 3)];
        float I_ext = NeuronState[index + (NumberofNeuron * 4)];
		int lastSpike = this->Neuron_State_Vector->LastSpikeGPU[index];
		// 判断是否处于不应期之外
		if (this->Neuron_State_Vector->LastSpikeGPU[index] > this->t_ref) {
			float current = 0;
			// 计算总输入电流
			if (this->AMPA) {
				current += gAMPA * (this->E_ampa - V);
			}
			if (this->GABA) {
				current += gGABA * (this->E_gaba - V);
			}
			if (this->NMDA) {
				float g_nmda_inf = 1.0f / (1.0f + expf(-0.062f * V) * (1.2f / 3.57f));
				current += gNMDA * g_nmda_inf * (this->E_ampa - V);
			}
			current += I_ext;
			AuxNeuronState[blockIdx.x * blockDim.x + threadIdx.x] = ((current * this->R) + (this->V_rest - V)) / this->tau;
		}
		else {
			AuxNeuronState[blockIdx.x * blockDim.x + threadIdx.x] = 0;
		}
	}

	/*
		* 计算时变电导项
		* index: 神经元下标
		* NumberofNeuron: 神经元数量
		* NeuronState: 神经元状态数组
		* elapsetime: 时间步长
		* elapsetimeindex: 时间步长索引
		*/
	__device__ void CaculateTimeDependentEquation(int index, int NumberofNeuron, float* NeuronState, float elapsetime, int elapsetimeindex) {
		float zerothrethhold = 1e-9;
		float* conductance_exp_values = this->Get_conductance_exponential_values(elapsetimeindex);
		if (NeuronState[NumberofNeuron * this->N_DifferentialStates + index] < zerothrethhold) {
			NeuronState[NumberofNeuron * this->N_DifferentialStates + index] = 0.0;
		}
		else {
			NeuronState[NumberofNeuron * this->N_DifferentialStates + index] *= conductance_exp_values[0];
		}
		if (NeuronState[NumberofNeuron * (this->N_DifferentialStates + 1) + index] < zerothrethhold) {
			NeuronState[NumberofNeuron * (this->N_DifferentialStates + 1) + index] = 0.0;
		}
		else {
			NeuronState[NumberofNeuron * (this->N_DifferentialStates + 1) + index] *= conductance_exp_values[1];
		}
		if (NeuronState[NumberofNeuron * (this->N_DifferentialStates + 2) + index] < zerothrethhold) {
			NeuronState[NumberofNeuron * (this->N_DifferentialStates + 2) + index] = 0.0;
		}
		else {
			NeuronState[NumberofNeuron * (this->N_DifferentialStates + 2) + index] *= conductance_exp_values[2];
		}
	}

	/*
		* 检查当前神经元是否放电
		* index: 神经元下标
		* previous_V: 上一时刻膜电位
		* NeuronState: 神经元状态数组
		* timestep: 时间步长
		*/
	__device__ void CaculateSpike(float previous_V, float* NeuronState, int index, float timestep) {
		// 判断是否达到放电阈值
		if (NeuronState[index] > this->V_th) {
			// 将膜电位重置为 V_reset
			NeuronState[index] = this->V_reset;
			// 记录本次放电时间
			this->Neuron_State_Vector->LastSpikeGPU[index] = 0;
			// 重置积分状态
			this->integration_method_on_GPU->ResetState(index);
			// 标记内部脉冲
			this->Neuron_State_Vector->InternalSpikeGPU[index] = true;

		}
	}




};


#endif // TIME_DRIVEN_LIF_EXPONENTIAL_TRIPLE_GPU_CUH
