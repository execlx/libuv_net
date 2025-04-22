#include "libuv_net/client.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace libuv_net;

int main() {
    // Setup logging
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    // Create client
    auto client = std::make_unique<Client>();

    // Setup handlers
    client->set_connect_handler([](bool success) {
        if (success) {
            spdlog::info("Connected to server");
        } else {
            spdlog::error("Failed to connect to server");
        }
    });

    client->set_message_handler([](std::shared_ptr<Message> message) {
        std::string payload(message->payload.begin(), message->payload.end());
        spdlog::info("Received message: {}", payload);
    });

    client->set_close_handler([]() {
        spdlog::info("Connection closed");
    });

    // Connect to server
    if (!client->connect("127.0.0.1", 8080)) {
        spdlog::error("Failed to start client");
        return 1;
    }

    // Wait for connection
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!client->is_connected()) {
        spdlog::error("Client not connected");
        return 1;
    }

    // Send messages
    std::string input;
    while (std::getline(std::cin, input)) {
        if (input == "quit") {
            break;
        }

        auto message = std::make_shared<Message>(
            MessageType::DATA,
            std::vector<uint8_t>(input.begin(), input.end())
        );

        client->send(message);
    }

    // Disconnect
    client->disconnect();

    return 0;
} 