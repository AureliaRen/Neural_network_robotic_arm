#include "../source_file_realtime_v1_async/communication/inc/ZMQAsyncInputOutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/communication/inc/ZmqTopicProtocol.h"
#include "../source_file_realtime_v1_async/Event/inc/Spike/InputSpike.h"
#include <cstring>
#include <stdexcept>

using zmq_topic_protocol::AsyncSpikeBatchHeader;

namespace {
static const uint32_t MAX_SPIKES_PER_BATCH = 10'000'000;
}

ZMQAsyncInputOutputSpikeDriver::ZMQAsyncInputOutputSpikeDriver(
    unsigned short publish_port,
    const std::string& subscribe_address,
    unsigned short subscribe_port,
    const std::string& publish_topic,
    const std::string& subscribe_topic)
    : publisher_socket_(new ZmqSocket(ZmqSocket::Mode::PUBLISH, "", static_cast<int>(publish_port))),
      subscriber_socket_(new ZmqSocket(ZmqSocket::Mode::SUBSCRIBE, subscribe_address, static_cast<int>(subscribe_port))),
      publish_topic_(publish_topic),
      subscribe_topic_(subscribe_topic),
      last_time_step_(0) {
    publisher_socket_->setLinger(0);
    subscriber_socket_->setLinger(0);
    subscriber_socket_->subscribe(subscribe_topic_);
    this->isFinished = false;
}

ZMQAsyncInputOutputSpikeDriver::~ZMQAsyncInputOutputSpikeDriver() {
    delete publisher_socket_;
    delete subscriber_socket_;
}

void ZMQAsyncInputOutputSpikeDriver::publishBatch(uint32_t time_step, const std::vector<OutputSpikeIO>& spikes) {
    AsyncSpikeBatchHeader header{};
    header.magic = zmq_topic_protocol::kAsyncSpikeMagic;
    header.version = zmq_topic_protocol::kAsyncSpikeProtocolVersion;
    header.time_step = time_step;
    header.spike_count = static_cast<uint32_t>(spikes.size());

    if (!publisher_socket_->sendString(publish_topic_, true)) {
        throw std::runtime_error("Failed to publish async topic");
    }

    const bool has_payload = !spikes.empty();
    if (!publisher_socket_->sendBuffer(&header, sizeof(header), has_payload)) {
        throw std::runtime_error("Failed to publish async header");
    }

    if (has_payload) {
        const size_t payload_size = spikes.size() * sizeof(OutputSpikeIO);
        if (!publisher_socket_->sendBuffer(spikes.data(), payload_size, false)) {
            throw std::runtime_error("Failed to publish async payload");
        }
    }
}

void ZMQAsyncInputOutputSpikeDriver::pollIncomingBatches() {
    while (true) {
        std::string topic = subscriber_socket_->receiveString(true);
        if (topic.empty()) {
            break;
        }

        AsyncSpikeBatchHeader header{};
        int received = subscriber_socket_->receiveBuffer(&header, sizeof(header));
        if (received != static_cast<int>(sizeof(header))) {
            throw std::runtime_error("Failed to receive async spike header");
        }

        if (topic != subscribe_topic_ ||
            header.magic != zmq_topic_protocol::kAsyncSpikeMagic ||
            header.version != zmq_topic_protocol::kAsyncSpikeProtocolVersion) {
            if (header.spike_count > 0) {
                std::vector<char> discard(static_cast<size_t>(header.spike_count) * sizeof(OutputSpikeIO));
                subscriber_socket_->receiveBuffer(discard.data(), discard.size());
            }
            continue;
        }

        if (header.spike_count > MAX_SPIKES_PER_BATCH) {
            throw std::runtime_error("Async input spike batch too large");
        }

        std::vector<OutputSpikeIO> received_spikes(header.spike_count);
        if (header.spike_count > 0) {
            const size_t payload_size = static_cast<size_t>(header.spike_count) * sizeof(OutputSpikeIO);
            received = subscriber_socket_->receiveBuffer(received_spikes.data(), payload_size);
            if (received != static_cast<int>(payload_size)) {
                throw std::runtime_error("Incomplete async input spike payload");
            }
        }

        std::lock_guard<std::mutex> lock(input_mutex_);
        pending_input_buffer_.insert(
            pending_input_buffer_.end(),
            received_spikes.begin(),
            received_spikes.end());
    }
}

void ZMQAsyncInputOutputSpikeDriver::LoadInputSpike(EventQueue* eventQueue, Network* network, int time) {
    last_time_step_ = static_cast<uint32_t>(time);
    pollIncomingBatches();

    std::vector<OutputSpikeIO> input_spikes;
    {
        std::lock_guard<std::mutex> lock(input_mutex_);
        input_spikes.swap(pending_input_buffer_);
    }

    for (const auto& spike : input_spikes) {
        if (spike.neuron < 0 || spike.neuron >= network->neuronsNum) {
            continue;
        }

        Neuron* target_neuron = &(network->neurons[spike.neuron]);
        if (target_neuron == nullptr) {
            continue;
        }

        int Time = static_cast<int>(spike.time / spike.basetimestepl);
        InputSpike* new_spike = new InputSpike(target_neuron, Time, target_neuron->Queue_index);
        eventQueue->Insert_a_Event(new_spike, new_spike->getIndex());
    }
}

void ZMQAsyncInputOutputSpikeDriver::LoadInputSpike(EventQueue* eventQueue, Network* network, int spikenum, const int* Times, const int* neuron_index) {
    (void)eventQueue;
    (void)network;
    (void)spikenum;
    (void)Times;
    (void)neuron_index;
}

void ZMQAsyncInputOutputSpikeDriver::WriteSpike(Spike* NewSpike, float basetimesteps) {
    if (!NewSpike) {
        return;
    }

    OutputSpikeIO out_spike(NewSpike->SourceNeuron->Neuron_index, NewSpike->getTime(), basetimesteps);
    std::lock_guard<std::mutex> lock(output_mutex_);
    output_buffer_.push_back(out_spike);
}

void ZMQAsyncInputOutputSpikeDriver::WriteState(int Time, Neuron* Source) {
    (void)Time;
    (void)Source;
}

bool ZMQAsyncInputOutputSpikeDriver::IsBuffered() {
    return true;
}

bool ZMQAsyncInputOutputSpikeDriver::IsWritePotentialCapable() {
    return false;
}

void ZMQAsyncInputOutputSpikeDriver::FlushBuffers() {
    std::vector<OutputSpikeIO> outgoing_spikes;
    {
        std::lock_guard<std::mutex> lock(output_mutex_);
        outgoing_spikes.swap(output_buffer_);
    }

    if (outgoing_spikes.size() > MAX_SPIKES_PER_BATCH) {
        throw std::runtime_error("Async output spike batch too large");
    }

    publishBatch(last_time_step_, outgoing_spikes);
}
