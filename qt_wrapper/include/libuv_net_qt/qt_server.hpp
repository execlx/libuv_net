#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include <libuv_net/server.hpp>

namespace libuv_net_qt {

class QtServer : public QObject
{
    Q_OBJECT

public:
    explicit QtServer(QObject* parent = nullptr);
    ~QtServer();

    // 启动服务器
    bool start(const QString& host, quint16 port);
    // 停止服务器
    void stop();
    // 广播消息
    void broadcast(const QByteArray& data);
    // 发送消息到指定会话
    void sendTo(quint64 sessionId, const QByteArray& data);

signals:
    // 新会话连接
    void sessionConnected(quint64 sessionId);
    // 会话断开
    void sessionDisconnected(quint64 sessionId);
    // 收到消息
    void messageReceived(quint64 sessionId, const QByteArray& data);
    // 错误信息
    void error(const QString& message);

private:
    // 处理新会话
    void handleNewSession(quint64 sessionId);
    // 处理会话关闭
    void handleSessionClosed(quint64 sessionId);
    // 处理消息
    void handleMessage(quint64 sessionId, std::shared_ptr<libuv_net::Message> message);

    std::unique_ptr<libuv_net::Server> server_;
};

} // namespace libuv_net_qt 