/*
* IntegrationMethodGPU_InterfaceTemplate.cuh
* 定义了一个 GPU 积分方法的模板类
*/

#ifndef INTEGRATIONMETHODGPU_INTERFACETEMPLATE_CUH_
#define INTEGRATIONMETHODGPU_INTERFACETEMPLATE_CUH_

#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodGPU_Interface.cuh"

template <typename NeuronModelGPU>
class IntegrationMethodGPU_InterfaceTemplate :public IntegrationMethodGPU_Interface {
public:

	//神经元模型对象
	NeuronModelGPU* neuron_model;

	/*
	* 构造函数
	*/
	IntegrationMethodGPU_InterfaceTemplate(NeuronModelGPU* Model) :IntegrationMethodGPU_Interface(), neuron_model(Model) {}

	/*
	* 析构函数
	*/
    virtual ~IntegrationMethodGPU_InterfaceTemplate() {}

	/*
	* 为 GPU 上的积分方法初始化参数
	*/
	virtual void InitIntegrationMethodOnGPU(int N_neurons, int Total_N_Thread) = 0;

	/*
	 * 获取积分参数
	 */
	virtual std::map<std::string, boost::any> getParameters() {
		std::map<std::string, boost::any> parameters = IntegrationMethodGPU_Interface::getParameters();
		return parameters;
	}


	/*
	 * 设置积分参数
	 */
	virtual void setParameters(std::map<std::string, boost::any> parameters) {
		IntegrationMethodGPU_Interface::setParameters(parameters);
	}

	/*
	 * 比较两个积分方法是否相同
	 */
	virtual bool compare(IntegrationMethodGPU_Interface* method) {
		if (!IntegrationMethodGPU_Interface::compare(method)) {
			return false;
		}
		IntegrationMethodGPU_InterfaceTemplate* method2 = dynamic_cast<IntegrationMethodGPU_InterfaceTemplate*>(method);
		if (method2 == 0) return false;
		return true;
	};

};

#endif
