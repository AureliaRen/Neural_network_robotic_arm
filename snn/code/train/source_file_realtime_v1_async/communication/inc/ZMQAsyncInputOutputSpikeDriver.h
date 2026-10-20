/*
* 鏂囦欢鍚嶏細ZMQAsyncInputOutputSpikeDriver.h
* 閫氳繃ZMQ鍙戝竷/璁㈤槄妯″紡瀹炵幇寮傛鑴夊啿杈撳叆杈撳嚭
*/
#ifndef ZMQASYNCINPUTOUTPUTSPIKEDRIVER_H
#define ZMQASYNCINPUTOUTPUTSPIKEDRIVER_H

#include "../source_file_realtime_v1_async/communication/inc/OutputSpikeDriver.h"
#include "../source_file_realtime_v1_async/InputSpikeDriver/inc/InputSpikeDriver.h"
#include "../source_file_realtime_v1_async/communication/inc/ZmqSocket.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

class ZMQAsyncInputOutputSpikeDriver : public InputSpikeDriver, public OutputSpikeDriver {
private:
    struct OutputSpikeIO {
        int neuron;
        float time;
        float basetimestepl;

        OutputSpikeIO() = default;
        OutputSpikeIO(int neuronID, int spiketime, float basetimestep)
            : neuron(neuronID), time(spiketime * basetimestep), basetimestepl(basetimestep) {}
    };

    ZmqSocket* publisher_socket_;
    ZmqSocket* subscriber_socket_;

    std::string publish_topic_;
    std::string subscribe_topic_;

    std::vector<OutputSpikeIO> output_buffer_;
    std::vector<OutputSpikeIO> pending_input_buffer_;

    std::mutex output_mutex_;
    std::mutex input_mutex_;

    uint32_t last_time_step_;

    void publishBatch(uint32_t time_step, const std::vector<OutputSpikeIO>& spikes);
    void pollIncomingBatches();

public:
    ZMQAsyncInputOutputSpikeDriver(
        unsigned short publish_port,
        const std::string& subscribe_address,
        unsigned short subscribe_port,
        const std::string& publish_topic,
        const std::string& subscribe_topic);

    ~ZMQAsyncInputOutputSpikeDriver();

    virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int time) override;
    virtual void LoadInputSpike(EventQueue* eventQueue, Network* network, int spikenum, const int* Times, const int* neuron_index) override;
    virtual void WriteSpike(Spike* NewSpike, float basetimesteps) override;
    virtual void WriteState(int Time, Neuron* Source) override;
    virtual bool IsBuffered() override;
    virtual bool IsWritePotentialCapable() override;
    virtual void FlushBuffers() override;
};

#endif // ZMQASYNCINPUTOUTPUTSPIKEDRIVER_H
