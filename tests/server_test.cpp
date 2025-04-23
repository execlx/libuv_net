#include <iostream>
#include "libuv_net/server.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <string>
#include <atomic>

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
    std::atomic<bool> should_exit{false};

    // 设置连接回调
    server->set_connect_handler([](std::shared_ptr<Session> session)
                                { spdlog::info("新客户端连接: {} ({}:{})",
                                               session->id(),
                                               session->get_remote_address(),
                                               session->get_remote_port()); });

    // 设置断开连接回调
    server->set_close_handler([](std::shared_ptr<Session> session)
                              { spdlog::info("客户端断开连接: {}", session->id()); });

    // 设置文本消息处理器
    server->set_packet_handler(PacketType::TEXT, [](std::shared_ptr<Session> session, std::shared_ptr<Packet> packet)
                               {
        std::string text(packet->data().begin(), packet->data().end());
        spdlog::info("收到来自 {} 的文本消息: {}", session->id(), text);

        // 回复消息
        std::string response = "服务器已收到消息: " + text;
        std::vector<uint8_t> data(response.begin(), response.end());
        data.push_back('\0');
        auto response_packet = std::make_shared<Packet>(PacketType::TEXT, data);
        session->send(response_packet); });

    // 设置二进制消息处理器
    server->set_packet_handler(PacketType::BINARY, [](std::shared_ptr<Session> session, std::shared_ptr<Packet> packet)
                               { spdlog::info("收到来自 {} 的二进制消息，长度: {}",
                                              session->id(),
                                              packet->data().size()); });

    // 设置心跳消息处理器
    server->set_packet_handler(PacketType::HEARTBEAT, [](std::shared_ptr<Session> session, std::shared_ptr<Packet> packet)
                               { spdlog::debug("收到来自 {} 的心跳包", session->id()); });

    // 设置默认消息处理器
    server->set_default_packet_handler([](std::shared_ptr<Session> session, std::shared_ptr<Packet> packet)
                                       { spdlog::warn("收到来自 {} 的未知类型消息: {}",
                                                      session->id(),
                                                      static_cast<int>(packet->type())); });

    // 启动事件循环
    if (!server->start())
    {
        spdlog::error("启动事件循环失败");
        return 1;
    }

    // 启动服务器
    server->listen("0.0.0.0", 8080);

    // 等待用户输入退出
    std::string input;
    while (!should_exit)
    {
        spdlog::info("输入 'quit' 退出服务器:");
        std::getline(std::cin, input);

        if (input == "quit")
        {
            should_exit = true;
        }
    }

    // 停止服务器
    server->stop_listening();
    server->stop();
    spdlog::info("服务器已退出");

    return 0;
}