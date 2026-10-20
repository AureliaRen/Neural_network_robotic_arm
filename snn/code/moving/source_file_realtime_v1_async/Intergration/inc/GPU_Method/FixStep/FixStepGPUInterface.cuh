/*
* FixStepGPUInterface.cuh
* 定步长积分方法的 GPU 接口
*/

#ifndef FIXSTEPGPUINTERFACE_CUH
#define FIXSTEPGPUINTERFACE_CUH

#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodGPU_InterfaceTemplate.cuh"

template<typename NeuronModelGPU>
class FixStepGPUInterface : public IntegrationMethodGPU_InterfaceTemplate<NeuronModelGPU> {
    public:
        /*
        * 构造函数
        */
        FixStepGPUInterface(NeuronModelGPU* Model): IntegrationMethodGPU_InterfaceTemplate<NeuronModelGPU>(Model) {}

        /*
        * 析构函数
        */
        virtual ~FixStepGPUInterface() {}

        /*
        * 为 GPU 上的积分方法初始化参数
        */
        virtual void InitIntegrationMethodOnGPU(int N_neurons, int Total_N_Thread) = 0;

        /*
        * 获取积分参数
        */
        virtual std::map<std::string, boost::any> getParameters() {
            // 返回参数表
            std::map<std::string, boost::any> newMap = IntegrationMethodGPU_InterfaceTemplate<NeuronModelGPU>::getParameters();
            return newMap;
        }

        /*
     * 设置积分参数
     */
        virtual void setParameters(std::map<std::string, boost::any> parameters) {
            // 写入参数表
            IntegrationMethodGPU_InterfaceTemplate<NeuronModelGPU>::setParameters(parameters);
        }

        /*
     * 比较两个积分方法是否相同
     */
        virtual bool compare(IntegrationMethodGPU_Interface* method) {
            if (!IntegrationMethodGPU_InterfaceTemplate<NeuronModelGPU>::compare(method)) {
                return false;
            }
            FixStepGPUInterface* method2 = dynamic_cast<FixStepGPUInterface*>(method);
            if (method2 == 0) {
                return false;
            }
            return true;
        }

};

#endif
