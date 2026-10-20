#ifndef COMMUNICATION_EVENT_H
#define COMMUNICATION_EVENT_H
#include "../source_file_realtime_v1_async/Event/inc/Event.h"
class CommunicationEvent : public Event {
public:
    int CommunicationTimeInterval;
    CommunicationEvent(int time, int CommunicationInterval, Simulation* simulation);
    ~CommunicationEvent();
    virtual void ProcessEvent(Simulation* simulation);
    virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);
    virtual enum EventPriority getPriority();
};
#endif