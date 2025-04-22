#pragma once

#include <memory>
#include <string>
#include <functional>
#include <uv.h>
#include "libuv_net/session.hpp"
#include "libuv_net/thread_pool.hpp"

namespace libuv_net {

class Client {
public:
    using ConnectHandler = std::function<void(bool)>;
    using MessageHandler = std::function<void(std::shared_ptr<Message>)>;
    using CloseHandler = std::function<void()>;

    Client();
    ~Client();

    // Disable copying
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    void send(std::shared_ptr<Message> message);

    void set_connect_handler(ConnectHandler handler) { connect_handler_ = std::move(handler); }
    void set_message_handler(MessageHandler handler) { message_handler_ = std::move(handler); }
    void set_close_handler(CloseHandler handler) { close_handler_ = std::move(handler); }

    bool is_connected() const { return session_ != nullptr; }

private:
    static void on_connect(uv_connect_t* req, int status);
    static void on_close(uv_handle_t* handle);

    void handle_message(std::shared_ptr<Message> message);
    void handle_close();

    uv_loop_t* loop_;
    uv_tcp_t socket_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::shared_ptr<Session> session_;

    ConnectHandler connect_handler_;
    MessageHandler message_handler_;
    CloseHandler close_handler_;

    bool is_connecting_{false};
};

} // namespace libuv_net 