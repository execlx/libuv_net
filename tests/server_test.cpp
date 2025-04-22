#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "libuv_net/server.hpp"

int main() {
    try {
        std::cout << "Creating server instance..." << std::endl;
        libuv_net::Server server;
        
        // 设置回调
        server.set_session_handler([](std::shared_ptr<libuv_net::Session> session) {
            std::cout << "New session connected: " << session->get_id() << std::endl;
        });

        server.set_message_handler([](std::shared_ptr<libuv_net::Session> session, std::shared_ptr<libuv_net::Message> message) {
            std::string data(reinterpret_cast<const char*>(message->payload.data()), message->payload.size());
            std::cout << "Received from session " << session->get_id() << ": " << data << std::endl;
            
            // 回显消息
            auto echo_msg = std::make_shared<libuv_net::Message>(
                libuv_net::MessageType::DATA,
                std::vector<uint8_t>(data.begin(), data.end())
            );
            session->send(echo_msg);
        });

        // 启动服务器
        std::cout << "Starting server on 127.0.0.1:8080..." << std::endl;
        if (!server.start("127.0.0.1", 8080)) {
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }
        std::cout << "Server started successfully" << std::endl;

        // 保持服务器运行
        std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 