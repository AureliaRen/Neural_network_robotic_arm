#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
#include "../source_file_realtime_v1_async/Event/inc/Event.h"


Spike::Spike():Event(0,0), SourceNeuron(0){}


Spike::Spike(Neuron* SourceNeuron, int time, int QueueIndex):Event(time, QueueIndex), SourceNeuron(SourceNeuron){}


Spike::~Spike() {

}