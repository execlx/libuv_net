#include "libuv_net/session.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

namespace libuv_net
{

    Session::Session(uv_loop_t *loop) : loop_(loop)
    {
        // 初始化套接字
        uv_tcp_init(loop_, &socket_);
        socket_.data = this;

        // 生成会话ID
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0')
           << reinterpret_cast<uintptr_t>(this);
        id_ = ss.str();
    }

    Session::Session(uv_loop_t *loop, uv_tcp_t *client)
        : loop_(loop)
    {
        // 初始化套接字
        uv_tcp_init(loop_, &socket_);
        socket_.data = this;

        // 生成会话ID
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0')
           << reinterpret_cast<uintptr_t>(this);
        id_ = ss.str();

        // 接受客户端连接
        uv_accept(reinterpret_cast<uv_stream_t *>(client),
                  reinterpret_cast<uv_stream_t *>(&socket_));

        // 获取远程地址信息
        struct sockaddr_storage addr;
        int addr_len = sizeof(addr);
        uv_tcp_getpeername(&socket_, reinterpret_cast<struct sockaddr *>(&addr), &addr_len);

        char ip[INET6_ADDRSTRLEN];
        if (addr.ss_family == AF_INET)
        {
            struct sockaddr_in *s = reinterpret_cast<struct sockaddr_in *>(&addr);
            uv_inet_ntop(AF_INET, &s->sin_addr, ip, sizeof(ip));
            remote_port_ = ntohs(s->sin_port);
        }
        else
        {
            struct sockaddr_in6 *s = reinterpret_cast<struct sockaddr_in6 *>(&addr);
            uv_inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof(ip));
            remote_port_ = ntohs(s->sin6_port);
        }
        remote_address_ = ip;

        spdlog::info("新会话已创建: {} ({}:{})", id_, remote_address_, remote_port_);
    }

    Session::~Session()
    {
        if (!is_closing_)
        {
            close();
        }
    }

    int Session::start_read()
    {
        if (is_closing_)
        {
            return UV_EBUSY;
        }

        int result = uv_read_start(reinterpret_cast<uv_stream_t *>(&socket_),
                                   on_alloc,
                                   on_read);
        if (result)
        {
            spdlog::error("开始读取失败: {}", uv_strerror(result));
        }
        return result;
    }

    void Session::stop()
    {
        uv_read_stop(reinterpret_cast<uv_stream_t *>(&socket_));
    }

    void Session::close()
    {
        if (!is_closing_ && !uv_is_closing(reinterpret_cast<uv_handle_t *>(&socket_)))
        {
            is_closing_ = true;
            uv_close(reinterpret_cast<uv_handle_t *>(&socket_), on_close);
        }
    }

    void Session::send(std::shared_ptr<Message> message)
    {
        if (is_closing_)
        {
            return;
        }

        // 序列化消息
        auto data = message->serialize();
        auto write_req = new uv_write_t;
        write_req->data = this;

        // 创建缓冲区
        uv_buf_t buf = uv_buf_init(reinterpret_cast<char *>(data.data()), data.size());

        // 发送数据
        int result = uv_write(write_req,
                              reinterpret_cast<uv_stream_t *>(&socket_),
                              &buf, 1, on_write);
        if (result)
        {
            spdlog::error("发送失败: {}", uv_strerror(result));
            delete write_req;
        }
    }

    void Session::on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
    {
        auto session = static_cast<Session *>(handle->data);
        if (session->alloc_callback_)
        {
            *buf = session->alloc_callback_(suggested_size);
        }
        else
        {
            *buf = uv_buf_init(new char[suggested_size], suggested_size);
        }
    }

    void Session::on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
    {
        auto session = static_cast<Session *>(stream->data);

        if (nread < 0)
        {
            if (nread != UV_EOF)
            {
                spdlog::error("读取错误: {}", uv_strerror(nread));
            }
            session->close();
            return;
        }

        if (nread == 0)
        {
            return;
        }

        // 处理接收到的数据
        if (session->read_handler_)
        {
            session->read_handler_(nread, buf);
        }
        else
        {
            session->append_to_buffer(buf->base, nread);
        }

        delete[] buf->base;
    }

    void Session::on_write(uv_write_t *req, int status)
    {
        auto session = static_cast<Session *>(req->data);
        delete req;

        if (status < 0)
        {
            spdlog::error("写入错误: {}", uv_strerror(status));
            session->close();
        }
    }

    void Session::on_close(uv_handle_t *handle)
    {
        auto session = static_cast<Session *>(handle->data);
        if (session->close_handler_)
        {
            session->close_handler_();
        }
        // 不要直接删除 session，因为它可能被 shared_ptr 管理
    }

    void Session::append_to_buffer(const char *data, size_t len)
    {
        read_buffer_.insert(read_buffer_.end(), data, data + len);
        process_buffer();
    }

    void Session::process_buffer()
    {
        while (!read_buffer_.empty())
        {
            // 检查是否有完整的消息头
            if (read_buffer_.size() < sizeof(MessageHeader))
            {
                return;
            }

            // 解析消息头
            MessageHeader header;
            std::memcpy(&header, read_buffer_.data(), sizeof(MessageHeader));

            // 检查消息体是否完整
            if (read_buffer_.size() < sizeof(MessageHeader) + header.length)
            {
                return;
            }

            // 创建消息对象
            auto message = std::make_shared<Message>();
            message->set_type(static_cast<MessageType>(header.type));
            message->set_data(std::vector<uint8_t>(
                read_buffer_.begin() + sizeof(MessageHeader),
                read_buffer_.begin() + sizeof(MessageHeader) + header.length));

            // 处理消息
            handle_message(message);

            // 移除已处理的数据
            read_buffer_.erase(
                read_buffer_.begin(),
                read_buffer_.begin() + sizeof(MessageHeader) + header.length);
        }
    }

    void Session::handle_message(std::shared_ptr<Message> message)
    {
        if (message_handler_)
        {
            message_handler_(message);
        }
    }

} // namespace libuv_net