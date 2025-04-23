#include "libuv_net/client.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

namespace libuv_net
{

    Client::Client()
    {
        loop_ = uv_loop_new();
        if (!loop_)
        {
            throw std::runtime_error("创建事件循环失败");
        }
        thread_pool_ = std::make_unique<ThreadPool>();

        // 初始化心跳定时器
        uv_timer_init(loop_, &heartbeat_timer_);
        heartbeat_timer_.data = this;
    }

    Client::~Client()
    {
        stop();
        disconnect();
        uv_loop_delete(loop_);
    }

    bool Client::start()
    {
        if (loop_thread_.joinable())
        {
            spdlog::warn("事件循环已经在运行");
            return false;
        }

        should_stop_ = false;
        loop_thread_ = std::thread([this]()
                                   {
            spdlog::info("事件循环线程启动");
            while (!should_stop_)
            {
                uv_run(loop_, UV_RUN_NOWAIT);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            spdlog::info("事件循环线程退出"); });

        return true;
    }

    void Client::stop()
    {
        if (!loop_thread_.joinable())
        {
            return;
        }

        should_stop_ = true;
        loop_thread_.join();
        stop_heartbeat();
    }

    bool Client::connect(const std::string &host, uint16_t port)
    {
        // 检查连接状态
        if (is_connected_ || is_connecting_)
        {
            spdlog::warn("客户端已经连接或正在连接");
            return false;
        }

        // 初始化套接字
        if (!init_socket())
        {
            return false;
        }

        // 解析地址
        struct sockaddr_in addr;
        uv_ip4_addr(host.c_str(), port, &addr);

        // 创建连接请求
        auto connect_req = new uv_connect_t;
        connect_req->data = this;

        // 发起连接
        int result = uv_tcp_connect(connect_req, &socket_,
                                    reinterpret_cast<const struct sockaddr *>(&addr),
                                    on_connect);
        if (result)
        {
            spdlog::error("连接失败: {}", uv_strerror(result));
            delete connect_req;
            is_connecting_ = false;
            return false;
        }

        is_connecting_ = true;
        spdlog::info("正在连接到服务器 {}:{}", host, port);
        return true;
    }

    void Client::disconnect()
    {
        if (!is_connected_ && !is_connecting_)
        {
            return;
        }

        // 关闭套接字
        if (!uv_is_closing((uv_handle_t *)&socket_))
        {
            uv_close((uv_handle_t *)&socket_, on_close);
        }

        is_connected_ = false;
        is_connecting_ = false;
        stop_heartbeat();
        spdlog::info("客户端已断开连接");
    }

    void Client::send(std::shared_ptr<Packet> packet)
    {
        if (!is_connected_)
        {
            spdlog::warn("客户端未连接，无法发送消息");
            return;
        }

        // 序列化消息
        auto data = packet->serialize();
        auto write_req = new uv_write_t;
        write_req->data = this;

        // 创建缓冲区
        uv_buf_t buf = uv_buf_init(reinterpret_cast<char *>(data.data()), data.size());

        // 发送数据
        int result = uv_write(write_req, (uv_stream_t *)&socket_, &buf, 1, on_write);
        if (result)
        {
            spdlog::error("发送失败: {}", uv_strerror(result));
            delete write_req;
        }
    }

    void Client::on_connect(uv_connect_t *req, int status)
    {
        auto client = static_cast<Client *>(req->data);
        delete req;

        if (status < 0)
        {
            spdlog::error("连接错误: {}", uv_strerror(status));
            client->is_connecting_ = false;
            return;
        }

        // 开始读取数据
        int result = uv_read_start((uv_stream_t *)&client->socket_,
                                   [](uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
                                   {
                                       *buf = uv_buf_init(new char[suggested_size], suggested_size);
                                   },
                                   on_read);
        if (result)
        {
            spdlog::error("开始读取失败: {}", uv_strerror(result));
            client->is_connecting_ = false;
            client->disconnect();
            return;
        }

        client->is_connected_ = true;
        client->is_connecting_ = false;
        client->start_heartbeat();
        spdlog::info("连接成功");

        // 调用连接回调
        if (client->connect_handler_)
        {
            client->connect_handler_();
        }
    }

    void Client::on_close(uv_handle_t *handle)
    {
        auto client = static_cast<Client *>(handle->data);
        client->is_connected_ = false;
        client->is_connecting_ = false;
        client->stop_heartbeat();

        // 调用断开连接回调
        if (client->disconnect_handler_)
        {
            client->disconnect_handler_();
        }
    }

    void Client::on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
    {
        auto client = static_cast<Client *>(stream->data);

        if (nread < 0)
        {
            if (nread != UV_EOF)
            {
                spdlog::error("读取错误: {}", uv_strerror(nread));
            }
            client->disconnect();
            return;
        }

        if (nread == 0)
        {
            return;
        }

        // 处理接收到的数据
        client->append_to_buffer(buf->base, nread);
    }

    void Client::on_write(uv_write_t *req, int status)
    {
        auto client = static_cast<Client *>(req->data);
        delete req;

        if (status < 0)
        {
            spdlog::error("写入错误: {}", uv_strerror(status));
            client->disconnect();
        }
    }

    void Client::on_heartbeat_timer(uv_timer_t *handle)
    {
        auto client = static_cast<Client *>(handle->data);

        // 检查心跳超时
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - client->last_heartbeat_time_)
                           .count();

        if (elapsed > HEARTBEAT_TIMEOUT_MS)
        {
            spdlog::warn("心跳超时，断开连接");
            client->disconnect();
            return;
        }

        // 发送心跳包
        client->send_heartbeat();
    }

    bool Client::init_socket()
    {
        // 检查套接字状态
        if (uv_is_active((uv_handle_t *)&socket_))
        {
            if (!uv_is_closing((uv_handle_t *)&socket_))
            {
                uv_close((uv_handle_t *)&socket_, nullptr);
            }
            return false;
        }

        // 初始化套接字
        int result = uv_tcp_init(loop_, &socket_);
        if (result)
        {
            spdlog::error("初始化套接字失败: {}", uv_strerror(result));
            return false;
        }

        socket_.data = this;
        return true;
    }

    void Client::append_to_buffer(const char *data, size_t len)
    {
        buffer_.insert(buffer_.end(), data, data + len);

        // 处理完整的消息
        while (buffer_.size() >= sizeof(PacketHeader))
        {
            const PacketHeader *header = reinterpret_cast<const PacketHeader *>(buffer_.data());
            size_t message_size = sizeof(PacketHeader) + header->length;

            if (buffer_.size() < message_size)
            {
                break;
            }

            // 解析消息
            auto packet = std::make_shared<Packet>();
            if (!packet->deserialize(buffer_.data(), message_size))
            {
                spdlog::error("消息解析失败");
                buffer_.clear();
                return;
            }

            // 处理心跳包
            if (packet->type() == PacketType::HEARTBEAT)
            {
                last_heartbeat_time_ = std::chrono::steady_clock::now();
                buffer_.erase(buffer_.begin(), buffer_.begin() + message_size);
                continue;
            }

            // 查找对应的处理器
            auto it = packet_handlers_.find(packet->type());
            if (it != packet_handlers_.end())
            {
                it->second(packet);
            }
            else if (default_packet_handler_)
            {
                default_packet_handler_(packet);
            }

            // 移除已处理的数据
            buffer_.erase(buffer_.begin(), buffer_.begin() + message_size);
        }
    }

    void Client::start_heartbeat()
    {
        last_heartbeat_time_ = std::chrono::steady_clock::now();
        uv_timer_start(&heartbeat_timer_, on_heartbeat_timer,
                       HEARTBEAT_INTERVAL_MS, HEARTBEAT_INTERVAL_MS);
    }

    void Client::stop_heartbeat()
    {
        uv_timer_stop(&heartbeat_timer_);
    }

    void Client::send_heartbeat()
    {
        auto packet = std::make_shared<Packet>(PacketType::HEARTBEAT, std::vector<uint8_t>());
        send(packet);
    }

} // namespace libuv_net