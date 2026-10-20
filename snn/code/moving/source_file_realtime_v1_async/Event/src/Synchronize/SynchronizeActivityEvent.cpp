#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SynchronizeActivityEvent.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Event/inc/Synchronize/SynchronizeSimulationEvent.h"
SynchronizeActivityEvent::SynchronizeActivityEvent(int time, Simulation* simulation) : Event(time, 0) {
    for (int i = 0; i < simulation->NumberOfQueue; i++) {
        SynchronizeSimulationEvent* event = new SynchronizeSimulationEvent(time, i);
        simulation->EventHeap->Insert_a_Event(event, i);
    }
}
SynchronizeActivityEvent::~SynchronizeActivityEvent() {}
void SynchronizeActivityEvent::ProcessEvent(Simulation* simulation) {
    this->ProcessEvent(simulation, ALL_EVENTS_ENABLED);
}
void SynchronizeActivityEvent::ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level) {
    if (simulation->DelayMin >= 1) {
        SynchronizeActivityEvent* event = new SynchronizeActivityEvent(this->getTime() + simulation->DelayMin, simulation);
        simulation->EventHeap->Insert_a_syn_Event(event);
    }
    else if (simulation->DelayMin == 0) {
        if (simulation->EventHeap->is_Buffer_Empty()) {
            SynchronizeActivityEvent* event = new SynchronizeActivityEvent(this->getTime() + 1, simulation);
            simulation->EventHeap->Insert_a_syn_Event(event);
        }
        else {
            SynchronizeActivityEvent* event = new SynchronizeActivityEvent(this->getTime(), simulation);
            simulation->EventHeap->Insert_a_syn_Event(event);
        }
    }
    for (int i = 0; i < simulation->NumberOfQueue; i++) {
        if (level < SPIKES_DISABLED) {
            simulation->EventHeap->Insert_Buffer_to_Event_Queue(i);
        }
        else {
            simulation->EventHeap->Reset_Buffer(i);
        }
    }
}
enum EventPriority SynchronizeActivityEvent::getPriority() {
    return SYNCHRONIZEACTIVITYEVENT;
}