#include "../source_file_realtime_v1_async/communication/inc/ZmqSocket.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>

// 静态辅助函数：获取套接字类型
static zmq::socket_type getSocketType(ZmqSocket::Mode mode) {
    switch (mode) {
    case ZmqSocket::Mode::REQUEST:
        return zmq::socket_type::req;
    case ZmqSocket::Mode::REPLY:
        return zmq::socket_type::rep;
    case ZmqSocket::Mode::PUBLISH:
        return zmq::socket_type::pub;
    case ZmqSocket::Mode::SUBSCRIBE:
        return zmq::socket_type::sub;
    case ZmqSocket::Mode::PUSH:
        return zmq::socket_type::push;
    case ZmqSocket::Mode::PULL:
        return zmq::socket_type::pull;
    default:
        throw std::invalid_argument("Unsupported socket mode");
    }
}

// 验证发送操作是否有效
void ZmqSocket::validateSendOperation() const {
    // 所有模式都支持发送
}

// 验证接收操作是否有效
void ZmqSocket::validateReceiveOperation() const {
    // 所有模式都支持接收
}

ZmqSocket::ZmqSocket(Mode mode, const std::string& address, int port)
    : context_(1), mode_(mode) {

    // 创建套接字
    socket_ = std::make_unique<zmq::socket_t>(context_, getSocketType(mode));

    // 构建地址
    if (mode == Mode::PUBLISH || mode == Mode::REPLY || mode == Mode::PUSH) {
        // 发布者、响应者、推送者绑定地址
        address_ = "tcp://*:" + std::to_string(port);
        socket_->bind(address_);

        // 对于发布者，给订阅者一些时间连接
        if (mode == Mode::PUBLISH) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    else {
        // 订阅者、请求者、拉取者通常连接到地址
        address_ = "tcp://" + address + ":" + std::to_string(port);
        socket_->connect(address_);
    }

    std::cout << "ZMQ Socket created ["
        << (mode == Mode::REQUEST ? "REQUEST" :
            mode == Mode::REPLY ? "REPLY" :
            mode == Mode::PUBLISH ? "PUBLISH" :
            mode == Mode::SUBSCRIBE ? "SUBSCRIBE" :
            mode == Mode::PUSH ? "PUSH" : "PULL")
        << "]: " << address_ << std::endl;
}

ZmqSocket::~ZmqSocket() {
    try {
        if (socket_) {
            // The default linger can block close forever if a peer is gone.
            socket_->set(zmq::sockopt::linger, 0);
            socket_->close();
            socket_.reset();
        }

        context_.shutdown();
        context_.close();
    }
    catch (const zmq::error_t& e) {
        std::cerr << "Error destroying ZMQ socket: " << e.what() << std::endl;
    }
}

bool ZmqSocket::sendBuffer(const void* buffer, size_t buffer_size, bool more_parts) {
    try {
        validateSendOperation();

        zmq::message_t msg(buffer_size);
        std::memcpy(msg.data(), buffer, buffer_size);

        zmq::send_flags flags = more_parts ? zmq::send_flags::sndmore : zmq::send_flags::none;
        auto result = socket_->send(msg, flags);
        return static_cast<bool>(result);
    }
    catch (const zmq::error_t& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
        return false;
    }
}

int ZmqSocket::receiveBuffer(void* buffer, size_t buffer_size, bool non_blocking) {
    try {
        validateReceiveOperation();

        zmq::recv_flags flags = non_blocking ? zmq::recv_flags::dontwait : zmq::recv_flags::none;
        zmq::message_t msg;
        auto result = socket_->recv(msg, flags);

        if (!result) {
            return -1; // 没收到信息
        }

        size_t recv_size = std::min(buffer_size, msg.size());
        std::memcpy(buffer, msg.data(), recv_size);

        return static_cast<int>(recv_size);
    }
    catch (const zmq::error_t& e) {
        if (!non_blocking || e.num() != EAGAIN) {
            std::cerr << "Error receiving message: " << e.what() << std::endl;
        }
        return -1;
    }
}

bool ZmqSocket::sendString(const std::string& message, bool more_parts) {
    return sendBuffer(message.c_str(), message.size(), more_parts);
}

std::string ZmqSocket::receiveString(bool non_blocking) {
    try {
        validateReceiveOperation();

        zmq::recv_flags flags = non_blocking ? zmq::recv_flags::dontwait : zmq::recv_flags::none;
        zmq::message_t msg;
        auto result = socket_->recv(msg, flags);

        if (!result) {
            return "";
        }

        return std::string(static_cast<char*>(msg.data()), msg.size());
    }
    catch (const zmq::error_t& e) {
        if (!non_blocking || e.num() != EAGAIN) {
            std::cerr << "Error receiving string: " << e.what() << std::endl;
        }
        return "";
    }
}

// 发布-订阅专用方法 - 修正版本
void ZmqSocket::subscribe(const std::string& topic) {
    if (mode_ != Mode::SUBSCRIBE) {
        throw std::logic_error("subscribe() can only be called on SUBSCRIBE mode sockets");
    }

    if (topic.empty()) {
        socket_->set(zmq::sockopt::subscribe, "");
    }
    else {
        socket_->set(zmq::sockopt::subscribe, topic.c_str());
    }
}

void ZmqSocket::unsubscribe(const std::string& topic) {
    if (mode_ != Mode::SUBSCRIBE) {
        throw std::logic_error("unsubscribe() can only be called on SUBSCRIBE mode sockets");
    }

    if (topic.empty()) {
        socket_->set(zmq::sockopt::unsubscribe, "");
    }
    else {
        socket_->set(zmq::sockopt::unsubscribe, topic.c_str());
    }
}

// === 具体的选项设置方法 ===

void ZmqSocket::setLinger(int linger_ms) {
    socket_->set(zmq::sockopt::linger, linger_ms);
}

void ZmqSocket::setSendBufferSize(int size) {
    socket_->set(zmq::sockopt::sndbuf, size);
}

void ZmqSocket::setReceiveBufferSize(int size) {
    socket_->set(zmq::sockopt::rcvbuf, size);
}

// 多部分消息处理
bool ZmqSocket::sendMultiPart(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return false;
    }

    // 发送前面的部分（带SNDMORE标志）
    for (size_t i = 0; i < parts.size() - 1; ++i) {
        if (!sendString(parts[i], true)) {
            return false;
        }
    }

    // 发送最后一部分（不带SNDMORE标志）
    return sendString(parts.back(), false);
}

std::vector<std::string> ZmqSocket::receiveMultiPart(bool non_blocking) {
    std::vector<std::string> parts;
    zmq::recv_flags flags = non_blocking ? zmq::recv_flags::dontwait : zmq::recv_flags::none;

    while (true) {
        zmq::message_t msg;
        auto result = socket_->recv(msg, flags);

        if (!result) {
            if (parts.empty()) {
                return {}; // 没有收到任何部分
            }
            break;
        }

        parts.emplace_back(static_cast<char*>(msg.data()), msg.size());

        // 检查是否还有更多部分
        if (!socket_->get(zmq::sockopt::rcvmore)) {
            break;
        }
    }

    return parts;
}
