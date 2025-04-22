# libuv_net

基于 libuv 的高性能 TCP 通信库，支持高并发、线程池、会话管理、任务调度等功能。

## 特性

- 基于 libuv 的高性能 TCP 通信
- 线程池支持并发任务处理
- 会话管理，支持唯一会话 ID
- 任务调度和管理
- 协议支持，包含消息帧
- 使用 spdlog 的日志系统
- 客户端和服务器实现
- 支持 C++11 标准
- 支持 Qt 框架

## 构建

### 依赖项

- CMake 3.15 或更高版本
- 支持 C++11 的编译器
- libuv
- spdlog
- Qt5 (Core, Network)

### 构建步骤

```bash
mkdir build
cd build
cmake ..
make
```

## 示例

### 回显服务器

一个简单的回显服务器，将接收到的消息原样返回：

```cpp
#include "libuv_net/server.hpp"
#include <spdlog/spdlog.h>

int main() {
    auto server = std::make_unique<libuv_net::Server>();
    
    server->set_message_handler([](std::shared_ptr<Session> session, 
                                 std::shared_ptr<Message> message) {
        // 将消息原样返回
        session->send(message);
    });

    server->start("0.0.0.0", 8080);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
```

### 回显客户端

一个简单的客户端，连接到回显服务器并发送消息：

```cpp
#include "libuv_net/client.hpp"
#include <spdlog/spdlog.h>

int main() {
    auto client = std::make_unique<libuv_net::Client>();
    
    client->set_message_handler([](std::shared_ptr<Message> message) {
        std::string payload(message->payload.begin(), message->payload.end());
        spdlog::info("收到消息: {}", payload);
    });

    client->connect("127.0.0.1", 8080);
    
    auto message = std::make_shared<Message>(
        MessageType::DATA,
        std::vector<uint8_t>("你好，世界！", 13)
    );
    
    client->send(message);
}
```

## 协议

库使用简单的协议进行消息帧：

```
| 长度(4字节) | 类型(1字节) | 负载(N字节) |
```

- 长度：32 位无符号整数（大端序）
- 类型：8 位无符号整数
- 负载：可变长度数据

## Qt 集成

该库可以与 Qt 应用程序无缝集成。以下是一个简单的 Qt 示例：

```cpp
#include "libuv_net/client.hpp"
#include <QCoreApplication>
#include <QTimer>

class MyClient : public QObject {
    Q_OBJECT
public:
    MyClient() {
        client_ = std::make_unique<libuv_net::Client>();
        
        client_->set_connect_handler([this](bool success) {
            if (success) {
                qDebug() << "连接成功";
                // 发送测试消息
                auto message = std::make_shared<libuv_net::Message>(
                    libuv_net::MessageType::DATA,
                    std::vector<uint8_t>("Hello from Qt", 13)
                );
                client_->send(message);
            } else {
                qDebug() << "连接失败";
            }
        });
        
        client_->set_message_handler([](std::shared_ptr<libuv_net::Message> message) {
            std::string payload(message->payload.begin(), message->payload.end());
            qDebug() << "收到消息:" << QString::fromStdString(payload);
        });
        
        // 启动事件循环
        QTimer::singleShot(0, this, &MyClient::start);
    }
    
private slots:
    void start() {
        client_->connect("127.0.0.1", 8080);
    }
    
private:
    std::unique_ptr<libuv_net::Client> client_;
};
```

## 许可证

MIT 许可证 