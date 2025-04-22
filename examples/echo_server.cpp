#include "libuv_net/server.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>

using namespace libuv_net;

std::unique_ptr<Server> server;

void signal_handler(int signum) {
    if (server) {
        server->stop();
    }
}

int main() {
    // Setup logging
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create server
    server = std::make_unique<Server>();

    // Setup handlers
    server->set_session_handler([](std::shared_ptr<Session> session) {
        spdlog::info("New connection from {}:{}", 
                    session->get_remote_address(), 
                    session->get_remote_port());
    });

    server->set_message_handler([](std::shared_ptr<Session> session, 
                                 std::shared_ptr<Message> message) {
        // Echo back the message
        session->send(message);
    });

    // Start server
    if (!server->start("0.0.0.0", 8080)) {
        spdlog::error("Failed to start server");
        return 1;
    }

    // Run event loop
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    return 0;
} 