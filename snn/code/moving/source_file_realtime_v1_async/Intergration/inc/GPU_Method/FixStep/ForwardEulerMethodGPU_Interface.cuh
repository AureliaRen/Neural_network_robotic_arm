/*
* ForwardEulerMethodGPU_Interface.cuh
* 定义了一个用于 GPU 的向前欧拉法接口
*/
#ifndef FORWARD_EULER_METHOD_GPU_INTERFACE_CUH
#define FORWARD_EULER_METHOD_GPU_INTERFACE_CUH
#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/FixStep/FixStepGPUInterface.cuh"
#include "../source_file_realtime_v1_async/error/cudaerror.h"
template <typename NeuronModelGPU>
class ForwardEulerMethodGPU_Interface :public FixStepGPUInterface<NeuronModelGPU> {
    public:
        /*
        * 增量辅助向量，用于存储增量
        */
        float* AuxNeuronState;


        /*
        * 带参构造函数
        */
        ForwardEulerMethodGPU_Interface(NeuronModelGPU* model): FixStepGPUInterface<NeuronModelGPU>(model), AuxNeuronState(0) {}

        /*
        * 析构函数
        */
        ~ForwardEulerMethodGPU_Interface() {
            if (this->AuxNeuronState != 0) {
                HANDLE_ERROR(cudaFree(this->AuxNeuronState));
            }
        }

        /*
        * 为 GPU 上的积分方法初始化参数
        * N_neurons: 神经元数量
        * Total_N_Thread: 总线程数
        */
        virtual void InitIntegrationMethodOnGPU(int N_neurons, int Total_N_Thread) {
            cudaMalloc((void**)&this->d_param, 2 * sizeof(void*));

            // dt
            float* d_dt;
            cudaMalloc(&d_dt, sizeof(float));
            cudaMemcpy(d_dt, &this->dt, sizeof(float), cudaMemcpyHostToDevice);

            // Aux
            cudaMalloc((void**)&AuxNeuronState, sizeof(float) * Total_N_Thread * this->neuron_model->N_NeuronStateVariables);

            // 组装参数表
            void* h_parm[2];
            h_parm[0] = d_dt;
            h_parm[1] = AuxNeuronState;

            cudaMemcpy(this->d_param, h_parm, 2 * sizeof(void*), cudaMemcpyHostToDevice);

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
            return "ForwardEulerMethod";
        }

        static IntegrationMethodGPU_Interface* CreateIntegerationMethod(std::map<std::string, boost::any> NeuronParameter, NeuronModelGPU* neuralmodel) {

            ForwardEulerMethodGPU_Interface* method = new ForwardEulerMethodGPU_Interface(neuralmodel);
            method->setParameters(NeuronParameter);
            return method;
        }


        virtual bool compare(IntegrationMethodGPU_Interface* method) {
            if (!FixStepGPUInterface<NeuronModelGPU>::compare(method)) {
                return false;
            }
            ForwardEulerMethodGPU_Interface* method2 = dynamic_cast<ForwardEulerMethodGPU_Interface*>(method);
            if (method2 == 0) {
                return false;
            }
            return true;
        }




};

#endif
