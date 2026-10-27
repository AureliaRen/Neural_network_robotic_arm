// spike_rx_demo.cpp —— C++ 接收端示例: 订阅 v1_to_spikes(Python) 发出的 SPK1 脉冲流
// 用途: 演示/验证 net_package SNN 侧如何接收视觉线的 ZMQ 输出; 解析逻辑与
//       ZMQAsyncInputOutputSpikeDriver::pollIncomingBatches + LoadInputSpike 一致。
// 用法: spike_rx_demo.exe [--endpoint tcp://127.0.0.1:5566] [--topic /robot/input_spikes]
// 协议: [topic][16B 头 SPK1,ver,t_step,count][n×12B {i32 neuron, f32 time 秒, f32 basetimestep 秒}]
//       空批只有前两帧(不出现第三帧)。
// 步数换算: Time = (int)(time / basetimestep) —— 与 C++ 驱动 LoadInputSpike 完全相同。
#include <zmq.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

constexpr uint32_t kMagic = 0x53504B31u;  // "SPK1"
constexpr uint32_t kVersion = 1;

#pragma pack(push, 1)
struct BatchHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t time_step;
    uint32_t spike_count;
};
struct SpikeIO {
    int32_t neuron;
    float time;
    float basetimestep;
};
#pragma pack(pop)
static_assert(sizeof(BatchHeader) == 16, "header must be 16 bytes");
static_assert(sizeof(SpikeIO) == 12, "spike must be 12 bytes");

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    setvbuf(stdout, nullptr, _IONBF, 0);  // 无缓冲: 重定向到文件/管道时实时可见
    std::string endpoint = "tcp://127.0.0.1:5566";
    std::string topic = "/robot/input_spikes";
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s == "--endpoint" && i + 1 < argc) endpoint = argv[++i];
        else if (s == "--topic" && i + 1 < argc) topic = argv[++i];
        else {
            std::printf("usage: spike_rx_demo.exe [--endpoint tcp://127.0.0.1:5566] "
                        "[--topic /robot/input_spikes]\n");
            return 1;
        }
    }

    zmq::context_t ctx(1);
    zmq::socket_t sub(ctx, zmq::socket_type::sub);
    sub.connect(endpoint);
    sub.set(zmq::sockopt::subscribe, topic);
    std::printf("SUB connect %s topic %s\nwaiting for batches (Ctrl+C to stop)...\n",
                endpoint.c_str(), topic.c_str());

    long long nBatch = 0, nSpike = 0, bad = 0;
    for (;;) {
        std::vector<zmq::message_t> frames;
        try {
            for (;;) {  // 手工收 multipart: cppzmq 4.10 无 recv_multipart 自由函数
                zmq::message_t msg;
                auto r = sub.recv(msg, zmq::recv_flags::none);
                if (!r) break;
                frames.push_back(std::move(msg));
                if (!sub.get(zmq::sockopt::rcvmore)) break;
            }
        } catch (const zmq::error_t& e) {
            std::printf("recv error: %s\n", e.what());
            break;
        }
        if (frames.empty()) continue;

        // 帧数: 2=空批, 3=有脉冲; 其它判坏
        if (frames.size() != 2 && frames.size() != 3) {
            std::printf("[BAD] frame count: %zu\n", frames.size());
            ++bad;
            continue;
        }
        std::string gotTopic(static_cast<char*>(frames[0].data()), frames[0].size());
        if (gotTopic != topic) {
            std::printf("[BAD] topic: %s\n", gotTopic.c_str());
            ++bad;
            continue;
        }
        BatchHeader hdr{};
        std::memcpy(&hdr, frames[1].data(), sizeof(hdr));
        if (hdr.magic != kMagic || hdr.version != kVersion) {
            std::printf("[BAD] magic/version: %08X %u\n", hdr.magic, hdr.version);
            ++bad;
            continue;
        }
        const size_t bodyBytes = frames.size() == 3 ? frames[2].size() : 0;
        if (bodyBytes != hdr.spike_count * sizeof(SpikeIO)) {
            std::printf("[BAD] body len %zu != count %u\n", bodyBytes, hdr.spike_count);
            ++bad;
            continue;
        }

        ++nBatch;
        nSpike += hdr.spike_count;
        if (hdr.spike_count == 0) {
            std::printf("batch#%-3lld hdr_step=%-4u spikes=0 (空批)\n", nBatch,
                        hdr.time_step);
            continue;
        }

        const auto* body = static_cast<const uint8_t*>(frames[2].data());
        int64_t firstStep = 0, lastStep = 0;
        std::printf("batch#%-3lld hdr_step=%-4u spikes=%-2u |", nBatch, hdr.time_step,
                    hdr.spike_count);
        for (uint32_t i = 0; i < hdr.spike_count; ++i) {
            SpikeIO s{};
            std::memcpy(&s, body + i * sizeof(SpikeIO), sizeof(s));
            // ← 与 C++ 驱动 LoadInputSpike 相同: Time = (int)(time / basetimestep)
            const int step = s.basetimestep > 0.0f
                                 ? static_cast<int>(s.time / s.basetimestep)
                                 : -1;
            if (i == 0) firstStep = step;
            lastStep = step;
            std::printf(" 步%d(%d)", step, s.neuron);
        }
        std::printf(" | 事件步 %lld..%lld\n", static_cast<long long>(firstStep),
                    static_cast<long long>(lastStep));
    }

    std::printf("\ndone: %lld batches, %lld spikes, %lld bad\n", nBatch, nSpike, bad);
    return 0;
}
