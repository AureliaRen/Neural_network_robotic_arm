#ifndef TIME_DRIVEN_INTERNAL_SPIKE_H
#define TIME_DRIVEN_INTERNAL_SPIKE_H
#include "../source_file_realtime_v1_async/Event/inc/Spike/InternalSpike.h"
class PropogatedSpikeGroup;
class NeuronModelPropogationStructure;
class Neuron;
class TimeDrivenInternalSpike : public InternalSpike {
public:
    Neuron_State_Vector* neuron_state_vector;
    NeuronModelPropogationStructure* neuronModelPropogationStructure;
    Neuron** Neurons;
    PropogatedSpikeGroup*** propogatedSpikeGroup;
    TimeDrivenInternalSpike(int time, int QueueIndex, Neuron_State_Vector* State, NeuronModelPropogationStructure* ModelPropogationStructure, Neuron** neuron_array);
    ~TimeDrivenInternalSpike();
    virtual void ProcessEvent(Simulation* simulation);
    virtual void ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level);
    void ProcessInternalSpikeEvent(Simulation* simulation, int index);
    void ProcessInternalSpikeEvent(Simulation* simulation, int index, RealTimeRestrictionLevel level);
    virtual enum EventPriority getPriority();
};
#endif