#include "libuv_net/server.hpp"
#include "libuv_net/client.hpp"
#include "libuv_net/json_interceptor.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace libuv_net;

// 服务器端处理函数
void handle_server_packet(std::shared_ptr<Packet> packet)
{
    switch (packet->type())
    {
    case PacketType::JSON:
    {
        auto interceptor = std::make_shared<JsonInterceptor>();
        auto json_data = interceptor->deserialize(packet->data());
        auto json = std::any_cast<nlohmann::json>(json_data);
        std::cout << "服务器收到 JSON 数据: " << json.dump(4) << std::endl;
        break;
    }
    case PacketType::BINARY:
    {
        const auto &binary_data = packet->data();
        std::cout << "服务器收到二进制数据: ";
        for (auto byte : binary_data)
        {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;
        break;
    }
    }
}

// 客户端处理函数
void handle_client_packet(std::shared_ptr<Packet> packet)
{
    switch (packet->type())
    {
    case PacketType::JSON:
    {
        auto interceptor = std::make_shared<JsonInterceptor>();
        auto json_data = interceptor->deserialize(packet->data());
        auto json = std::any_cast<nlohmann::json>(json_data);
        std::cout << "客户端收到 JSON 数据: " << json.dump(4) << std::endl;
        break;
    }
    case PacketType::BINARY:
    {
        const auto &binary_data = packet->data();
        std::cout << "客户端收到二进制数据: ";
        for (auto byte : binary_data)
        {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl;
        break;
    }
    }
}

int main()
{
    // 创建服务器
    auto server = std::make_shared<Server>("127.0.0.1", 8080);
    server->set_packet_handler(handle_server_packet);
    server->start();

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 创建客户端
    auto client = std::make_shared<Client>();
    client->set_packet_handler(handle_client_packet);
    client->connect("127.0.0.1", 8080);

    // 等待连接建立
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 发送 JSON 数据
    nlohmann::json json_data = {
        {"name", "测试用户"},
        {"age", 25},
        {"scores", {90, 85, 95}}};
    auto json_interceptor = std::make_shared<JsonInterceptor>();
    auto serialized_json = json_interceptor->serialize(json_data);
    Packet json_packet(PacketType::JSON, serialized_json, 1);
    client->send(json_packet);

    // 发送二进制数据
    std::vector<uint8_t> binary_data = {0x01, 0x02, 0x03, 0x04, 0x05};
    Packet binary_packet(PacketType::BINARY, binary_data, 2);
    client->send(binary_packet);

    // 等待数据发送和接收
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 清理
    client->disconnect();
    server->stop();

    return 0;
}