#include "../source_file_realtime_v1_async/Neuron/inc/NeuronPropogationStructure.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModelPropogationStructure.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/Openmp/inc/Openmp.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"


NeuronPropogationStructure::NeuronPropogationStructure(Neuron* neuron) {
	//鍒嗛厤鍐呭瓨
	this->NDifferentdelays = new int[NumberOfOpenMPQueues]();
	this->NInterconnections = new int*[NumberOfOpenMPQueues];
	this->SynapseDelayIndex = new int*[NumberOfOpenMPQueues];
    this->SynapseDelay = new int*[NumberOfOpenMPQueues];
	this->interconnections = new Interconnections**[NumberOfOpenMPQueues];
	int delay1, delay2;
	//按照队列数开始循环
	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		//濡傛灉绁炵粡鍏冨璇ラ槦鍒楁湁杩炴帴
		if (neuron->Output_Synaps_Number[i] > 0) {
			//鍙栧嚭绗竴涓繛鎺ョ殑寤惰繜
			delay1 = neuron->Output_Synaps[i][0]->delay;
			//更新不同延迟的个数
			this->NDifferentdelays[i]++;
			//按照连接开始循环
			for (int j = 1; j < neuron->Output_Synaps_Number[i]; j++) {
                //鍙栧嚭绗琷涓繛鎺ョ殑寤惰繜
				delay2 = neuron->Output_Synaps[i][j]->delay;
				// 如果与上一个连接的延迟不同
				if (delay1 != delay2) {
					//更新不同延迟的个数
					this->NDifferentdelays[i]++;
                    //鏇存柊涓婁竴涓繛鎺ョ殑寤惰繜
					delay1 = delay2;
				}
			}
		}
	}

	//根据NDifferentdelays的值分配内存
	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		//濡傛灉绁炵粡鍏冨璇ラ槦鍒楁湁杩炴帴
		if (this->NDifferentdelays[i] > 0) {
			this->NInterconnections[i] = new int[this->NDifferentdelays[i]]();
			this->SynapseDelayIndex[i] = new int[this->NDifferentdelays[i]];
			this->interconnections[i] = new Interconnections * [this->NDifferentdelays[i]];
			this->SynapseDelay[i] = new int[this->NDifferentdelays[i]];
			//取出第一个延迟
			delay1 = neuron->Output_Synaps[i][0]->delay;
			this->NInterconnections[i][0]++;
			this->SynapseDelay[i][0] = delay1;
			this->interconnections[i][0] = neuron->Output_Synaps[i][0];
			//绗琲ndex寤惰繜
			int index = 0;
			for (int j = 1; j < neuron->Output_Synaps_Number[i]; j++) {
				delay2 = neuron->Output_Synaps[i][j]->delay;
				//娉ㄦ剰:neuron涓殑Output_Synaps鏄寜鐓у欢杩熶粠灏忓埌澶ф帓搴忕殑
				if (delay1 != delay2) {
					//发现了新的延迟
					index++;
					delay1 = delay2;
					//存储延迟数值
					this->SynapseDelay[i][index] = delay2;
					//存储连接的指针
					this->interconnections[i][index] = neuron->Output_Synaps[i][j];
                    //更新连接的个数
					this->NInterconnections[i][index]++;

				}
				else {
					this->NInterconnections[i][index]++;
				}
			}
		}
	}


}


NeuronPropogationStructure::~NeuronPropogationStructure() {
	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		if (this->NDifferentdelays[i] > 0) {
			delete[] this->NInterconnections[i];
			delete[] this->SynapseDelayIndex[i];
			delete[] this->SynapseDelay[i];
			delete[] this->interconnections[i];
		}
	}
	delete[] this->NDifferentdelays;
	delete[] this->NInterconnections;
	delete[] this->SynapseDelayIndex;
	delete[] this->SynapseDelay;
	delete[] this->interconnections;
}

void NeuronPropogationStructure::CalculateSynapseDelayIndex(NeuronModelPropogationStructure* neuronModelPropogationStructure) {
	//按照队列数开始循环
	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		//濡傛灉绁炵粡鍏冨璇ラ槦鍒楁湁杩炴帴
		if (this->NDifferentdelays[i] > 0) {
			//按照不同延迟的个数开始循环
			for (int j = 0; j < this->NDifferentdelays[i]; j++) {
				int index = 0;
				//如果局部延迟与全局延迟不同则向后查找
				while (this->SynapseDelay[i][j] != neuronModelPropogationStructure->SynapseDelay[i][index]) {
					index++;
				}
                //瀛樺偍鍏ㄥ眬绱㈠紩
				this->SynapseDelayIndex[i][j] = index;
			}
		}

	}
}

