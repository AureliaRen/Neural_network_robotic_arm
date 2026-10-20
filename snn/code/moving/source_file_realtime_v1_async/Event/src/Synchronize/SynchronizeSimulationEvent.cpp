#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SynchronizeSimulationEvent.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"

SynchronizeSimulationEvent::SynchronizeSimulationEvent(int newtime, int QueueIndex): Event(newtime, QueueIndex){}


SynchronizeSimulationEvent::~SynchronizeSimulationEvent(){}


void SynchronizeSimulationEvent::ProcessEvent(Simulation* sim) {
	sim->SyncThread[this->getIndex()] = true;
}

enum EventPriority SynchronizeSimulationEvent::getPriority() {
	return SYNCHRONIZEACTIVITYEVENT;
}
