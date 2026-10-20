#include "../source_file_realtime_v1_async/Event/inc/Synchronize/CommunicationEvent.h"
#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SynchronizeSimulationEvent.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
CommunicationEvent::CommunicationEvent(int newtime, int CommunicationInterval, Simulation* sim) : Event(newtime, 0) {
    this->CommunicationTimeInterval = CommunicationInterval;
    for (int i = 0; i < sim->NumberOfQueue; i++) {
        SynchronizeSimulationEvent* syncEvent = new SynchronizeSimulationEvent(newtime, i);
        sim->EventHeap->Insert_a_Event(syncEvent, i);
    }
}
CommunicationEvent::~CommunicationEvent() {}
void CommunicationEvent::ProcessEvent(Simulation* sim) {
    sim->LoadInput(this);
    sim->PublishOutput(this);
    if (this->CommunicationTimeInterval > 0) {
        CommunicationEvent* newEvent = new CommunicationEvent(getTime() + this->CommunicationTimeInterval, this->CommunicationTimeInterval, sim);
        sim->EventHeap->Insert_a_syn_Event(newEvent);
    }
}
void CommunicationEvent::ProcessEvent(Simulation* sim, RealTimeRestrictionLevel level) {
    (void) level;
    this->ProcessEvent(sim);
}
enum EventPriority CommunicationEvent::getPriority() {
    return COMMUNICATION;
}