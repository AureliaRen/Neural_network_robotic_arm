/*
* Integration_method_GPU_Interface_Facory.cuh
* GPU上积分方法接口的工厂
*/

#ifndef INTEGRATION_METHOD_GPU_INTERFACE_FACTORY_CUH
#define INTEGRATION_METHOD_GPU_INTERFACE_FACTORY_CUH
#include <boost/any.hpp>
#include <map>

#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/FixStep/ForwardEulerMethodGPU_Interface.cuh"

template<typename NeuronModelGPUInerface>
class Integration_method_GPU_Interface_Factory {
public:
	static IntegrationMethodGPU_Interface* create_Integration_method_GPU_Interface(ModelDescription intDescription, NeuronModelGPUInerface* neuralmodel) {
		if (intDescription.ModelName == "ForwardEulerMethod") {
			IntegrationMethodGPU_Interface* Int_Model = ForwardEulerMethodGPU_Interface<NeuronModelGPUInerface>::CreateIntegerationMethod(intDescription.ModelParameter, neuralmodel);
			return Int_Model;
		}
		else {
			std::cout << "Unknown learning rule type: " << intDescription.ModelName << std::endl;
			return 0;
		}


	}

};



#endif // !INTEGRATION_METHOD_GPU_INTERFACE_FACTORY_CUH