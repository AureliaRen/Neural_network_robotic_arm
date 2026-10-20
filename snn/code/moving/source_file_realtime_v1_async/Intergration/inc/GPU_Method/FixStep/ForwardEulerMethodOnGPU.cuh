/*

* 文件名：ForwardEulerMethodOnGPU.cuh

* 定义了前向欧拉法在 GPU 上的实现

*/



#ifndef FORWARD_EULER_METHOD_ON_GPU_CUH

#define FORWARD_EULER_METHOD_ON_GPU_CUHGPU



#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/FixStep/FixStepOnGPU.cuh"



template<typename NeuronModelOnGPU>

class ForwardEulerMethodOnGPU :public FixStepOnGPU<NeuronModelOnGPU> {

public:

	//辅助增量向量

	float* AuxNeuronState;



	/*

	* 构造函数

	*/

	__device__ ForwardEulerMethodOnGPU(NeuronModelOnGPU* Model, void** d_parm) : FixStepOnGPU<NeuronModelOnGPU>(Model, d_parm) {

		this->AuxNeuronState = ((float*)d_parm[1]);

	}



	/*

	* 析构函数

	*/

	__device__ ~ForwardEulerMethodOnGPU() {}





	/*

	* 计算微分方程增量

	*/

	__device__ virtual void CaculateIncreament(int SizeStates, float* NeuronState) {

		//计算线程索引

		int index = blockIdx.x * blockDim.x + threadIdx.x;

		while (index < SizeStates) {

			float previous_voltage = NeuronState[index];

			//计算增量

			this->neuron_model->CaculateDifferentialEquation(index, SizeStates, NeuronState, this->AuxNeuronState, this->dt);

			// 将微分方程结果累加回状态变量

			int batch_offset = gridDim.x * blockDim.x;

			int GPU_offset = blockDim.x * blockIdx.x + threadIdx.x;

			for (int i = 0; i < this->neuron_model->N_DifferentialStates; i++) {

				NeuronState[i * SizeStates + index] += this->dt * this->AuxNeuronState[i * batch_offset + GPU_offset];

			}

			//计算电导

			this->neuron_model->CaculateTimeDependentEquation(index, SizeStates, NeuronState, this->dt, 0);

			// 累加距离上次放电的时间步数

			this->neuron_model->Neuron_State_Vector->LastSpikeGPU[index] += 1;

			//判断是否放电

			this->neuron_model->CaculateSpike(previous_voltage, this->neuron_model->Neuron_State_Vector->Vector_of_State_VariableGPU, index, this->dt);

			// 检查积分结果是否有效

			this->neuron_model->CheckValidIntegeration(index);



			index += gridDim.x * blockDim.x;



		}

	}



	/*

	* 重置状态

	*/

	__device__ virtual void ResetState(int index) {};





	/*

	* 计算电导

	*/

	__device__ virtual void Calculate_conductance_exp_values() {

		this->neuron_model->Initialize_conductance_exp_values(neuron_model->TimeDependentInputSize, 1);

		this->neuron_model->Caculate_Conductance(0, this->dt);

	}



};





#endif // FORWARD_EULER_METHOD_ON_GPU_CUH

