#include "../source_file_realtime_v1_async/Event/inc/Spike/PropogatedSpike.h"
#include "../source_file_realtime_v1_async/connections/inc/Interconnections.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InternalSpike.h"
#include "../source_file_realtime_v1_async/LearningRule/inc/LearningRule.h"
PropogatedSpike::PropogatedSpike() : Spike() {}
PropogatedSpike::PropogatedSpike(int time, int QueueIndex, Neuron* Neuron, int PropogationDelayIndex, int UpperBoundDelayIndex)
    : Spike(Neuron, time, QueueIndex), PropogationDelayIndex(PropogationDelayIndex), UpperBoundDelayIndex(UpperBoundDelayIndex) {
    this->inter = Neuron->PropogationStructure->interconnections[QueueIndex][PropogationDelayIndex];
    this->NSynapses = Neuron->PropogationStructure->NInterconnections[QueueIndex][PropogationDelayIndex];
}
PropogatedSpike::~PropogatedSpike() {}
void PropogatedSpike::ProcessEvent(Simulation* simulation) {
    this->ProcessEvent(simulation, ALL_EVENTS_ENABLED);
}
void PropogatedSpike::ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level) {
    if (level >= SPIKES_DISABLED) {
        return;
    }
    InternalSpike* Generate;
    LearningRule* lr;
    for (int i = 0; i < this->NSynapses; i++) {
        Generate = this->inter->TargetNeuronModel->ProcessSpike(inter, this->getTime());
        if (Generate != 0) {
            simulation->EventHeap->Insert_a_Event(Generate, this->getIndex());
        }
        if (level < LEARNING_RULES_DISABLED) {
            lr = inter->LearningRule_withPost;
            if (lr != 0) {
                lr->ApplyPreSynaticSpike(inter, this->getTime(), simulation);
            }
            lr = inter->LearningRule_withTrigger;
            if (lr != 0) {
                lr->ApplyPreSynaticSpike(inter, this->getTime(), simulation);
            }
            lr = inter->LearningRule_withPostAndTrigger;
            if (lr != 0) {
                lr->ApplyPreSynaticSpike(inter, this->getTime(), simulation);
            }
        }
        inter++;
    }
    if (this->UpperBoundDelayIndex > this->PropogationDelayIndex + 1) {
        PropogatedSpike* Spike = new PropogatedSpike(
            this->getTime() - this->SourceNeuron->PropogationStructure->SynapseDelay[this->getIndex()][this->PropogationDelayIndex] +
            this->SourceNeuron->PropogationStructure->SynapseDelay[this->getIndex()][this->PropogationDelayIndex + 1],
            this->getIndex(), this->SourceNeuron, this->PropogationDelayIndex + 1, this->UpperBoundDelayIndex);
        simulation->EventHeap->Insert_a_Event(Spike, this->getIndex());
    }
}
enum EventPriority PropogatedSpike::getPriority() {
    return PROPOGATEDSPIKE;
}