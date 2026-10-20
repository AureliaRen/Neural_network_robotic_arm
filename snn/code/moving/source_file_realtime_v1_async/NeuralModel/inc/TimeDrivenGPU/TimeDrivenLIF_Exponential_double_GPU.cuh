/*
* 文件名：TimeDrivenLIF_Exponential_double_GPU.cuh
* 定义了事件驱动 LIF 神经元模型类，该类在 GPU 上进行并行计算，继承自 TimeDrivenNeuralModelGPU 类
*/

#ifndef TIME_DRIVEN_LIF_EXPONENTIAL_DOUBLE_On_GPU_CUH
#define TIME_DRIVEN_LIF_EXPONENTIAL_DOUBLE_On_GPU_CUH

#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuralModelGPU.cuh"
#include "../source_file_realtime_v1_async/ModelFactory/Integration_method_GPU_Factory.cuh"
#include <iostream>
#include <array>

class TimeDrivenLIF_Exponential_double_GPU : public TimeDrivenNeuralModelGPU {
	public:

		float V_rest = -60.; //默认静息电位

		float tau = 20.; //默认时间常数

		float V_th = -50.; //默认阈值

		float R = 1.; //默认输入电阻

		float V_reset = -60.; //默认重置电位

		int t_ref = 50; //默认不应期（基准时间步倍数 5ms）

		float gexc_tau = 5.; //兴奋性突触门控时间常数

		float Eexc = 0.; //兴奋性反转膜电位

		float ginh_tau = 10.; //抑制性突触门控时间常数

		float Einhibitory = -80.; //抑制性反转膜电位

		const int N_NeuronStateVariables = 4; //神经元状态变量个数：电压，兴奋门控，抑制门控，外加电流

		const int N_DifferentialStates = 1; //微分方程个数：电压

		const int index_V = 0; //电压在状态变量中的索引（优先存储微分方程变量）

		const int index_gexc = 1; //兴奋门控在状态变量中的索引

		const int index_ginh = 2; //抑制门控在状态变量中的索引

		const int TimeDependentInputSize = 3; //时间依赖输入个数：外加电流，兴奋电导，抑制电导

		const int I_EXT_index = 3; //外加电流在状态变量中的索引



		bool I_EXT = false;

		bool Excited = false;

		bool Inhibitory = false;

		/*
		* 设置该模型启用的输入类型：兴奋/抑制/外部电流
		*/
		__device__ virtual void SetEnabledSynapsis(bool new_Excited, bool new_Inhibited, bool new_I_EXT) {
			Excited = new_Excited;
			Inhibitory = new_Inhibited;
			I_EXT = new_I_EXT;
		}

		/*
		* 构造函数
		* V_rest: 静息电位
		* tau: 时间常数
		* V_th: 阈值
		* R: 输入电阻
		* V_reset: 重置电位
		* t_ref: 不应期
		* gexc_tau: 兴奋性突触门控时间常数
		* Exc: 兴奋性反转膜电位
		* ginh_tau: 抑制性突触门控时间常数
		* Einhibitory: 抑制性反转膜电位
		* Timestep: 当前时间步
		* timesteps: 时间步长
		* parm: 积分参数
		* N_neurons: 神经元个数
		*/
		__device__ TimeDrivenLIF_Exponential_double_GPU(char* int_Name, float newV_rest, float newtau, float newV_th, float newR, float newV_reset, int newt_ref, float newgexc_tau, float newExc,
			float newginh_tau, float newEinhibitory, int Timestep, float timesteps, void** parm, int N_neurons) :TimeDrivenNeuralModelGPU(Timestep, timesteps), V_rest(newV_rest), tau(newtau), V_th(newV_th), R(newR), V_reset(newV_reset), t_ref(newt_ref), gexc_tau(newgexc_tau), Eexc(newExc), ginh_tau(newginh_tau), Einhibitory(newEinhibitory) {
			//创建积分方法
			this->integration_method_on_GPU = Integration_method_GPU_Factory<TimeDrivenLIF_Exponential_double_GPU>::create_Integration_method_GPU(int_Name, parm, this);
			this->integration_method_on_GPU->Calculate_conductance_exp_values();
		}

		/*
		* 析构函数
		*/
		__device__ ~TimeDrivenLIF_Exponential_double_GPU() {
			delete this->integration_method_on_GPU;
		}


