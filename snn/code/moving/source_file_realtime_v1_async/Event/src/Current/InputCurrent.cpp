#include "../source_file_realtime_v1_async/Event/inc/Current/InputCurrent.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Event/inc/Current/PropogatedCurrent.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
InputCurrent::InputCurrent():Current(){}

InputCurrent::InputCurrent(Neuron* sourceNeuron, int time, int QueueIndex, float current): Current(sourceNeuron, time, QueueIndex, current){}

InputCurrent::~InputCurrent() {

}

void InputCurrent::ProcessEvent(Simulation* simulation) {
	for (int i = 0; i < simulation->NumberOfQueue; i++) {
		if (this->SourceNeuron->Output_Synaps_Number[i] != 0) {
			PropogatedCurrent* propogatedCurrent = new PropogatedCurrent(this->getTime() + this->SourceNeuron->Output_Synaps[i][0]->delay, i, this->SourceNeuron, 0, this->SourceNeuron->PropogationStructure->NDifferentdelays[i], this->current);
			if (i == this->getIndex()) {
				simulation->EventHeap->Insert_a_Event(propogatedCurrent, i);
			}
			else {
				simulation->EventHeap->Insert_a_Event_to_Buffer(propogatedCurrent, this->getIndex(), i);
			}
		}
	}
}

enum EventPriority InputCurrent::getPriority() {
	return PROPOGATEDCURRENT;
}