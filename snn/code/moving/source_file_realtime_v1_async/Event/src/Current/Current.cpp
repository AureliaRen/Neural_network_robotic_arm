#include "../source_file_realtime_v1_async/Event/inc/Current/Current.h"

Current::Current() :Event(0, 0), SourceNeuron(0) {}

Current::Current(Neuron* sourceNeuron, int time, int QueueIndex, float current):Event(time, QueueIndex), SourceNeuron(sourceNeuron), current(current){}


Current::~Current() {}
