#pragma once

#include <memory>
#include <string>
#include <functional>
#include <uv.h>
#include "libuv_net/message.hpp"
#include "libuv_net/thread_pool.hpp"
#include <spdlog/spdlog.h>
#include <map>

namespace libuv_net
{

    /**
     * @brief TCP 客户端类
     *
     * 提供 TCP 客户端功能，包括：
     * - 连接到服务器
     * - 发送和接收消息
     * - 自动重连
     * - 心跳检测
     */
    class Client
    {
    public:
        // 回调函数类型定义
        using ConnectHandler = std::function<void()>;                       // 连接回调
        using DisconnectHandler = std::function<void()>;                    // 断开连接回调
        using PacketHandler = std::function<void(std::shared_ptr<Packet>)>; // 消息处理回调

        Client();
        ~Client();

        // 禁用拷贝构造和赋值
        Client(const Client &) = delete;
        Client &operator=(const Client &) = delete;

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
         * @brief 连接到服务器
         * @param host 服务器主机名或 IP 地址
         * @param port 服务器端口
         * @return 是否成功发起连接
         */
        bool connect(const std::string &host, uint16_t port);

        /**
         * @brief 断开与服务器的连接
         */
        void disconnect();

        /**
         * @brief 发送消息到服务器
         * @param packet 要发送的消息
         */
        void send(std::shared_ptr<Packet> packet);

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
         * @param type 消息类型
         * @param handler 回调函数
         */
        void set_packet_handler(PacketType type, PacketHandler handler)
        {
            packet_handlers_[type] = std::move(handler);
        }

        /**
         * @brief 设置默认消息处理回调
         * @param handler 回调函数
         */
        void set_default_packet_handler(PacketHandler handler)
        {
            default_packet_handler_ = std::move(handler);
        }

        /**
         * @brief 检查是否已连接
         * @return 是否已连接
         */
        bool is_connected() const { return is_connected_; }

        /**
         * @brief 检查是否正在连接
         * @return 是否正在连接
         */
        bool is_connecting() const { return is_connecting_; }

    private:
        // libuv 回调函数
        static void on_connect(uv_connect_t *req, int status);
        static void on_close(uv_handle_t *handle);
        static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
        static void on_write(uv_write_t *req, int status);
        static void on_heartbeat_timer(uv_timer_t *handle);

        // 内部处理函数
        bool init_socket();
        void append_to_buffer(const char *data, size_t len);
        void start_heartbeat();
        void stop_heartbeat();
        void send_heartbeat();

        // 成员变量
        uv_loop_t *loop_;                         // libuv 事件循环
        uv_tcp_t socket_;                         // TCP 套接字
        std::unique_ptr<ThreadPool> thread_pool_; // 线程池
        std::thread loop_thread_;                 // 事件循环线程
        bool should_stop_{false};                 // 是否应该停止事件循环

        // 状态标志
        bool is_connected_{false};  // 是否已连接
        bool is_connecting_{false}; // 是否正在连接

        // 回调函数
        ConnectHandler connect_handler_;                      // 连接回调
        DisconnectHandler disconnect_handler_;                // 断开连接回调
        std::map<PacketType, PacketHandler> packet_handlers_; // 消息处理回调
        PacketHandler default_packet_handler_;                // 默认消息处理回调

        // 缓冲区
        std::vector<uint8_t> buffer_; // 接收缓冲区

        // 心跳相关
        uv_timer_t heartbeat_timer_;
        std::chrono::steady_clock::time_point last_heartbeat_time_;
        static constexpr int HEARTBEAT_INTERVAL_MS = 30000; // 30秒
        static constexpr int HEARTBEAT_TIMEOUT_MS = 90000;  // 90秒
    };

} // namespace libuv_net