		/*
		* 更新状态
		* time: 当前时间步
		* timestep: 时间步长
		*/
		__device__ virtual void UpdateState(int time, float timestep) {
			//并行更新神经元状态
			int index = blockIdx.x * blockDim.x + threadIdx.x;
			while (index < this->Neuron_State_Vector->NumberofNeurons)
			{
				//如果有兴奋性电导输入
				if (this->Excited) {
					this->Neuron_State_Vector->Vector_of_State_VariableGPU[(this->N_DifferentialStates * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index];
				}
				//如果有抑制性电导输入
				if (this->Inhibitory) {
					this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 1) * this->Neuron_State_Vector->NumberofNeurons) + index] += this->Neuron_State_Vector->AuxStateGPU[index + this->Neuron_State_Vector->NumberofNeurons];
				}
				//如果接受外界电流输入
				if (this->I_EXT) {
					this->Neuron_State_Vector->Vector_of_State_VariableGPU[((this->N_DifferentialStates + 2) * this->Neuron_State_Vector->NumberofNeurons) + index] = this->Neuron_State_Vector->AuxStateGPU[index + (this->Neuron_State_Vector->NumberofNeurons * 2)];
				}
	
				//记录本次更新时间
				this->Neuron_State_Vector->LastUpdateGPU[index] = time;
				//如果神经元数量多于线程
				index += blockDim.x * gridDim.x;
			}

			//计算积分增量
			this->integration_method_on_GPU->CaculateIncreament(this->Neuron_State_Vector->NumberofNeurons, this->Neuron_State_Vector->Vector_of_State_VariableGPU);

		}

		/*
		* 初始化 NeuronStateVector
		* NumberofState: 状态变量个数
		* AuxStateGPU: 辅助状态变量
		* VectorNeuronStateGPU: 状态变量
		* LastUpdate: 上次更新时间
		* LastSpike: 上次发放脉冲时间
		* InternalSpikeGPU: 内部发放脉冲标志
		* NumberofNeuron: 神经元个数
		*/
		__device__ void InitializeNeuronStateVector(int NumberofState, float* initState, float* AuxStateGPU, float* VectorNeuronStateGPU, int* LastUpdate, int* LastSpike, bool* InternalSpikeGPU, int NumberofNeuron) {
			this->Neuron_State_Vector = new Neuron_State_Vector_on_GPU(NumberofState, initState, AuxStateGPU, VectorNeuronStateGPU, LastUpdate, LastSpike, InternalSpikeGPU, NumberofNeuron);
		}

		/*
		* 计算电导的值
		* index: 电导组索引（为 0）
		* elapsed_time: 时间步长
		*/
		__device__ void Caculate_Conductance(int index, float elapsed_time) {
			//兴奋性电导
			this->Set_conductance_exp_values(index, 0, exp(-elapsed_time / this->gexc_tau));
			//抑制性电导
			this->Set_conductance_exp_values(index, 1, exp(-elapsed_time / this->ginh_tau));
		}

		/*
		* 计算微分方程
		* index: 神经元索引
		* NumberofNeuron: 神经元个数
		* NeuronState: 神经元状态变量
		* AuxNeuronState: 辅助状态变量
		* elapsetime: 时间步长
		*/
		__device__ void CaculateDifferentialEquation(int index, int NumberofNeuron, float* NeuronState, float* AuxNeuronState, float elapsetime) {
			float V = NeuronState[index];
			float gexc = NeuronState[index + NumberofNeuron];
            float ginh = NeuronState[index + (NumberofNeuron * 2)];
			float I_ext = NeuronState[index + (NumberofNeuron * 3)];
			int lastSpike = this->Neuron_State_Vector->LastSpikeGPU[index];
			//判断是否处于不应期之外
			if (this->Neuron_State_Vector->LastSpikeGPU[index] > this->t_ref) {
				float current = 0;
				//计算电流
				if (this->Excited) {
					current += gexc * (this->Eexc - V);
				}
				if (this->Einhibitory) {
					current += ginh * (this->Einhibitory - V);
				}
				current += I_ext;
				AuxNeuronState[blockIdx.x * blockDim.x + threadIdx.x] = ((current * this->R) + (this->V_rest - V)) / this->tau;
			}
			else {
				AuxNeuronState[blockIdx.x * blockDim.x + threadIdx.x] = 0;
			}
		}

		/*
		* 计算电导方程
		* index: 神经元索引
		* NumberofNeuron: 神经元个数
		* NeuronState: 神经元状态变量
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
			
		}



		/*
		* 计算当前神经元的放电
		* index: 神经元索引
		* previous_V: 上次更新时的膜电位
		* NeuronState: 神经元状态变量
		* timestep: 时间步长
		*/
		__device__ void CaculateSpike(float previous_V, float* NeuronState, int index, float timestep) {
			//如果越过了阈值
			if (NeuronState[index] > this->V_th) {
				//将膜电位重置为 V_reset
				NeuronState[index] = this->V_reset;
				//记录本次放电时间
				this->Neuron_State_Vector->LastSpikeGPU[index] = 0;
				//重置积分状态
				this->integration_method_on_GPU->ResetState(index);
				//将 InternalSpikeGPU 标记为 true
                this->Neuron_State_Vector->InternalSpikeGPU[index] = true;

			}
		}


};


#endif // !TIME_DRIVEN_LIF_EXPONENTIAL_DOUBLE_GPU_CUH
