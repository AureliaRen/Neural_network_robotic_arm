#ifndef OUTER_DYNAMIC_SPIKE_BUFFER_H
#define OUTER_DYNAMIC_SPIKE_BUFFER_H

#include <mutex>
#include <unordered_set>
#include <vector>

class OuterDynamicSpikeBuffer {
public:
    struct SpikeRecord {
        int time = 0;
        int neuron = -1;
    };

    void RegisterWatchedNeurons(const std::vector<int>& neuron_ids);
    void RecordSpike(int time, int neuron_id);
    int CountSpikesInWindow(int start_time, int end_time, const std::vector<int>& neuron_ids) const;
    void DiscardOlderThan(int cutoff_time);
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_set<int> watched_neurons_;
    std::vector<SpikeRecord> spikes_;
};

#endif
