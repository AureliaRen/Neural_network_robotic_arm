#ifndef EVENT_H
#define EVENT_H
#include <iostream>
#include "../source_file_realtime_v1_async/Simulation/inc/RealTimeRestriction.h"
class Simulation;
enum EventPriority {ENDSIMULATION, COMMUNICATION, SYNCHRONIZEACTIVITYEVENT, SYNCHRONIZESIMULATIONEVENT, SAVEWEIGHTEVENT, INTERNALSPIKE, TIMEEVENT, OUTERUPDATEEVENT, PROPOGATEDSPIKE, PROPOGATEDCURRENT};
class Event {
public:
    Event();
    Event(int time, int index);
    virtual ~Event();
    int getTime();
    void setTime(int time);
    int getIndex();
    virtual void ProcessEvent(Simulation* simulation) = 0;
    virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level) {
        (void) level;
        this->ProcessEvent(simulation);
    }
    virtual enum EventPriority getPriority() = 0;
private:
    int time;
    int QueueIndex;
};
#endif
