#pragma once

#include <memory>
#include <string>
#include <functional>
#include <uv.h>
#include "libuv_net/message.hpp"
#include "libuv_net/thread_pool.hpp"
#include <spdlog/spdlog.h>

namespace libuv_net {

/**
 * @brief TCP 客户端类
 * 
 * 提供 TCP 客户端功能，包括：
 * - 连接到服务器
 * - 发送和接收消息
 * - 自动重连
 */
class Client {
public:
    // 回调函数类型定义
    using ConnectHandler = std::function<void()>;          // 连接回调
    using DisconnectHandler = std::function<void()>;       // 断开连接回调
    using MessageHandler = std::function<void(std::shared_ptr<Message>)>;  // 消息处理回调

    Client();
    ~Client();

    // 禁用拷贝构造和赋值
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    /**
     * @brief 连接到服务器
     * @param host 服务器主机名或 IP 地址
     * @param port 服务器端口
     * @return 是否成功发起连接
     */
    bool connect(const std::string& host, uint16_t port);

    /**
     * @brief 断开与服务器的连接
     */
    void disconnect();

    /**
     * @brief 发送消息到服务器
     * @param message 要发送的消息
     */
    void send(std::shared_ptr<Message> message);

    /**
     * @brief 设置连接回调
     * @param handler 回调函数
     */
    void set_connect_handler(ConnectHandler handler) { connect_handler_ = std::move(handler); }

    /**
     * @brief 设置断开连接回调
     * @param handler 回调函数
     */
    void set_disconnect_handler(DisconnectHandler handler) { disconnect_handler_ = std::move(handler); }

    /**
     * @brief 设置消息处理回调
     * @param handler 回调函数
     */
    void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }

private:
    // libuv 回调函数
    static void on_connect(uv_connect_t* req, int status);
    static void on_close(uv_handle_t* handle);
    static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void on_write(uv_write_t* req, int status);
    static uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size);

    // 内部处理函数
    bool init_socket();
    void append_to_buffer(const char* data, size_t len);

    // 成员变量
    uv_loop_t* loop_;                    // libuv 事件循环
    uv_tcp_t socket_;                    // TCP 套接字
    std::unique_ptr<ThreadPool> thread_pool_;  // 线程池

    // 状态标志
    bool is_connected_{false};           // 是否已连接
    bool is_connecting_{false};          // 是否正在连接

    // 回调函数
    ConnectHandler connect_handler_;      // 连接回调
    DisconnectHandler disconnect_handler_;  // 断开连接回调
    MessageHandler message_handler_;      // 消息处理回调

    // 缓冲区
    std::vector<uint8_t> buffer_;        // 接收缓冲区
};

} // namespace libuv_net 