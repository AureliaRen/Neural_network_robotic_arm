/*
* Integration_method_GPU_Factory.cuh
* 定义了GPU上积分方法的工厂类
*/

#ifndef INTEGRATION_METHOD_GPU_FACTORY_CUH
#define INTEGRATION_METHOD_GPU_FACTORY_CUH

#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodOnGPU.cuh"
#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/FixStep/ForwardEulerMethodOnGPU.cuh"

template<typename NeuronModelGPU>
class Integration_method_GPU_Factory {
public:

	/*
	* 比较两个字符串是否相等
	*/
	__device__ static int cmp4(char const* c1, char const* c2, int size) {
		for (int j = 0; j < size; j++) {
			if ((int)c1[j] > (int)c2[j]) {
				return 1;
			}
			else if ((int)c1[j] < (int)c2[j]) {
				return -1;
			}
		}
		return 0;
	}
	
	__device__ static IntegrationMethodOnGPU* create_Integration_method_GPU(char* Name, void** d_parm, NeuronModelGPU* model) {

		if (Integration_method_GPU_Factory::cmp4(Name, "ForwardEulerMethod", 18) == 0) {
			IntegrationMethodOnGPU* Int_Model = new ForwardEulerMethodOnGPU<NeuronModelGPU>(model, d_parm);
			Int_Model->name = Name;
			return Int_Model;
		}
		else {
			printf("Unknown Learning Rule Type\n", Name);
			return 0;
		}
	}
};


#endif // INTEGRATION_METHOD_GPU_FACTORY_CUH