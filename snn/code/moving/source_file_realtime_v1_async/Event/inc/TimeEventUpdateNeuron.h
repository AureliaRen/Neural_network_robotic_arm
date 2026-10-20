#ifndef TIMEEVENTUPDATENEURON_H
#define TIMEEVENTUPDATENEURON_H
#include "../source_file_realtime_v1_async/Event/inc/Event.h"
class NeuronModel;
class Neuron;
class TimeEventUpdateNeuron : public Event {
public:
    NeuronModel* neuronModel;
    Neuron** neurons;
    TimeEventUpdateNeuron(int Time, int QueueIndex, NeuronModel* neuronModel, Neuron** neurons);
    ~TimeEventUpdateNeuron();
    virtual void ProcessEvent(Simulation* simulation);
    virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);
    virtual enum EventPriority getPriority();
};
#endif