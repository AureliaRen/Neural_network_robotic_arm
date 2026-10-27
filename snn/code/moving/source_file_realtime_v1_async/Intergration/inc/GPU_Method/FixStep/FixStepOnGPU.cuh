/*
* 文件名：FixStepOnGPU.cuh
* 定义了固定步长GPU积分计算方法
*/

#ifndef FIXSTEPONGPU_CUH
#define FIXSTEPONGPU_CUH

#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodOnGPUTemplate.cuh"
template<typename NeuronModelOnGPU>
class FixStepOnGPU : public IntegrationMethodOnGPUTemplate<NeuronModelOnGPU> {
public:

	__device__ FixStepOnGPU(NeuronModelOnGPU* newmodel, void** d_param) :IntegrationMethodOnGPUTemplate<NeuronModelOnGPU>(newmodel, d_param) {}


	__device__ ~FixStepOnGPU() {}

	__device__ virtual void  CaculateIncreament(int SizeStates, float* NeuronState){}


	__device__ virtual void ResetState(int index){}


	__device__ virtual void Calculate_conductance_exp_values() {
	}

};
#endif