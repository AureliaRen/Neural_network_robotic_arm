
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector_Interface.cuh"
#include <iostream>
#include "../source_file_realtime_v1_async/error/cudaerror.h"
#include <random>

Neuron_State_Vector_Interface::Neuron_State_Vector_Interface(int StateNumber) :Neuron_State_Vector(StateNumber, true, true),
AuxStateCPU(0), AuxStateGPU(0), Vector_of_StateVariableGPU(0), LastUpdateGPU(0),
LastSpikingGPU(0), InternalSpikeGPU(0), InternalSpikeCPU(0), InitialStateGPU(0), InitialStateCPU(0), deviceProp() {}

Neuron_State_Vector_Interface::~Neuron_State_Vector_Interface() {
	if (this->AuxStateCPU != 0) {
		HANDLE_ERROR(cudaFreeHost(this->AuxStateCPU));
	}

	if (this->InternalSpikeCPU != 0) {
		HANDLE_ERROR(cudaFreeHost(this->InternalSpikeCPU));
	}

	if (this->Vector_of_StateVariableGPU != 0) {
		HANDLE_ERROR(cudaFree(this->Vector_of_StateVariableGPU));
	}

	//鍒ゆ柇鏄惁鑳藉搴旂敤涓绘満鍐呭瓨鏄犲皠锛屽鏋滀笉鑳斤紝閲婃斁GPU鍐呭瓨
	if (!this->deviceProp.canMapHostMemory) {
		if (this->AuxStateGPU != 0) {
			HANDLE_ERROR(cudaFree(this->AuxStateGPU));
		}

		if (this->InternalSpikeGPU != 0) {
			HANDLE_ERROR(cudaFree(this->InternalSpikeGPU));
		}
	}

	if (this->LastUpdateGPU != 0) {
		HANDLE_ERROR(cudaFree(this->LastUpdateGPU));
	}

	if (this->LastSpikingGPU != 0) {
		HANDLE_ERROR(cudaFree(this->LastSpikingGPU));
	}

	if (this->InitialStateGPU != 0) {
        HANDLE_ERROR(cudaFree(this->InitialStateGPU));
	}
}

void Neuron_State_Vector_Interface::InitNeuronStateGPU(int NeuronNum, float* initStateVariable, float* sigma, int N_AuxState, cudaDeviceProp prop) {
	//为神经元个数幅值
	this->NumberofNeuron = NeuronNum;
	std::random_device rd;       // 用于生成随机种子
	std::mt19937 gen(rd());      // 随机数引擎（梅森旋转算法）

	// 创建标准正态分布对象：mean = 0, stddev = 1
	std::normal_distribution<float> dist(0.0, 1.0);
	//为CPU上的状态内存向量初始化内存分配
	this->Vector_of_StateVariable = new float[this->NumberofNeuron * this->NumberofStateVariable];
	this->LastSpike = new int[this->NumberofNeuron]();
	this->LastUpdate = new int[this->NumberofNeuron]();
	this->InitialStateCPU = initStateVariable;

	if (!this->TimeDriven) {
		PredictSpike = new int[this->NumberofNeuron]();
	}
	// 为 CPU 上的状态向量赋初值，该向量用于与 GPU 上的向量同步
	for (int i = 0; i < this->NumberofNeuron; i++) {
		for (int j = 0; j < this->NumberofStateVariable; j++) {
			this->Vector_of_StateVariable[this->NumberofNeuron * j + i] = initStateVariable[j] + sigma[j] * dist(gen);
		}
	}
	for (int i = 0; i < this->NumberofNeuron; i++) {
		this->LastSpike[i] = 100000;
	}

	//为AuxState分配页锁定内存，实现GPU与CPU内存的高速同步
	//为CPU上的AuxState内存向量初始化内存分配，并初始化为0
	HANDLE_ERROR(cudaHostAlloc((void**)&this->AuxStateCPU, N_AuxState * this->NumberofNeuron * sizeof(float), cudaHostAllocMapped));
	memset(this->AuxStateCPU, 0, N_AuxState * this->NumberofNeuron * sizeof(float));
	//为CPU上的InternalSpike内存向量初始化内存分配，并初始化为0
	HANDLE_ERROR(cudaHostAlloc((void**)&this->InternalSpikeCPU, this->NumberofNeuron * sizeof(bool), cudaHostAllocMapped));
	memset(this->InternalSpikeCPU, 0, this->NumberofNeuron * sizeof(bool));

	//在GPU上分配内存
	//鍒ゆ柇鏄惁鑳藉搴旂敤涓绘満鍐呭瓨鏄犲皠锛屽鏋滀笉鑳斤紝鍒嗛厤GPU鍐呭瓨
	if (prop.canMapHostMemory) {
		//建立主机与GPU之间的内存映射
		HANDLE_ERROR(cudaHostGetDevicePointer(&this->AuxStateGPU, this->AuxStateCPU, 0));
		HANDLE_ERROR(cudaHostGetDevicePointer(&this->InternalSpikeGPU, this->InternalSpikeCPU, 0));
	}
	else {
		//分配GPU内存
		HANDLE_ERROR(cudaMalloc((void**)&this->AuxStateGPU, N_AuxState * sizeof(float) * this->NumberofNeuron));
		HANDLE_ERROR(cudaMalloc((void**)&this->InternalSpikeGPU, this->NumberofNeuron * sizeof(bool)));
		HANDLE_ERROR(cudaMemset(this->InternalSpikeGPU, 0, this->NumberofNeuron * sizeof(bool)));
	}

	//分配GPU专享内存
	HANDLE_ERROR(cudaMalloc((void**)&this->Vector_of_StateVariableGPU, this->NumberofNeuron * this->NumberofStateVariable * sizeof(float)));
	HANDLE_ERROR(cudaMalloc((void**)&this->LastUpdateGPU, this->NumberofNeuron * sizeof(int)));
	HANDLE_ERROR(cudaMalloc((void**)&this->LastSpikingGPU, this->NumberofNeuron * sizeof(int)));
	HANDLE_ERROR(cudaMalloc((void**)&this->InitialStateGPU, this->NumberofStateVariable * sizeof(float)));

	//将CPU上的状态向量同步到GPU上
	HANDLE_ERROR(cudaMemcpy(this->Vector_of_StateVariableGPU, this->Vector_of_StateVariable, this->NumberofNeuron * this->NumberofStateVariable * sizeof(float), cudaMemcpyHostToDevice));
	HANDLE_ERROR(cudaMemcpy(this->LastUpdateGPU, this->LastUpdate, this->NumberofNeuron * sizeof(int), cudaMemcpyHostToDevice));
	HANDLE_ERROR(cudaMemcpy(this->LastSpikingGPU, this->LastSpike, this->NumberofNeuron * sizeof(int), cudaMemcpyHostToDevice));
	HANDLE_ERROR(cudaMemcpy(this->InitialStateGPU, this->InitialStateCPU, this->NumberofStateVariable * sizeof(float), cudaMemcpyHostToDevice));
	
}

bool* Neuron_State_Vector_Interface::getInternalSpike() {
	return this->InternalSpikeCPU;
}
