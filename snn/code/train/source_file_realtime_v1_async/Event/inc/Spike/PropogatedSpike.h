#ifndef PROPAGATEDSPIKE_H
#define PROPAGATEDSPIKE_H
#include "../source_file_realtime_v1_async/Event/inc/Spike/Spike.h"
class PropogatedSpike : public Spike {
public:
    int PropogationDelayIndex;
    int UpperBoundDelayIndex;
    int NSynapses;
    Interconnections* inter;
    PropogatedSpike();
    PropogatedSpike(int time, int QueueIndex, Neuron* Neuron, int PropogationDelayIndex, int UpperBoundDelayIndex);
    ~PropogatedSpike();
    virtual void ProcessEvent(Simulation* simulation);
    virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);
    virtual enum EventPriority getPriority();
};
#endif