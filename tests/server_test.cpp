#include "libuv_net/server.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace libuv_net;

int main()
{
    // 设置控制台编码为 UTF-8（仅 Windows）
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 设置日志级别
    spdlog::set_level(spdlog::level::debug);

    // 创建服务器
    auto server = std::make_unique<Server>();

    // 设置连接处理回调
    server->set_connect_handler([](std::shared_ptr<Session> session)
                                { spdlog::info("新客户端连接: {}", session->id()); });

    // 设置关闭处理回调
    server->set_close_handler([](std::shared_ptr<Session> session)
                              { spdlog::info("客户端断开连接: {}", session->id()); });

    // 设置消息处理回调
    server->set_message_handler([&server](std::shared_ptr<Message> message)
                                {
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
        } });

    // 启动事件循环
    if (!server->start())
    {
        spdlog::error("启动事件循环失败");
        return 1;
    }

    // 启动服务器
    server->listen("0.0.0.0", 8080);

    // 启动心跳线程
    std::thread heartbeat_thread([&server]()
                                 {
        while (true) {
            // 广播心跳消息
            auto ping_msg = std::make_shared<Message>(
                MessageType::PING,
                std::vector<uint8_t>()
            );
            server->broadcast(ping_msg);
            spdlog::debug("已发送心跳消息");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        } });
    heartbeat_thread.detach();

    // 等待用户输入退出
    spdlog::info("按回车键退出...");
    std::cin.get();

    // 停止服务器
    server->stop_listening();
    server->stop();
    spdlog::info("服务器已停止");

    return 0;
}