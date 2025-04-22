#include "libuv_net/session.hpp"
#include <spdlog/spdlog.h>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace libuv_net {

Session::Session(uv_loop_t* loop, uv_tcp_t* client)
    : loop_(loop), client_(client) {
    
    // Generate unique session ID
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) 
       << reinterpret_cast<uintptr_t>(this);
    id_ = ss.str();

    // Get remote address and port
    struct sockaddr_storage addr;
    int addr_len = sizeof(addr);
    uv_tcp_getpeername(client_, (struct sockaddr*)&addr, &addr_len);

    char ip[INET6_ADDRSTRLEN];
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in* s = (struct sockaddr_in*)&addr;
        uv_inet_ntop(AF_INET, &s->sin_addr, ip, sizeof(ip));
        remote_port_ = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
        uv_inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof(ip));
        remote_port_ = ntohs(s->sin6_port);
    }
    remote_address_ = ip;

    spdlog::info("New session created: {} ({}:{})", id_, remote_address_, remote_port_);
}

Session::~Session() {
    stop();
    spdlog::info("Session destroyed: {}", id_);
}

void Session::start() {
    uv_read_start((uv_stream_t*)client_, alloc_buffer, on_read);
}

void Session::stop() {
    if (!is_closing_) {
        is_closing_ = true;
        uv_close((uv_handle_t*)client_, on_close);
    }
}

void Session::send(std::shared_ptr<Message> message) {
    if (is_closing_) {
        spdlog::warn("Attempt to send message on closing session: {}", id_);
        return;
    }

    auto buffer = message->serialize();
    auto write_req = new uv_write_t;
    auto buf = new uv_buf_t;
    buf->base = new char[buffer.size()];
    buf->len = buffer.size();
    std::copy(buffer.begin(), buffer.end(), buf->base);

    write_req->data = this;
    uv_write(write_req, (uv_stream_t*)client_, buf, 1, on_write);
}

void Session::alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    auto session = static_cast<Session*>(handle->data);
    session->read_buffer_.resize(suggested_size);
    buf->base = reinterpret_cast<char*>(session->read_buffer_.data());
    buf->len = session->read_buffer_.size();
}

void Session::on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    auto session = static_cast<Session*>(stream->data);
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            spdlog::error("Read error on session {}: {}", session->id_, uv_strerror(nread));
        }
        session->stop();
        return;
    }

    if (nread > 0) {
        session->message_buffer_.insert(session->message_buffer_.end(),
                                      buf->base,
                                      buf->base + nread);
        session->process_buffer();
    }
}

void Session::on_write(uv_write_t* req, int status) {
    auto session = static_cast<Session*>(req->data);
    
    if (status < 0) {
        spdlog::error("Write error on session {}: {}", session->id_, uv_strerror(status));
        session->stop();
    }

    delete[] req->bufs[0].base;
    delete req->bufs;
    delete req;
}

void Session::on_close(uv_handle_t* handle) {
    auto session = static_cast<Session*>(handle->data);
    if (session->close_handler_) {
        session->close_handler_();
    }
    delete session;
}

void Session::process_buffer() {
    while (message_buffer_.size() >= Message::HEADER_SIZE) {
        auto msg = Message::deserialize(message_buffer_);
        if (!msg) {
            // Not enough data for a complete message
            break;
        }

        // Remove processed data from buffer
        message_buffer_.erase(message_buffer_.begin(),
                            message_buffer_.begin() + Message::HEADER_SIZE + msg->length);

        handle_message(msg);
    }
}

void Session::handle_message(std::shared_ptr<Message> message) {
    if (message_handler_) {
        message_handler_(message);
    }
}

} // namespace libuv_net 