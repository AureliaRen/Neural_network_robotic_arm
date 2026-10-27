#include "../source_file_realtime_v1_async/InputCurrentDriver/inc/ArrayInputCurrentDriver.h"
#include "../source_file_realtime_v1_async/Event/inc/Current/InputCurrent.h"
#include "../source_file_realtime_v1_async/Network/inc/Network.h"
#include "../source_file_realtime_v1_async/EventQueue/inc/EventQueue.h"

ArrayInputCurrentDriver::ArrayInputCurrentDriver(): InputCurrentDriver(){}


ArrayInputCurrentDriver::~ArrayInputCurrentDriver(){}


void ArrayInputCurrentDriver::LoadInputCurrent(EventQueue* eventQueue, Network* network, int currentnum, const int* Times, const int* neuron_index, const float* current) {
	if (currentnum > 0) {
		for (int i = 0; i < currentnum; i++) {
			InputCurrent* inputCurrent = new InputCurrent(&(network->neurons[neuron_index[i]]), Times[i], network->neurons[neuron_index[i]].Queue_index, current[i]);
			eventQueue->Insert_a_Event(inputCurrent, inputCurrent->getIndex());
		}
	}
}