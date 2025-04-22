#pragma once

#include <memory>
#include <string>
#include <functional>
#include <uv.h>
#include "libuv_net/protocol.hpp"

namespace libuv_net {

// 会话类，管理单个 TCP 连接
class Session {
public:
    // 消息处理回调函数类型
    using MessageHandler = std::function<void(std::shared_ptr<Message>)>;
    // 关闭处理回调函数类型
    using CloseHandler = std::function<void()>;

    // 构造函数
    Session(uv_loop_t* loop, uv_tcp_t* client);
    ~Session();

    // 禁用拷贝构造和赋值
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // 启动会话
    void start();
    // 停止会话
    void stop();
    // 发送消息
    void send(std::shared_ptr<Message> message);

    // 设置消息处理回调
    void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }
    // 设置关闭处理回调
    void set_close_handler(CloseHandler handler) { close_handler_ = std::move(handler); }

    // 获取会话 ID
    const std::string& get_id() const { return id_; }
    // 获取远程地址
    const std::string& get_remote_address() const { return remote_address_; }
    // 获取远程端口
    uint16_t get_remote_port() const { return remote_port_; }

private:
    // libuv 回调函数
    static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void on_write(uv_write_t* req, int status);
    static void on_close(uv_handle_t* handle);

    // 处理接收缓冲区
    void process_buffer();
    // 处理消息
    void handle_message(std::shared_ptr<Message> message);

    uv_loop_t* loop_; // libuv 事件循环
    uv_tcp_t* client_; // TCP 客户端句柄
    std::string id_; // 会话 ID
    std::string remote_address_; // 远程地址
    uint16_t remote_port_; // 远程端口

    std::vector<uint8_t> read_buffer_; // 读取缓冲区
    std::vector<uint8_t> message_buffer_; // 消息缓冲区

    MessageHandler message_handler_; // 消息处理回调
    CloseHandler close_handler_; // 关闭处理回调

    bool is_closing_{false}; // 是否正在关闭
};

} // namespace libuv_net 