#include "libuv_net/server.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <iostream>

using namespace libuv_net;

int main() {
    // 设置日志级别
    spdlog::set_level(spdlog::level::debug);

    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        spdlog::error("创建事件循环失败");
        return 1;
    }

    // 创建服务器
    auto server = std::make_unique<Server>(loop);

    // 设置连接处理回调
    server->set_connect_handler([](std::shared_ptr<Session> session) {
        spdlog::info("新客户端连接: {}", session->id());
    });

    // 设置关闭处理回调
    server->set_close_handler([](std::shared_ptr<Session> session) {
        spdlog::info("客户端断开连接: {}", session->id());
    });

    // 设置消息处理回调
    server->set_message_handler([&server](std::shared_ptr<Message> message) {
        if (message->type() == MessageType::TEXT) {
            std::string text(message->data().begin(), message->data().end());
            spdlog::info("收到消息: {}", text);

            // 广播消息
            auto reply = std::make_shared<Message>(
                MessageType::TEXT,
                std::vector<uint8_t>{'R', 'e', 'c', 'e', 'i', 'v', 'e', 'd', '!', '\0'}
            );
            server->broadcast(reply);
            spdlog::info("已广播回复消息");
        }
    });

    // 启动服务器
    server->start("0.0.0.0", 8080);

    spdlog::info("服务器已启动，监听端口 8080");

    // 启动心跳线程
    std::thread heartbeat_thread([&server]() {
        while (true) {
            // 广播心跳消息
            auto ping_msg = std::make_shared<Message>(
                MessageType::PING,
                std::vector<uint8_t>()
            );
            server->broadcast(ping_msg);
            spdlog::debug("已发送心跳消息");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
    heartbeat_thread.detach();

    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);

    // 停止服务器
    server->stop();
    spdlog::info("服务器已停止");

    return 0;
} 