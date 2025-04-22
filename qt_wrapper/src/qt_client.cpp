#include "libuv_net_qt/qt_client.hpp"
#include <QDebug>

namespace libuv_net_qt {

QtClient::QtClient(QObject* parent)
    : QObject(parent)
    , client_(std::make_unique<libuv_net::Client>())
{
    // 设置连接处理回调
    client_->set_connect_handler([this](bool success) {
        handleConnect(success);
    });

    // 设置消息处理回调
    client_->set_message_handler([this](std::shared_ptr<libuv_net::Message> message) {
        handleMessage(message);
    });

    // 设置关闭处理回调
    client_->set_close_handler([this]() {
        handleClose();
    });
}

QtClient::~QtClient()
{
    disconnect();
}

bool QtClient::connect(const QString& host, quint16 port)
{
    return client_->connect(host.toStdString(), port);
}

void QtClient::disconnect()
{
    client_->disconnect();
}

void QtClient::send(const QByteArray& data)
{
    if (!client_->is_connected()) {
        emit error("未连接到服务器");
        return;
    }

    auto message = std::make_shared<libuv_net::Message>(
        libuv_net::MessageType::DATA,
        std::vector<uint8_t>(data.begin(), data.end())
    );

    client_->send(message);
}

bool QtClient::isConnected() const
{
    return client_->is_connected();
}

void QtClient::handleConnect(bool success)
{
    if (success) {
        emit connected();
    } else {
        emit error("连接失败");
    }
}

void QtClient::handleMessage(std::shared_ptr<libuv_net::Message> message)
{
    QByteArray data(reinterpret_cast<const char*>(message->payload.data()),
                   message->payload.size());
    emit messageReceived(data);
}

void QtClient::handleClose()
{
    emit disconnected();
}

} // namespace libuv_net_qt 