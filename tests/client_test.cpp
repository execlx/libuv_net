#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "libuv_net/client.hpp"

int main() {
    try {
        std::cout << "Creating client instance..." << std::endl;
        libuv_net::Client client;
        
        // 设置回调
        client.set_connect_handler([](bool success) {
            if (success) {
                std::cout << "Connected to server successfully" << std::endl;
            } else {
                std::cout << "Failed to connect to server" << std::endl;
            }
        });

        client.set_message_handler([](std::shared_ptr<libuv_net::Message> message) {
            std::string data(reinterpret_cast<const char*>(message->payload.data()), message->payload.size());
            std::cout << "Received from server: " << data << std::endl;
        });

        client.set_close_handler([]() {
            std::cout << "Disconnected from server" << std::endl;
        });

        // 尝试连接服务器，最多重试3次
        const int max_retries = 3;
        int retry_count = 0;
        bool connected = false;

        while (retry_count < max_retries && !connected) {
            // 确保在重试前断开连接
            client.disconnect();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 等待一小段时间确保断开完成

            std::cout << "Connecting to server at 127.0.0.1:8080 (attempt " << (retry_count + 1) << "/" << max_retries << ")..." << std::endl;
            
            if (!client.connect("127.0.0.1", 8080)) {
                std::cerr << "Failed to initiate connection" << std::endl;
                retry_count++;
                if (retry_count < max_retries) {
                    std::cout << "Retrying in 2 seconds..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
                continue;
            }

            // 等待连接建立
            std::cout << "Waiting for connection to be established..." << std::endl;
            for (int i = 0; i < 10; i++) {  // 等待最多10秒
                if (client.is_connected()) {
                    connected = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            if (!connected) {
                retry_count++;
                if (retry_count < max_retries) {
                    std::cout << "Connection timeout. Retrying in 2 seconds..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
            }
        }

        if (!connected) {
            std::cerr << "Failed to connect to server after " << max_retries << " attempts. Please check if the server is running." << std::endl;
            return 1;
        }

        // 发送测试消息
        std::string message;
        while (true) {
            std::cout << "Enter message (or 'quit' to exit): ";
            std::getline(std::cin, message);
            
            if (message == "quit") {
                break;
            }

            if (client.is_connected()) {
                auto msg = std::make_shared<libuv_net::Message>(
                    libuv_net::MessageType::DATA,
                    std::vector<uint8_t>(message.begin(), message.end())
                );
                client.send(msg);
                std::cout << "Message sent: " << message << std::endl;
            } else {
                std::cout << "Not connected to server. Please restart the client." << std::endl;
                break;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 