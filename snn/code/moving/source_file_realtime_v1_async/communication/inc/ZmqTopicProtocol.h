#ifndef ZMQ_TOPIC_PROTOCOL_H
#define ZMQ_TOPIC_PROTOCOL_H

#include <cstdint>

namespace zmq_topic_protocol {

constexpr uint32_t kAsyncSpikeMagic = 0x53504B31u; // "SPK1"
constexpr uint32_t kAsyncSpikeProtocolVersion = 1u;

constexpr const char* kDefaultSnnOutputTopic = "/snn/output_spikes";
constexpr const char* kDefaultRobotInputTopic = "/robot/input_spikes";

struct AsyncSpikeBatchHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t time_step;
    uint32_t spike_count;
};

static_assert(sizeof(AsyncSpikeBatchHeader) == 16, "AsyncSpikeBatchHeader size mismatch");

} // namespace zmq_topic_protocol

#endif // ZMQ_TOPIC_PROTOCOL_H
