#include "../source_file_realtime_v1_async/NeuralModel/inc/TimeDrivenGPU/TimeDrivenNeuronModelGPU_Interface.cuh"
#include "../source_file_realtime_v1_async/Intergration/inc/GPU_Method/IntegrationMethodGPU_Interface.cuh"

TimeDrivenNeuronModelGPU_Interface::TimeDrivenNeuronModelGPU_Interface(int Timestep):TimeDrivenModel(Timestep),
gridsize(1), blocksize(1), State_GPU(0), integrationMethodGPUInterface(0), copyStream(0), computeStream(0) {
	this->IsGPU = true;
}

TimeDrivenNeuronModelGPU_Interface::~TimeDrivenNeuronModelGPU_Interface() {
	//delete this->State_GPU;
	delete this->integrationMethodGPUInterface;
	if (this->copyStream != 0) cudaStreamDestroy(this->copyStream);
	if (this->computeStream != 0) cudaStreamDestroy(this->computeStream);
}