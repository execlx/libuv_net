#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <uv.h>
#include "libuv_net/session.hpp"
#include "libuv_net/thread_pool.hpp"

namespace libuv_net {

// 服务器类，管理 TCP 服务器和会话
class Server {
public:
    // 会话处理回调函数类型
    using SessionHandler = std::function<void(std::shared_ptr<Session>)>;
    // 消息处理回调函数类型
    using MessageHandler = std::function<void(std::shared_ptr<Session>, std::shared_ptr<Message>)>;

    // 构造函数
    Server();
    ~Server();

    // 禁用拷贝构造和赋值
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // 启动服务器
    bool start(const std::string& host, uint16_t port);
    // 停止服务器
    void stop();

    // 设置会话处理回调
    void set_session_handler(SessionHandler handler) { session_handler_ = std::move(handler); }
    // 设置消息处理回调
    void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }

    // 广播消息到所有会话
    void broadcast(std::shared_ptr<Message> message);
    // 发送消息到指定会话
    void send_to(const std::string& session_id, std::shared_ptr<Message> message);

private:
    // libuv 回调函数
    static void on_new_connection(uv_stream_t* server, int status);
    static void on_close(uv_handle_t* handle);

    // 处理新会话
    void handle_new_session(uv_tcp_t* client);
    // 处理会话关闭
    void handle_session_close(const std::string& session_id);

    uv_loop_t* loop_; // libuv 事件循环
    uv_tcp_t server_; // TCP 服务器句柄
    std::unique_ptr<ThreadPool> thread_pool_; // 线程池

    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_; // 会话映射表
    std::mutex sessions_mutex_; // 会话映射表互斥锁

    SessionHandler session_handler_; // 会话处理回调
    MessageHandler message_handler_; // 消息处理回调

    bool is_running_{false}; // 服务器是否正在运行
};

} // namespace libuv_net 