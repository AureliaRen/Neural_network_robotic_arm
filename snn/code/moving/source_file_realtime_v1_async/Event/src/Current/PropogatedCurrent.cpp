#include "../source_file_realtime_v1_async/Event/inc/Current/PropogatedCurrent.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InternalSpike.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"

PropogatedCurrent::PropogatedCurrent():Current(){}

PropogatedCurrent::PropogatedCurrent(int time, int QueueIndex, Neuron* neuron, int PropogationDelayIndex, int UpperBoundDelayIndex, float current) : Current(neuron, time, QueueIndex, current), PropogationDelayIndex(PropogationDelayIndex), UpperBoundDelayIndex(UpperBoundDelayIndex) {
	this->inter = neuron->PropogationStructure->interconnections[QueueIndex][PropogationDelayIndex];
	this->NSynapses = neuron->PropogationStructure->NInterconnections[QueueIndex][PropogationDelayIndex];
}

PropogatedCurrent::~PropogatedCurrent(){}

void PropogatedCurrent::ProcessEvent(Simulation* simulation) {
	InternalSpike* Generate;
	for (int i = 0; i < this->NSynapses; i++) {
		this->inter->TargetNeuronModel->ProcessCurrent(this->inter, this->inter->TargetNeuron, this->current);
		this->inter++;
	}
	if (this->UpperBoundDelayIndex > this->PropogationDelayIndex + 1) {
		PropogatedCurrent* currentEvent = new PropogatedCurrent(this->getTime() - this->SourceNeuron->PropogationStructure->SynapseDelay[this->getIndex()][this->PropogationDelayIndex] + this->SourceNeuron->PropogationStructure->SynapseDelay[this->getIndex()][this->PropogationDelayIndex + 1], this->getIndex(), this->SourceNeuron, this->PropogationDelayIndex + 1, this->UpperBoundDelayIndex, this->current);
		simulation->EventHeap->Insert_a_Event(currentEvent, this->getIndex());
	}

}

enum EventPriority PropogatedCurrent::getPriority() {
	return PROPOGATEDCURRENT;
}
