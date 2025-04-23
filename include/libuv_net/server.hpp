#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <uv.h>
#include "libuv_net/session.hpp"
#include "libuv_net/thread_pool.hpp"
#include <iostream>

namespace libuv_net
{

    /**
     * @brief TCP 服务器类
     *
     * 提供 TCP 服务器功能，包括：
     * - 监听端口并接受连接
     * - 管理多个客户端会话
     * - 广播消息
     * - 发送消息到指定会话
     */
    class Server
    {
    public:
        // 回调函数类型定义
        using SessionHandler = std::function<void(std::shared_ptr<Session>)>; // 会话处理回调
        using MessageHandler = std::function<void(std::shared_ptr<Message>)>; // 消息处理回调

        Server();
        ~Server();

        // 禁用拷贝构造和赋值
        Server(const Server &) = delete;
        Server &operator=(const Server &) = delete;

        /**
         * @brief 启动事件循环
         * @return 是否成功启动
         */
        bool start();

        /**
         * @brief 停止事件循环
         */
        void stop();

        /**
         * @brief 启动服务器
         * @param host 监听主机名或 IP 地址
         * @param port 监听端口
         */
        void listen(const std::string &host, int port);

        /**
         * @brief 停止服务器
         */
        void stop_listening();

        /**
         * @brief 设置连接处理回调
         * @param handler 回调函数
         */
        void set_connect_handler(SessionHandler handler) { connect_handler_ = std::move(handler); }

        /**
         * @brief 设置关闭处理回调
         * @param handler 回调函数
         */
        void set_close_handler(SessionHandler handler) { close_handler_ = std::move(handler); }

        /**
         * @brief 设置消息处理回调
         * @param handler 回调函数
         */
        void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }

        /**
         * @brief 广播消息到所有会话
         * @param message 要广播的消息
         */
        void broadcast(std::shared_ptr<Message> message);

        /**
         * @brief 发送消息到指定会话
         * @param session_id 会话ID
         * @param message 要发送的消息
         */
        void send_to(const std::string &session_id, std::shared_ptr<Message> message);

    private:
        // libuv 回调函数
        static void on_connection(uv_stream_t *server, int status);
        static void on_close(uv_handle_t *handle);

        // 内部处理函数
        void handle_new_session(uv_tcp_t *client);
        void on_session_closed(std::shared_ptr<Session> session);
        void on_read(std::shared_ptr<Session> session, ssize_t nread, const uv_buf_t *buf);
        uv_buf_t on_alloc(uv_handle_t *handle, size_t suggested_size);

        // 成员变量
        uv_loop_t *loop_;                         // libuv 事件循环
        uv_tcp_t server_;                         // TCP 服务器句柄
        std::unique_ptr<ThreadPool> thread_pool_; // 线程池
        std::thread loop_thread_;                 // 事件循环线程
        bool should_stop_{false};                 // 是否应该停止事件循环

        std::vector<std::shared_ptr<Session>> sessions_; // 会话列表
        std::mutex sessions_mutex_;                      // 会话映射表互斥锁

        // 回调函数
        SessionHandler connect_handler_; // 连接处理回调
        SessionHandler close_handler_;   // 关闭处理回调
        MessageHandler message_handler_; // 消息处理回调

        bool is_listening_{false}; // 服务器是否正在监听
    };

} // namespace libuv_net