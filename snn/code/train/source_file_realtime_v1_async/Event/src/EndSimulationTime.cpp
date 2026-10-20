#include "../source_file_realtime_v1_async/Event/inc/EndSimulationTime.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InputSpike.h"
#include "../source_file_realtime_v1_async/Event/inc/Synchronize/CommunicationEvent.h"

EndSimulationTime::EndSimulationTime(): Event(0,0){}

EndSimulationTime::EndSimulationTime(int time, int index): Event(time, index){}

EndSimulationTime::~EndSimulationTime(){}

void EndSimulationTime::ProcessEvent(Simulation* simulation) {
	// 首先创建一个 Spike 事件和一个新的神经元
	simulation->EndSimulation(this->getIndex());
}

enum EventPriority EndSimulationTime::getPriority() {
	return ENDSIMULATION;
}