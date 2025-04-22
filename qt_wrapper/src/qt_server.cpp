#include "libuv_net_qt/qt_server.hpp"
#include <QDebug>

namespace libuv_net_qt {

QtServer::QtServer(QObject* parent)
    : QObject(parent)
    , server_(std::make_unique<libuv_net::Server>())
{
    // 设置会话连接回调
    server_->setNewSessionCallback([this](quint64 sessionId) {
        handleNewSession(sessionId);
    });

    // 设置会话关闭回调
    server_->setSessionClosedCallback([this](quint64 sessionId) {
        handleSessionClosed(sessionId);
    });

    // 设置消息回调
    server_->setMessageCallback([this](quint64 sessionId, std::shared_ptr<libuv_net::Message> message) {
        handleMessage(sessionId, message);
    });
}

QtServer::~QtServer()
{
    stop();
}

bool QtServer::start(const QString& host, quint16 port)
{
    try {
        server_->start(host.toStdString(), port);
        return true;
    } catch (const std::exception& e) {
        emit error(QString("Failed to start server: %1").arg(e.what()));
        return false;
    }
}

void QtServer::stop()
{
    try {
        server_->stop();
    } catch (const std::exception& e) {
        emit error(QString("Failed to stop server: %1").arg(e.what()));
    }
}

void QtServer::broadcast(const QByteArray& data)
{
    try {
        server_->broadcast(data.constData(), data.size());
    } catch (const std::exception& e) {
        emit error(QString("Failed to broadcast message: %1").arg(e.what()));
    }
}

void QtServer::sendTo(quint64 sessionId, const QByteArray& data)
{
    try {
        server_->sendTo(sessionId, data.constData(), data.size());
    } catch (const std::exception& e) {
        emit error(QString("Failed to send message: %1").arg(e.what()));
    }
}

void QtServer::handleNewSession(quint64 sessionId)
{
    emit sessionConnected(sessionId);
}

void QtServer::handleSessionClosed(quint64 sessionId)
{
    emit sessionDisconnected(sessionId);
}

void QtServer::handleMessage(quint64 sessionId, std::shared_ptr<libuv_net::Message> message)
{
    QByteArray data(reinterpret_cast<const char*>(message->data()), message->size());
    emit messageReceived(sessionId, data);
}

} // namespace libuv_net_qt 