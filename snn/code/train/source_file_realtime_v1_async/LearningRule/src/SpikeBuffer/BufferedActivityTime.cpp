#include "../source_file_realtime_v1_async/LearningRule/inc/SpikeBuffer/BufferedActivityTime.h"

BufferedActivityTime::BufferedActivityTime(int newsize) {
    this->BufferSize = newsize;
	this->structure = new BufferedActivityTimesData[newsize]();
	for (int i = 0; i < newsize; i++) {
		this->structure[i].size = 2;
		this->structure[i].first_element = 0;
		this->structure[i].last_element = 0;
		this->structure[i].N_elements = 0;
		this->structure[i].SpikeData = new SpikeData[2];
	}
	this->size_output_array = new int[NumberOfOpenMPQueues];
	this->output_spike_data = new SpikeData*[NumberOfOpenMPQueues];
	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		this->size_output_array[i] = 64;
		this->output_spike_data[i] = new SpikeData[this->size_output_array[i]];
	}

}

BufferedActivityTime::~BufferedActivityTime() {
	for (int i = 0; i < this->BufferSize; i++) {
		delete[] this->structure[i].SpikeData;
	}
	for (int i = 0; i < NumberOfOpenMPQueues; i++) {
		delete[] this->output_spike_data[i];
	}
	delete[] this->output_spike_data;
	delete[] this->structure;
	delete[] this->size_output_array;
}

