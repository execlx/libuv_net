#include <iostream>
#include "libuv_net/client.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace libuv_net;

bool try_connect(Client &client, const std::string &host, int port, int max_retries = 3)
{
    int retry_count = 0;
    while (retry_count < max_retries)
    {
        spdlog::info("尝试连接服务器 (尝试 {}/{})", retry_count + 1, max_retries);
        if (client.connect(host, port))
        {
            return true;
        }
        retry_count++;
        if (retry_count < max_retries)
        {
            spdlog::info("连接失败，5秒后重试...");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    return false;
}

int main()
{
    // 设置控制台编码为 UTF-8（仅 Windows）
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 设置日志级别
    spdlog::set_level(spdlog::level::debug);

    // 创建客户端
    auto client = std::make_unique<Client>();
    bool should_exit = false;
    bool is_connected = false;

    // 设置连接回调
    client->set_connect_handler([&]()
                                {
        spdlog::info("已连接到服务器");
        is_connected = true; });

    // 设置断开连接回调
    client->set_disconnect_handler([&]()
                                   {
        spdlog::info("已断开与服务器的连接");
        is_connected = false;
        should_exit = true; });

    // 设置消息回调
    client->set_message_handler([&](std::shared_ptr<Message> message)
                                {
        if (message->type() == MessageType::TEXT) {
            std::string text(message->data().begin(), message->data().end());
            spdlog::info("收到消息: {}", text);
        } });

    // 启动事件循环
    if (!client->start())
    {
        spdlog::error("启动事件循环失败");
        return 1;
    }

    // 尝试连接服务器
    if (!try_connect(*client, "127.0.0.1", 8080))
    {
        spdlog::error("无法连接到服务器，已达到最大重试次数");
        client->stop();
        return 1;
    }

    // 持续接收用户输入并发送消息
    std::string input;
    while (!should_exit)
    {
        spdlog::info("请输入消息 (输入 'quit' 退出):");
        std::getline(std::cin, input);

        if (input == "quit")
        {
            break;
        }

        if (!input.empty())
        {
            // 检查连接状态，如果断开则尝试重连
            if (!client->is_connected())
            {
                spdlog::info("连接已断开，尝试重新连接...");
                if (!try_connect(*client, "127.0.0.1", 8080))
                {
                    spdlog::error("重连失败，程序退出");
                    break;
                }
            }

            // 将字符串转换为字节向量
            std::vector<uint8_t> data(input.begin(), input.end());
            data.push_back('\0'); // 添加字符串结束符

            auto message = std::make_shared<Message>(MessageType::TEXT, data);
            client->send(message);
            spdlog::info("已发送消息: {}", input);
        }
    }

    // 断开连接
    client->disconnect();
    spdlog::info("客户端已退出");

    return 0;
}