#include "../source_file_realtime_v1_async/communication/inc/ArrayOutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/Neuron/inc/Neuron.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
#include <iostream>

ArrayOutputSpikeDriver::ArrayOutputSpikeDriver() {
}

ArrayOutputSpikeDriver::~ArrayOutputSpikeDriver() {
}


void ArrayOutputSpikeDriver::WriteSpike(Spike* NewSpike, float basetimestep) {
	
	OutputSpike spike(NewSpike->SourceNeuron->Neuron_index, NewSpike->getTime(), basetimestep);
    #pragma omp critical (ArrayOutputSpikeDriver)
	{
		this->OutputBuffer.push_back(spike);
	}
}

void ArrayOutputSpikeDriver::WriteState(int Time, Neuron* Source) {
}

bool ArrayOutputSpikeDriver::IsBuffered() {
	return true;
}

bool ArrayOutputSpikeDriver::IsWritePotentialCapable(){
	return false;
}

int ArrayOutputSpikeDriver::GetBufferedSpikes(int*& Times, int*& Cells) {
	//杈撳嚭Buffer涓殑鏀剧數淇℃伅
	unsigned int size = this->OutputBuffer.size();

	if (size > 0) {
		Times = new int[size];

		if (!Times) {
			std::cerr << "Error: Not enough memory" << std::endl;
		}

		Cells = new int[size];

		if (!Cells) {
			std::cerr << "Error: Not enough memory" << std::endl;
		}

		for (int i = 0; i < size; ++i) {
			Times[i] = this->OutputBuffer[i].time;
			Cells[i] = this->OutputBuffer[i].neuron;
		}

		this->OutputBuffer.clear();
	}

	return size;

}

bool ArrayOutputSpikeDriver::RemoveBufferedSpike(int& Time, int& Cell) {
	bool noempty = !this->OutputBuffer.empty();
	if (noempty) {
		Time = this->OutputBuffer[0].time;
		Cell = this->OutputBuffer[0].neuron;
		this->OutputBuffer.erase(this->OutputBuffer.begin());
	}
	return noempty;
}

void ArrayOutputSpikeDriver::FlushBuffers() {
	return;
}

void ArrayOutputSpikeDriver::ClearBuffer() {
	this->OutputBuffer.clear();
}
