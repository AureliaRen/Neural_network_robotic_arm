#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModelPropogationStructure.h"
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include <iostream>

NeuronModelPropogationStructure::NeuronModelPropogationStructure() {
	//鍒嗛厤鍐呭瓨
	this->AllocatedSize = new int[NumberOfOpenMPQueues]();
	this->SynapseDelay = new int*[NumberOfOpenMPQueues];
	this->NumberOfDelays = new int[NumberOfOpenMPQueues]();

	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		this->AllocatedSize[i] = 1;
		this->SynapseDelay[i] = new int[1];
	}

}


NeuronModelPropogationStructure::~NeuronModelPropogationStructure() {
	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		delete [] this->SynapseDelay[i];
	}
	delete [] this->SynapseDelay;
	delete [] this->NumberOfDelays;
	delete [] this->AllocatedSize;
	this->SynapseDelay = NULL;
	this->NumberOfDelays = NULL;
    this->AllocatedSize = NULL;
}

void NeuronModelPropogationStructure::IncludeNewDelay(int Queue_index, int Delay) {
	//鍏堟鏌ユ槸鍚﹀凡缁忓瓨鍦ㄤ簡delay
	int i = 0;
	while (i < this->NumberOfDelays[Queue_index] && this->SynapseDelay[Queue_index][i] != Delay) {
		i++;
	}
	//如果不存在，则添加
	if (i == this->NumberOfDelays[Queue_index]) {
		//濡傛灉宸茬粡婊′簡锛屽垯閲嶆柊鍒嗛厤鍐呭瓨
		if (this->NumberOfDelays[Queue_index] == this->AllocatedSize[Queue_index]) {
			int* temp;
			temp = this->SynapseDelay[Queue_index];
			this->AllocatedSize[Queue_index] *= 2;
			this->SynapseDelay[Queue_index] = new int[this->AllocatedSize[Queue_index]]();
			memcpy(this->SynapseDelay[Queue_index], temp, this->NumberOfDelays[Queue_index] * sizeof(int));
			delete[] temp;
			temp = NULL;
		}
		this->SynapseDelay[Queue_index][this->NumberOfDelays[Queue_index]] = Delay;
		this->NumberOfDelays[Queue_index]++;
		//鎸夌収寤惰繜鍗囧簭鎺掑垪
		for (int j = this->NumberOfDelays[Queue_index] - 1; j > 0; j--) {
			if (this->SynapseDelay[Queue_index][j] < this->SynapseDelay[Queue_index][j - 1]) {
				int delay_temp = this->SynapseDelay[Queue_index][j];
				this->SynapseDelay[Queue_index][j] = this->SynapseDelay[Queue_index][j - 1];
				this->SynapseDelay[Queue_index][j - 1] = delay_temp;
			}
		}
	}
}
