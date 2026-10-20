#ifndef OUTER_UPDATE_EVENT_H
#define OUTER_UPDATE_EVENT_H

#include "../source_file_realtime_v1_async/Event/inc/Event.h"

class OuterDynamicModel;

class OuterUpdateEvent : public Event {
public:
    OuterUpdateEvent(int time, int queue_index, OuterDynamicModel* model);
    ~OuterUpdateEvent();

    virtual void ProcessEvent(Simulation* simulation);
    virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);
    virtual enum EventPriority getPriority();

private:
    OuterDynamicModel* model_;
};

#endif
