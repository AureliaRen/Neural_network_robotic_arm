#ifndef SYNCHRONIZEACTIVITYEVENT_H
#define SYNCHRONIZEACTIVITYEVENT_H
#include "../source_file_realtime_v1_async/Event/inc/Event.h"
class Simulation;
class SynchronizeActivityEvent : public Event {
public:
    SynchronizeActivityEvent(int Newtime, Simulation* simulation);
    ~SynchronizeActivityEvent();
    virtual void ProcessEvent(Simulation* simulation);
    virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);
    virtual enum EventPriority getPriority();
};
#endif