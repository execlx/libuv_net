#include <iostream>
#include "libuv_net/client.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

using namespace libuv_net;

int main() {
    // 设置日志级别
    spdlog::set_level(spdlog::level::debug);

    // 创建客户端
    auto client = std::make_unique<Client>();

    // 设置连接回调
    client->set_connect_handler([&]() {
        spdlog::info("已连接到服务器");

        // 发送测试消息
        auto message = std::make_shared<Message>(
            MessageType::TEXT,
            std::vector<uint8_t>{'H', 'e', 'l', 'l', 'o', '!', '\0'}
        );
        client->send(message);
        spdlog::info("已发送测试消息");
    });

    // 设置断开连接回调
    client->set_disconnect_handler([&]() {
        spdlog::info("已断开与服务器的连接");
    });

    // 设置消息回调
    client->set_message_handler([&](std::shared_ptr<Message> message) {
        if (message->type() == MessageType::TEXT) {
            std::string text(message->data().begin(), message->data().end());
            spdlog::info("收到消息: {}", text);
        }
    });

    // 尝试连接服务器
    int retry_count = 0;
    const int max_retries = 3;
    bool connected = false;

    while (retry_count < max_retries && !connected) {
        spdlog::info("尝试连接服务器 (尝试 {}/{})", retry_count + 1, max_retries);
        connected = client->connect("127.0.0.1", 8080);
        
        if (!connected) {
            retry_count++;
            if (retry_count < max_retries) {
                spdlog::info("连接失败，5秒后重试...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    if (!connected) {
        spdlog::error("无法连接到服务器，已达到最大重试次数");
        return 1;
    }

    // 等待用户输入退出
    spdlog::info("按回车键退出...");
    std::cin.get();

    // 断开连接
    client->disconnect();

    return 0;
} 