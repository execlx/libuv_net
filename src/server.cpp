#include "libuv_net/server.hpp"
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

    Server::Server()
    {
        loop_ = uv_loop_new();
        if (!loop_)
        {
            throw std::runtime_error("创建事件循环失败");
        }
        thread_pool_ = std::make_unique<ThreadPool>();

        // 初始化服务器套接字
        uv_tcp_init(loop_, &server_);
        server_.data = this;
    }

    Server::~Server()
    {
        stop_listening();
        stop();
        uv_loop_delete(loop_);
    }

    bool Server::start()
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

    void Server::stop()
    {
        if (!loop_thread_.joinable())
        {
            return;
        }

        should_stop_ = true;
        loop_thread_.join();
    }

    void Server::listen(const std::string &host, int port)
    {
        if (is_listening_)
        {
            spdlog::warn("服务器已经在监听");
            return;
        }

        // 解析地址
        struct sockaddr_in addr;
        uv_ip4_addr(host.c_str(), port, &addr);

        // 绑定地址
        int result = uv_tcp_bind(&server_, reinterpret_cast<const struct sockaddr *>(&addr), 0);
        if (result)
        {
            spdlog::error("绑定地址失败: {}", uv_strerror(result));
            return;
        }

        // 开始监听
        result = uv_listen(reinterpret_cast<uv_stream_t *>(&server_), SOMAXCONN, on_connection);
        if (result)
        {
            spdlog::error("监听失败: {}", uv_strerror(result));
            return;
        }

        is_listening_ = true;
        spdlog::info("服务器已启动，监听 {}:{}", host, port);
    }

    void Server::stop_listening()
    {
        if (!is_listening_)
        {
            return;
        }

        if (!uv_is_closing(reinterpret_cast<uv_handle_t *>(&server_)))
        {
            uv_close(reinterpret_cast<uv_handle_t *>(&server_), on_close);
        }

        is_listening_ = false;
        spdlog::info("服务器已停止监听");
    }

    void Server::broadcast(std::shared_ptr<Message> message)
    {
        for (const auto &session : sessions_)
        {
            session->send(message);
        }
    }

    void Server::send_to(const std::string &session_id, std::shared_ptr<Message> message)
    {
        auto it = std::find_if(sessions_.begin(), sessions_.end(),
                               [&session_id](const auto &session)
                               {
                                   return session->id() == session_id;
                               });
        if (it != sessions_.end())
        {
            (*it)->send(message);
        }
    }

    void Server::on_connection(uv_stream_t *server, int status)
    {
        auto self = static_cast<Server *>(server->data);
        if (status < 0)
        {
            spdlog::error("连接错误: {}", uv_strerror(status));
            return;
        }

        // 创建新的会话
        auto session = std::make_shared<Session>(self->loop_, reinterpret_cast<uv_tcp_t *>(server));
        self->sessions_.push_back(session);

        // 设置消息处理回调
        session->set_message_handler([self](std::shared_ptr<Message> message)
                                     {
        if (self->message_handler_) {
            self->message_handler_(message);
        } });

        // 设置关闭处理回调
        session->set_close_handler([self, session]()
                                   {
        self->sessions_.erase(
            std::remove(self->sessions_.begin(), self->sessions_.end(), session),
            self->sessions_.end()
        );
        if (self->close_handler_) {
            self->close_handler_(session);
        } });

        // 启动会话
        session->start();

        // 调用连接处理回调
        if (self->connect_handler_)
        {
            self->connect_handler_(session);
        }
    }

    void Server::on_close(uv_handle_t *handle)
    {
        auto server = static_cast<Server *>(handle->data);
        // 服务器关闭时不需要调用 close_handler_，因为它需要一个 Session 参数
        // 如果需要通知服务器关闭，可以添加一个新的回调类型
    }

    void Server::handle_new_session(uv_tcp_t *client)
    {
        // 创建新的会话
        auto session = std::make_shared<Session>(loop_, client);
        sessions_.push_back(session);

        // 设置消息处理回调
        session->set_message_handler([this](std::shared_ptr<Message> message)
                                     {
            if (message_handler_) {
                message_handler_(message);
            } });

        // 设置关闭处理回调
        session->set_close_handler([this, session]()
                                   { on_session_closed(session); });

        // 启动会话
        session->start();

        // 调用连接处理回调
        if (connect_handler_)
        {
            connect_handler_(session);
        }
    }

    void Server::on_session_closed(std::shared_ptr<Session> session)
    {
        // 从会话列表中移除
        sessions_.erase(
            std::remove(sessions_.begin(), sessions_.end(), session),
            sessions_.end());

        // 调用关闭处理回调
        if (close_handler_)
        {
            close_handler_(session);
        }
    }

    void Server::on_read(std::shared_ptr<Session> session, ssize_t nread, const uv_buf_t *buf)
    {
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
        session->append_to_buffer(buf->base, nread);
    }

    uv_buf_t Server::on_alloc(uv_handle_t *handle, size_t suggested_size)
    {
        auto server = static_cast<Server *>(handle->data);
        return uv_buf_init(new char[suggested_size], suggested_size);
    }

} // namespace libuv_net