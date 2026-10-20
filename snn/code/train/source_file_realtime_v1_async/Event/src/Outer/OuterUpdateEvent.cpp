#include "../source_file_realtime_v1_async/Event/inc/Outer/OuterUpdateEvent.h"

#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicModel.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"

OuterUpdateEvent::OuterUpdateEvent(int time, int queue_index, OuterDynamicModel* model)
    : Event(time, queue_index), model_(model) {}

OuterUpdateEvent::~OuterUpdateEvent() {}

void OuterUpdateEvent::ProcessEvent(Simulation* simulation) {
    const int current_time = this->getTime();
    this->model_->Update(current_time, simulation);
    simulation->FlushOuterDynamicInputSpikes(current_time);
    simulation->EventHeap->Insert_a_Event(
        new OuterUpdateEvent(
            current_time + this->model_->getTimestepSize(),
            this->model_->getQueueIndex(),
            this->model_),
        this->model_->getQueueIndex());
}

void OuterUpdateEvent::ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level) {
    (void)level;
    this->ProcessEvent(simulation);
}

enum EventPriority OuterUpdateEvent::getPriority() {
    return OUTERUPDATEEVENT;
}
