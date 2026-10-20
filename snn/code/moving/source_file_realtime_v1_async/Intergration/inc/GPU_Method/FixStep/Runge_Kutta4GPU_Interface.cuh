/*
* Runge_Kutta4GPU_Interface.cuh
* 定义了用于GPU的Runge-Kutta4方法的接口
*/

#ifndef RUNGE_KUTTA4GPU_INTERFACE_CUH
#define RUNGE_KUTTA4GPU_INTERFACE_CUH

#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/FixStep/FixStepGPUInterface.cuh"

#include "../source_file_realtime_v1_async/error/cudaerror.h"

template <typename NeuronModelGPU>

class Runge_Kutta4GPUInterface : public FixStepGPUInterface<NeuronModelGPU> {

	public:
		/*
		* 辅助向量
		*/
		float* AuxNeuronState;
		float* AuxNeuronState1;
		float* AuxNeuronState2;
		float* AuxNeuronState3;
		float* AuxNeuronState4;

		/*
		* 带参构造函数
		*/
		Runge_Kutta4GPUInterface(NeuronModelGPU* model) : FixStepGPUInterface<NeuronModelGPU>(model), AuxNeuronState(0),AuxNeuronState1(0),AuxNeuronState2(0),AuxNeuronState3(0),AuxNeuronState4(0) {}


		/*
		* 析构函数
		*/
		~Runge_Kutta4GPUInterface() {
			if (this->AuxNeuronState != 0) {
				HANDLE_ERROR(cudaFree(this->AuxNeuronState));
			}
			if (this->AuxNeuronState1 != 0) {
				HANDLE_ERROR(cudaFree(this->AuxNeuronState1));
			}
			if (this->AuxNeuronState2 != 0) {
				HANDLE_ERROR(cudaFree(this->AuxNeuronState2));
			}
			if (this->AuxNeuronState3 != 0) {
				HANDLE_ERROR(cudaFree(this->AuxNeuronState3));
			}
			if (this->AuxNeuronState4 != 0) {
				HANDLE_ERROR(cudaFree(this->AuxNeuronState4));
			}
		}

		/*
		* 为GPU上的积分方法初始化参数
		*/
		virtual void InitIntegrationMethodOnGPU(int N_neurons, int Total_N_Thread) {

			cudaMalloc((void**)&this->d_parm, 6 * sizeof(void*));

			//dt

			float* d_dt;

			cudaMalloc(&d_dt, sizeof(float));

			cudaMemecpy(d_dt, &this->dt, sizeof(float), cudaMemecpyHostToDevice);

			//Aux

			cudaMalloc((void**)&AuxNeuronState, sizeof(float) * Total_N_Thread * this->neuron_model->N_NeuronStateVariables);
			cudaMalloc((void**)&AuxNeuronState1, sizeof(float) * Total_N_Thread * this->neuron_model->N_NeuronStateVariables);
			cudaMalloc((void**)&AuxNeuronState2, sizeof(float) * Total_N_Thread * this->neuron_model->N_NeuronStateVariables);
			cudaMalloc((void**)&AuxNeuronState3, sizeof(float) * Total_N_Thread * this->neuron_model->N_NeuronStateVariables);
			cudaMalloc((void**)&AuxNeuronState4, sizeof(float) * Total_N_Thread * this->neuron_model->N_NeuronStateVariables);

			//组装参数表
			void* h_parm[6];
            h_parm[0] = d_dt;
			h_parm[1] = AuxNeuronState;
			h_parm[2] = AuxNeuronState1;
			h_parm[3] = AuxNeuronState2;
			h_parm[4] = AuxNeuronState3;
			h_parm[5] = AuxNeuronState4;

			cudaMemcpy(this->d_param, h_parm, 6 * sizeof(void*), cudaMemcpyHostToDevice);

		}

		/*
		* 获取积分参数
		*/
		virtual std::map<std::string, boost::any> getParameters() {
			// 返回参数表
			std::map<std::string, boost::any> newMap = IntegrationMethodGPU_InterfaceTemplate<NeuronModelGPU>::getParameters();

			newMap["name"] = getName();

			return newMap;
		}

		/*
		* 设置积分参数
		*/
		virtual void setParameters(std::map<std::string, boost::any> parameters) {
			// 写入参数表

			parameters["name"] = getName();

			FixStepGPUInterface<NeuronModelGPU>::setParameters(parameters);
		}

		/*
		* 获取积分方法名称
		*/
		static std::string getName() {
			return "Runge_Kutta4GPU";
		}


		static IntegrationMethodGPU_Interface* CreateIntegerationMethod(std::map<std::string, boost::any> NeuronParameter, NeuronModelGPU* neuralmodel) {

			Runge_Kutta4GPUInterface* method = new Runge_Kutta4GPUInterface(neuralmodel);

			method->setParameters(NeuronParameter);

			return method;

		}


		virtual bool compare(IntegrationMethodGPU_Interface* method) {
			if (!FixStepGPUInterface<NeuronModelGPU>::compare(method)) {

				return false;

			}

			Runge_Kutta4GPUInterface* method2 = dynamic_cast<Runge_Kutta4GPUInterface*>(method);

			if (method2 == 0) {

				return false;

			}

			return true;
		}


};


#endif // !RUNGE_KUTTA4GPU_INTERFACE_CUH