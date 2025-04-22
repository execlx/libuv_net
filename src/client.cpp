#include "libuv_net/client.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace libuv_net {

Client::Client() {
    loop_ = uv_default_loop();
    thread_pool_ = std::make_unique<ThreadPool>();
    uv_tcp_init(loop_, &socket_);
    socket_.data = this;
}

Client::~Client() {
    disconnect();
}

bool Client::connect(const std::string& host, uint16_t port) {
    if (is_connecting_ || is_connected()) {
        spdlog::warn("Client is already connecting or connected");
        return false;
    }

    is_connecting_ = true;

    struct sockaddr_in addr;
    uv_ip4_addr(host.c_str(), port, &addr);

    auto connect_req = new uv_connect_t;
    connect_req->data = this;

    int result = uv_tcp_connect(connect_req, &socket_, (const struct sockaddr*)&addr, on_connect);
    if (result) {
        spdlog::error("Connect error: {}", uv_strerror(result));
        is_connecting_ = false;
        delete connect_req;
        return false;
    }

    return true;
}

void Client::disconnect() {
    if (session_) {
        session_->stop();
        session_.reset();
    }
    is_connecting_ = false;
}

void Client::send(std::shared_ptr<Message> message) {
    if (!is_connected()) {
        spdlog::warn("Attempt to send message while not connected");
        return;
    }
    session_->send(message);
}

void Client::on_connect(uv_connect_t* req, int status) {
    auto client = static_cast<Client*>(req->data);
    client->is_connecting_ = false;

    if (status < 0) {
        spdlog::error("Connection error: {}", uv_strerror(status));
        if (client->connect_handler_) {
            client->connect_handler_(false);
        }
        delete req;
        return;
    }

    client->session_ = std::make_shared<Session>(client->loop_, &client->socket_);
    
    client->session_->set_message_handler([client](std::shared_ptr<Message> message) {
        client->handle_message(message);
    });

    client->session_->set_close_handler([client]() {
        client->handle_close();
    });

    client->session_->start();

    if (client->connect_handler_) {
        client->connect_handler_(true);
    }

    delete req;
}

void Client::on_close(uv_handle_t* handle) {
    // Nothing to do here as the socket is managed by the Session
}

void Client::handle_message(std::shared_ptr<Message> message) {
    if (message_handler_) {
        message_handler_(message);
    }
}

void Client::handle_close() {
    session_.reset();
    is_connecting_ = false;
    if (close_handler_) {
        close_handler_();
    }
}

} // namespace libuv_net 