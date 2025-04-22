#include "libuv_net/server.hpp"
#include "libuv_net/session.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace libuv_net {

Server::Server(uv_loop_t* loop) : loop_(loop) {
    // 初始化服务器套接字
    uv_tcp_init(loop_, &server_);
    server_.data = this;
}

Server::~Server() {
    stop();
}

void Server::start(const std::string& host, int port) {
    // 解析地址
    struct sockaddr_in addr;
    uv_ip4_addr(host.c_str(), port, &addr);

    // 绑定地址
    int result = uv_tcp_bind(&server_, reinterpret_cast<const struct sockaddr*>(&addr), 0);
    if (result) {
        spdlog::error("绑定地址失败: {}", uv_strerror(result));
        return;
    }

    // 开始监听
    result = uv_listen(reinterpret_cast<uv_stream_t*>(&server_), 128, on_connection);
    if (result) {
        spdlog::error("监听失败: {}", uv_strerror(result));
        return;
    }

    spdlog::info("服务器启动成功: {}:{}", host, port);
}

void Server::stop() {
    if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&server_))) {
        uv_close(reinterpret_cast<uv_handle_t*>(&server_), on_close);
    }
}

void Server::broadcast(std::shared_ptr<Message> message) {
    for (const auto& session : sessions_) {
        session->send(message);
    }
}

void Server::send_to(const std::string& session_id, std::shared_ptr<Message> message) {
    auto it = std::find_if(sessions_.begin(), sessions_.end(),
                          [&session_id](const auto& session) {
                              return session->id() == session_id;
                          });
    if (it != sessions_.end()) {
        (*it)->send(message);
    }
}

void Server::on_connection(uv_stream_t* server, int status) {
    auto self = static_cast<Server*>(server->data);
    if (status < 0) {
        spdlog::error("连接错误: {}", uv_strerror(status));
        return;
    }

    // 创建新的会话
    auto session = std::make_shared<Session>(self->loop_, reinterpret_cast<uv_tcp_t*>(server));
    self->sessions_.push_back(session);

    // 设置消息处理回调
    session->set_message_handler([self](std::shared_ptr<Message> message) {
        if (self->message_handler_) {
            self->message_handler_(message);
        }
    });

    // 设置关闭处理回调
    session->set_close_handler([self, session]() {
        self->sessions_.erase(
            std::remove(self->sessions_.begin(), self->sessions_.end(), session),
            self->sessions_.end()
        );
        if (self->close_handler_) {
            self->close_handler_(session);
        }
    });

    // 启动会话
    session->start();

    // 调用连接处理回调
    if (self->connect_handler_) {
        self->connect_handler_(session);
    }
}

void Server::on_close(uv_handle_t* handle) {
    auto server = static_cast<Server*>(handle->data);
    // 服务器关闭时不需要调用 close_handler_，因为它需要一个 Session 参数
    // 如果需要通知服务器关闭，可以添加一个新的回调类型
}

} // namespace libuv_net 