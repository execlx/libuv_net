#include "libuv_net/server.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace libuv_net {

Server::Server() {
    loop_ = uv_default_loop();
    thread_pool_ = std::make_unique<ThreadPool>();
}

Server::~Server() {
    stop();
}

bool Server::start(const std::string& host, uint16_t port) {
    if (is_running_) {
        spdlog::warn("Server is already running");
        return false;
    }

    uv_tcp_init(loop_, &server_);
    server_.data = this;

    struct sockaddr_in addr;
    uv_ip4_addr(host.c_str(), port, &addr);

    int result = uv_tcp_bind(&server_, (const struct sockaddr*)&addr, 0);
    if (result) {
        spdlog::error("Bind error: {}", uv_strerror(result));
        return false;
    }

    result = uv_listen((uv_stream_t*)&server_, 128, on_new_connection);
    if (result) {
        spdlog::error("Listen error: {}", uv_strerror(result));
        return false;
    }

    is_running_ = true;
    spdlog::info("Server started on {}:{}", host, port);
    return true;
}

void Server::stop() {
    if (!is_running_) {
        return;
    }

    is_running_ = false;

    // Close all sessions
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& pair : sessions_) {
            pair.second->stop();
        }
        sessions_.clear();
    }

    // Close server
    uv_close((uv_handle_t*)&server_, on_close);
}

void Server::broadcast(std::shared_ptr<Message> message) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    for (auto& pair : sessions_) {
        pair.second->send(message);
    }
}

void Server::send_to(const std::string& session_id, std::shared_ptr<Message> message) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second->send(message);
    }
}

void Server::on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        spdlog::error("New connection error: {}", uv_strerror(status));
        return;
    }

    auto srv = static_cast<Server*>(server->data);
    auto client = new uv_tcp_t;
    uv_tcp_init(srv->loop_, client);
    
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        srv->handle_new_session(client);
    } else {
        uv_close((uv_handle_t*)client, nullptr);
    }
}

void Server::on_close(uv_handle_t* handle) {
    delete handle;
}

void Server::handle_new_session(uv_tcp_t* client) {
    auto session = std::make_shared<Session>(loop_, client);
    
    session->set_message_handler([this, session](std::shared_ptr<Message> message) {
        if (message_handler_) {
            message_handler_(session, message);
        }
    });

    session->set_close_handler([this, session]() {
        handle_session_close(session->get_id());
    });

    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[session->get_id()] = session;
    }

    if (session_handler_) {
        session_handler_(session);
    }

    session->start();
}

void Server::handle_session_close(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    sessions_.erase(session_id);
}

} // namespace libuv_net 