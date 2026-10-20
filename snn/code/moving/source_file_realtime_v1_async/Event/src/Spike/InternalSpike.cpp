#include "../source_file_realtime_v1_async/Event/inc/Spike/InternalSpike.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"	

InternalSpike::InternalSpike():Spike(){}

InternalSpike::InternalSpike(Neuron* neuron, int time, int QueueIndex):Spike(neuron, time, QueueIndex){}


InternalSpike::~InternalSpike(){}
