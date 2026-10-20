#include "../source_file_realtime_v1_async/Event/inc/TimeEventUpdateNeuron.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuronModel.h"
#include "../source_file_realtime_v1_async/NeuralModel/inc/NeuralStateVector/Neuron_State_Vector.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/TimeDrivenInternalSpike.h"
#include "../source_file_realtime_v1_async/Simulation/inc/Simulation.h"
#include "../source_file_realtime_v1_async/mainprogram/inc/BenchProfiling.h"
TimeEventUpdateNeuron::TimeEventUpdateNeuron(int time, int QueueIndex, NeuronModel* neuronModel, Neuron** neurons)
    : Event(time, QueueIndex), neuronModel(neuronModel), neurons(neurons) {}
TimeEventUpdateNeuron::~TimeEventUpdateNeuron() {}
void TimeEventUpdateNeuron::ProcessEvent(Simulation* simulation) {
    const long long total_start = bench_profile::now_ns();
    int Currenttime = this->getTime();
    const long long update_start = bench_profile::now_ns();
    this->neuronModel->UpdateState(-1, Currenttime, simulation);
    bench_profile::time_event_update_state_ns.fetch_add(bench_profile::now_ns() - update_start, std::memory_order_relaxed);
    const long long spike_start = bench_profile::now_ns();
    TimeDrivenInternalSpike InternalSpikeEvent(Currenttime, this->getIndex(), this->neuronModel->StateVector, this->neuronModel->PropogationStructure, this->neurons);
    InternalSpikeEvent.ProcessEvent(simulation);
    bench_profile::time_event_internal_spike_ns.fetch_add(bench_profile::now_ns() - spike_start, std::memory_order_relaxed);
    const long long reschedule_start = bench_profile::now_ns();
    simulation->EventHeap->Insert_a_Event(new TimeEventUpdateNeuron(Currenttime + this->neuronModel->getTimestepSize(), this->getIndex(), this->neuronModel, this->neurons), this->getIndex());
    bench_profile::time_event_reschedule_ns.fetch_add(bench_profile::now_ns() - reschedule_start, std::memory_order_relaxed);
    bench_profile::time_event_total_ns.fetch_add(bench_profile::now_ns() - total_start, std::memory_order_relaxed);
    bench_profile::time_event_count.fetch_add(1, std::memory_order_relaxed);
}
void TimeEventUpdateNeuron::ProcessEvent(Simulation* simulation, RealTimeRestrictionLevel level) {
    const long long total_start = bench_profile::now_ns();
    int Currenttime = this->getTime();
    const long long update_start = bench_profile::now_ns();
    this->neuronModel->UpdateState(-1, Currenttime, simulation);
    bench_profile::time_event_update_state_ns.fetch_add(bench_profile::now_ns() - update_start, std::memory_order_relaxed);
    const long long spike_start = bench_profile::now_ns();
    TimeDrivenInternalSpike InternalSpikeEvent(Currenttime, this->getIndex(), this->neuronModel->StateVector, this->neuronModel->PropogationStructure, this->neurons);
    InternalSpikeEvent.ProcessEvent(simulation, level);
    bench_profile::time_event_internal_spike_ns.fetch_add(bench_profile::now_ns() - spike_start, std::memory_order_relaxed);
    const long long reschedule_start = bench_profile::now_ns();
    simulation->EventHeap->Insert_a_Event(new TimeEventUpdateNeuron(Currenttime + this->neuronModel->getTimestepSize(), this->getIndex(), this->neuronModel, this->neurons), this->getIndex());
    bench_profile::time_event_reschedule_ns.fetch_add(bench_profile::now_ns() - reschedule_start, std::memory_order_relaxed);
    bench_profile::time_event_total_ns.fetch_add(bench_profile::now_ns() - total_start, std::memory_order_relaxed);
    bench_profile::time_event_count.fetch_add(1, std::memory_order_relaxed);
}
enum EventPriority TimeEventUpdateNeuron::getPriority() {
    return TIMEEVENT;
}
