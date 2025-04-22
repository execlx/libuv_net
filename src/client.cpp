#include "libuv_net/client.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace libuv_net {

Client::Client() {
    loop_ = uv_default_loop();
    thread_pool_ = std::make_unique<ThreadPool>();
}

Client::~Client() {
    disconnect();
}

bool Client::connect(const std::string& host, uint16_t port) {
    // 检查连接状态
    if (is_connected_) {
        spdlog::warn("客户端已经连接");
        return false;
    }

    // 初始化套接字
    if (!init_socket()) {
        return false;
    }

    // 解析地址
    struct sockaddr_in addr;
    int result = uv_ip4_addr(host.c_str(), port, &addr);
    if (result) {
        spdlog::error("无效的地址: {}", uv_strerror(result));
        return false;
    }

    // 创建连接请求
    auto connect_req = new uv_connect_t;
    connect_req->data = this;

    // 开始连接
    result = uv_tcp_connect(connect_req, &socket_, (const struct sockaddr*)&addr, on_connect);
    if (result) {
        spdlog::error("连接失败: {}", uv_strerror(result));
        delete connect_req;
        return false;
    }

    is_connecting_ = true;
    spdlog::info("正在连接到服务器 {}:{}", host, port);
    return true;
}

void Client::disconnect() {
    if (!is_connected_ && !is_connecting_) {
        return;
    }

    // 关闭套接字
    if (!uv_is_closing((uv_handle_t*)&socket_)) {
        uv_close((uv_handle_t*)&socket_, on_close);
    }

    is_connected_ = false;
    is_connecting_ = false;
    spdlog::info("客户端已断开连接");
}

void Client::send(std::shared_ptr<Message> message) {
    if (!is_connected_) {
        spdlog::warn("客户端未连接，无法发送消息");
        return;
    }

    // 序列化消息
    auto data = message->serialize();
    auto write_req = new uv_write_t;
    write_req->data = this;

    // 创建缓冲区
    uv_buf_t buf = uv_buf_init(reinterpret_cast<char*>(data.data()), data.size());

    // 发送数据
    int result = uv_write(write_req, (uv_stream_t*)&socket_, &buf, 1, on_write);
    if (result) {
        spdlog::error("发送失败: {}", uv_strerror(result));
        delete write_req;
    }
}

void Client::on_connect(uv_connect_t* req, int status) {
    auto client = static_cast<Client*>(req->data);
    delete req;

    if (status < 0) {
        spdlog::error("连接错误: {}", uv_strerror(status));
        client->is_connecting_ = false;
        return;
    }

    // 开始读取数据
    int result = uv_read_start((uv_stream_t*)&client->socket_, 
                              [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
                                  *buf = uv_buf_init(new char[suggested_size], suggested_size);
                              },
                              on_read);
    if (result) {
        spdlog::error("开始读取失败: {}", uv_strerror(result));
        client->disconnect();
        return;
    }

    client->is_connected_ = true;
    client->is_connecting_ = false;
    spdlog::info("连接成功");

    // 调用连接回调
    if (client->connect_handler_) {
        client->connect_handler_();
    }
}

void Client::on_close(uv_handle_t* handle) {
    auto client = static_cast<Client*>(handle->data);
    client->is_connected_ = false;
    client->is_connecting_ = false;

    // 调用断开连接回调
    if (client->disconnect_handler_) {
        client->disconnect_handler_();
    }
}

void Client::on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    auto client = static_cast<Client*>(stream->data);

    if (nread < 0) {
        if (nread != UV_EOF) {
            spdlog::error("读取错误: {}", uv_strerror(nread));
        }
        client->disconnect();
        return;
    }

    if (nread == 0) {
        return;
    }

    // 处理接收到的数据
    client->append_to_buffer(buf->base, nread);
}

void Client::on_write(uv_write_t* req, int status) {
    auto client = static_cast<Client*>(req->data);
    delete req;

    if (status < 0) {
        spdlog::error("写入错误: {}", uv_strerror(status));
        client->disconnect();
    }
}

bool Client::init_socket() {
    // 检查套接字状态
    if (uv_is_active((uv_handle_t*)&socket_)) {
        if (!uv_is_closing((uv_handle_t*)&socket_)) {
            uv_close((uv_handle_t*)&socket_, nullptr);
        }
        return false;
    }

    // 初始化套接字
    int result = uv_tcp_init(loop_, &socket_);
    if (result) {
        spdlog::error("初始化套接字失败: {}", uv_strerror(result));
        return false;
    }

    socket_.data = this;
    return true;
}

void Client::append_to_buffer(const char* data, size_t len) {
    buffer_.insert(buffer_.end(), data, data + len);
    
    // 处理完整的消息
    while (buffer_.size() >= sizeof(MessageHeader)) {
        auto header = reinterpret_cast<const MessageHeader*>(buffer_.data());
        size_t message_size = sizeof(MessageHeader) + header->length;
        
        if (buffer_.size() < message_size) {
            break;
        }
        
        // 解析消息
        auto message = std::make_shared<Message>();
        if (!message->deserialize(buffer_.data(), message_size)) {
            spdlog::error("消息解析失败");
            buffer_.clear();
            return;
        }
        
        // 调用消息处理回调
        if (message_handler_) {
            message_handler_(message);
        }
        
        // 移除已处理的数据
        buffer_.erase(buffer_.begin(), buffer_.begin() + message_size);
    }
}

} // namespace libuv_net 