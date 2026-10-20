/*
* IntegrationMethodOnGPUTemplate.cuh
*/
#ifndef INTEGRATIONMETHODONGPUTEMPLATE_CUH
#define INTEGRATIONMETHODONGPUTEMPLATE_CUH

#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodOnGPU.cuh"

template <typename NeuronModelOnGPU>
class IntegrationMethodOnGPUTemplate :public IntegrationMethodOnGPU {
public:

	NeuronModelOnGPU* neuron_model;

	__device__ IntegrationMethodOnGPUTemplate(NeuronModelOnGPU* model, void** d_parm) : IntegrationMethodOnGPU(d_parm), neuron_model(model) {}

    __device__ ~IntegrationMethodOnGPUTemplate() {}


	__device__ virtual void  CaculateIncreament(int SizeStates, float* NeuronState){}


	__device__ virtual void ResetState(int index){}

	__device__ virtual void Calculate_conductance_exp_values() {
	}

};

#endif