#ifndef ZMQ_SOCKET_H
#define ZMQ_SOCKET_H

#include <zmq.hpp>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

class ZmqSocket {
public:
    //定义了一个枚举变量用于处理不同的请求模式
    enum class Mode {
        REQUEST,      // REQ - 请求模式（客户端）
        REPLY,        // REP - 响应模式（服务器）
        PUBLISH,      // PUB - 发布模式
        SUBSCRIBE,    // SUB - 订阅模式
        PUSH,         // PUSH - 推送模式
        PULL          // PULL - 拉取模式
    };

private:
    //zmq上下文对象
    zmq::context_t context_;
    //一个指向zmq套接字类的智能指针
    std::unique_ptr<zmq::socket_t> socket_;
    //套接字地址
    std::string address_;
    //Socket类型
    Mode mode_;
    /*
    * 辅助方法：检查操作是否在当前模式下有效
    */
    void validateSendOperation() const;
    void validateReceiveOperation() const;

public:
    /*
    * 构造函数：根据给定的模式、地址和端口创建一个ZmqSocket对象
    */
    ZmqSocket(Mode mode, const std::string& address, int port);

    /*
    * 析构函数
    */
    ~ZmqSocket();

    // === 通用发送/接收方法 ===

    /*
    * 发送二进制数据
    */
    bool sendBuffer(const void* buffer, size_t buffer_size, bool more_parts = false);

    /*
    * 接受二进制数据
    */
    int receiveBuffer(void* buffer, size_t buffer_size, bool non_blocking = false);

    /*
    * 发送字符串消息
    */
    bool sendString(const std::string& message, bool more_parts = false);

    /*
    * 接受字符串消息
    */
    std::string receiveString(bool non_blocking = false);

    // === 发布-订阅专用方法 ===

    /*
    * 订阅指定的主题
    */
    void subscribe(const std::string& topic = "");

    /*
    * 退订指定主题
    */
    void unsubscribe(const std::string& topic);

    // === 具体的套接字选项设置（替代通用方法）===

    // 设置 linger 选项
    void setLinger(int linger_ms);

    /*
    * 设置发送缓冲区大小
    */
    void setSendBufferSize(int size);

    /*
    * 设置接受缓冲区大小
    */
    void setReceiveBufferSize(int size);

    // === 多部分消息处理 ===

    /*
    * 发送多部分消息
    */
    bool sendMultiPart(const std::vector<std::string>& parts);

    /*
    * 接受多部分消息
    */
    std::vector<std::string> receiveMultiPart(bool non_blocking = false);

    // === 工具方法 ===

    /*
    * 查询当前模式
    */
    Mode getMode() const { return mode_; }

    /*
    * 获取链接地址
    */
    std::string getAddress() const { return address_; }
};

#endif // ZMQ_SOCKET_H
