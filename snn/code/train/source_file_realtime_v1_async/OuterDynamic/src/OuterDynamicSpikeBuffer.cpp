#include "../source_file_realtime_v1_async/OuterDynamic/inc/OuterDynamicSpikeBuffer.h"

#include <algorithm>
#include <unordered_set>

void OuterDynamicSpikeBuffer::RegisterWatchedNeurons(const std::vector<int>& neuron_ids) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    for (int neuron_id : neuron_ids) {
        this->watched_neurons_.insert(neuron_id);
    }
}

void OuterDynamicSpikeBuffer::RecordSpike(int time, int neuron_id) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    if (!this->watched_neurons_.empty() && this->watched_neurons_.find(neuron_id) == this->watched_neurons_.end()) {
        return;
    }
    SpikeRecord record;
    record.time = time;
    record.neuron = neuron_id;
    this->spikes_.push_back(record);
}

int OuterDynamicSpikeBuffer::CountSpikesInWindow(int start_time, int end_time, const std::vector<int>& neuron_ids) const {
    if (start_time >= end_time || neuron_ids.empty()) {
        return 0;
    }

    std::unordered_set<int> local_set(neuron_ids.begin(), neuron_ids.end());
    int count = 0;
    std::lock_guard<std::mutex> lock(this->mutex_);
    for (const SpikeRecord& spike : this->spikes_) {
        if (spike.time >= start_time && spike.time < end_time && local_set.find(spike.neuron) != local_set.end()) {
            ++count;
        }
    }
    return count;
}

void OuterDynamicSpikeBuffer::DiscardOlderThan(int cutoff_time) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->spikes_.erase(
        std::remove_if(
            this->spikes_.begin(),
            this->spikes_.end(),
            [cutoff_time](const SpikeRecord& spike) { return spike.time < cutoff_time; }),
        this->spikes_.end());
}

void OuterDynamicSpikeBuffer::Clear() {
    std::lock_guard<std::mutex> lock(this->mutex_);
    this->spikes_.clear();
}
