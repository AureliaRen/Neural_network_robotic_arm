#include "../source_file_realtime_v1_async/InputSpikeDriver/inc/ArrayInputSpikeDriver.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InputSpike.h"
#include "../source_file_realtime_v1_async/InputSpikeDriver/inc/InputSpikeDriver.h"

ArrayInputSpikeDriver::	ArrayInputSpikeDriver():InputSpikeDriver(){
	this->isFinished = false;
}


ArrayInputSpikeDriver::~ArrayInputSpikeDriver(){}




void ArrayInputSpikeDriver::LoadInputSpike(EventQueue* eventQueue, Network* network, int spikenum, const int* Times, const int* neuron_index) {
	if (spikenum > 0) {
		for (int i = 0; i < spikenum; i++) {
			InputSpike* Spike = new InputSpike(&(network->neurons[neuron_index[i]]), Times[i], network->neurons[neuron_index[i]].Queue_index);
			eventQueue->Insert_a_Event(Spike, Spike->getIndex());
		}
	}
	//瀹屾垚鍒濆杈撳叆
	this->isFinished = true;
}