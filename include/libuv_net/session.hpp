#pragma once

#include <memory>
#include <string>
#include <functional>
#include <uv.h>
#include "libuv_net/protocol.hpp"
#include <vector>
#include "libuv_net/message.hpp"
#include <spdlog/spdlog.h>

namespace libuv_net {

/**
 * @brief 会话类
 * 
 * 表示一个客户端连接会话，提供：
 * - 消息发送和接收
 * - 连接状态管理
 * - 错误处理
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    // 消息处理回调函数类型
    using MessageHandler = std::function<void(std::shared_ptr<Message>)>;
    // 关闭处理回调函数类型
    using CloseHandler = std::function<void()>;
    // 读取处理回调函数类型
    using ReadHandler = std::function<void(ssize_t, const uv_buf_t*)>;

    // 构造函数
    explicit Session(uv_loop_t* loop);
    Session(uv_loop_t* loop, uv_tcp_t* client);
    ~Session();

    // 禁用拷贝构造和赋值
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    /**
     * @brief 获取会话ID
     * @return 会话ID
     */
    std::string id() const { return id_; }

    // 设置回调函数
    void set_alloc_callback(std::function<uv_buf_t(size_t)> callback) {
        alloc_callback_ = std::move(callback);
    }

    void set_read_callback(ReadHandler handler) {
        read_handler_ = std::move(handler);
    }

    void set_close_callback(CloseHandler handler) {
        close_handler_ = std::move(handler);
    }

    // 启动会话
    void start() {
        start_read();
    }

    // 开始读取数据
    int start_read();
    // 停止读取数据
    void stop();
    // 关闭会话
    void close();
    // 发送消息
    void send(std::shared_ptr<Message> message);

    // 获取底层 socket
    uv_tcp_t& get_socket() { return socket_; }

    // 获取远程地址
    const std::string& get_remote_address() const { return remote_address_; }
    // 获取远程端口
    uint16_t get_remote_port() const { return remote_port_; }

    // 设置消息处理回调
    void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }
    // 设置关闭处理回调
    void set_close_handler(CloseHandler handler) { close_handler_ = std::move(handler); }

    // 处理接收到的数据
    void append_to_buffer(const char* data, size_t len);

private:
    static void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void on_close(uv_handle_t* handle);
    static void on_write(uv_write_t* req, int status);

    // 处理接收到的数据
    void process_buffer();
    // 处理消息
    void handle_message(std::shared_ptr<Message> message);

    uv_loop_t* loop_;
    uv_tcp_t socket_;
    std::string id_;
    std::function<uv_buf_t(size_t)> alloc_callback_;
    ReadHandler read_handler_;
    CloseHandler close_handler_;
    bool is_closing_ = false;

    std::string remote_address_; // 远程地址
    uint16_t remote_port_; // 远程端口

    std::vector<uint8_t> read_buffer_; // 读取缓冲区
    std::vector<uint8_t> message_buffer_; // 消息缓冲区

    MessageHandler message_handler_; // 消息处理回调
};

} // namespace libuv_net 