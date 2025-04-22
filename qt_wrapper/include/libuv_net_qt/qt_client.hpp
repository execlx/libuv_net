#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "libuv_net/client.hpp"

namespace libuv_net_qt {

// Qt 客户端封装类
class QtClient : public QObject {
    Q_OBJECT

public:
    explicit QtClient(QObject* parent = nullptr);
    ~QtClient();

    // 连接到服务器
    Q_INVOKABLE bool connect(const QString& host, quint16 port);
    // 断开连接
    Q_INVOKABLE void disconnect();
    // 发送消息
    Q_INVOKABLE void send(const QByteArray& data);
    // 检查是否已连接
    Q_INVOKABLE bool isConnected() const;

signals:
    // 连接状态改变信号
    void connected();
    void disconnected();
    // 收到消息信号
    void messageReceived(const QByteArray& data);
    // 错误信号
    void error(const QString& message);

private:
    // 处理连接结果
    void handleConnect(bool success);
    // 处理消息
    void handleMessage(std::shared_ptr<libuv_net::Message> message);
    // 处理关闭
    void handleClose();

    std::unique_ptr<libuv_net::Client> client_;
};

} // namespace libuv_net_qt 