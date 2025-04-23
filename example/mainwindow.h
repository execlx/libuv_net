#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "libuv_net_qt/qt_client.hpp"
#include "libuv_net_qt/qt_server.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSendClicked();
    void onStartServerClicked();
    void onStopServerClicked();

    void handleClientConnected();
    void handleClientDisconnected();
    void handleClientError(const QString &message);
    void handleClientMessage(const QByteArray &data);

    void handleServerStarted();
    void handleServerStopped();
    void handleServerError(const QString &message);
    void handleServerMessage(const QByteArray &data);

private:
    void setupUi();
    void setupConnections();

    QWidget *centralWidget_;
    QVBoxLayout *mainLayout_;
    QTextEdit *logTextEdit_;
    QLineEdit *hostLineEdit_;
    QLineEdit *portLineEdit_;
    QLineEdit *messageLineEdit_;
    QPushButton *connectButton_;
    QPushButton *disconnectButton_;
    QPushButton *sendButton_;
    QPushButton *startServerButton_;
    QPushButton *stopServerButton_;

    libuv_net_qt::QtClient *client_;
    libuv_net_qt::QtServer *server_;
};