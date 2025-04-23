#include <iostream>
#include "libuv_net/client.hpp"
#include "libuv_net/json_interceptor.hpp"
#include "libuv_net/protobuf_interceptor.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <string>
#include <nlohmann/json.hpp>

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

    // 添加拦截器
    client->add_interceptor(std::make_shared<JsonInterceptor>());
    client->add_interceptor(std::make_shared<ProtobufInterceptor>());

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

    // 设置文本消息处理器
    client->set_packet_handler(PacketType::TEXT, [&](std::shared_ptr<Packet> packet)
                               {
        std::string text(packet->data().begin(), packet->data().end());
        spdlog::info("收到文本消息: {}", text); });

    // 设置JSON消息处理器
    client->set_packet_handler(PacketType::JSON, [&](std::shared_ptr<Packet> packet)
                               {
        auto interceptor = std::make_shared<JsonInterceptor>();
        auto data = interceptor->deserialize(packet->data());
        if (data.has_value()) {
            auto json = std::any_cast<nlohmann::json>(data);
            spdlog::info("收到JSON消息: {}", json.dump(4));
        } });

    // 设置二进制消息处理器
    client->set_packet_handler(PacketType::BINARY, [&](std::shared_ptr<Packet> packet)
                               { spdlog::info("收到二进制消息，长度: {}", packet->data().size()); });

    // 设置心跳消息处理器
    client->set_packet_handler(PacketType::HEARTBEAT, [&](std::shared_ptr<Packet> /*packet*/)
                               { spdlog::debug("收到心跳包"); });

    // 设置默认消息处理器
    client->set_default_packet_handler([&](std::shared_ptr<Packet> packet)
                                       { spdlog::warn("收到未知类型消息: {}", static_cast<int>(packet->type())); });

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
        spdlog::info("请输入消息类型 (text/json/binary/quit):");
        std::getline(std::cin, input);

        if (input == "quit")
        {
            break;
        }

        if (input == "text")
        {
            spdlog::info("请输入文本消息:");
            std::getline(std::cin, input);
            if (!input.empty())
            {
                std::vector<uint8_t> data(input.begin(), input.end());
                data.push_back('\0');
                auto packet = std::make_shared<Packet>(PacketType::TEXT, data);
                client->send(packet);
                spdlog::info("已发送文本消息: {}", input);
            }
        }
        else if (input == "json")
        {
            spdlog::info("请输入JSON消息:");
            std::getline(std::cin, input);
            if (!input.empty())
            {
                try
                {
                    auto json = nlohmann::json::parse(input);
                    client->send_data(PacketType::JSON, json);
                    spdlog::info("已发送JSON消息: {}", json.dump(4));
                }
                catch (const nlohmann::json::parse_error &e)
                {
                    spdlog::error("JSON解析失败: {}", e.what());
                }
            }
        }
        else if (input == "binary")
        {
            spdlog::info("请输入二进制数据 (十六进制):");
            std::getline(std::cin, input);
            if (!input.empty())
            {
                std::vector<uint8_t> data;
                for (size_t i = 0; i < input.length(); i += 2)
                {
                    std::string byte = input.substr(i, 2);
                    data.push_back(static_cast<uint8_t>(std::stoi(byte, nullptr, 16)));
                }
                auto packet = std::make_shared<Packet>(PacketType::BINARY, data);
                client->send(packet);
                spdlog::info("已发送二进制消息，长度: {}", data.size());
            }
        }
    }

    // 断开连接
    client->disconnect();
    spdlog::info("客户端已退出");

    return 0;
}