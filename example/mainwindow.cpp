#include "mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), client_(new libuv_net_qt::QtClient(this)), server_(new libuv_net_qt::QtServer(this))
{
    setupUi();
    setupConnections();

    // 初始化客户端和服务器
    client_->initialize();
    server_->initialize();
}

MainWindow::~MainWindow()
{
    if (client_->isConnected())
    {
        client_->disconnect();
    }
    if (server_->isRunning())
    {
        server_->stop();
    }
}

void MainWindow::setupUi()
{
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);

    mainLayout_ = new QVBoxLayout(centralWidget_);

    // 日志显示区域
    logTextEdit_ = new QTextEdit(this);
    logTextEdit_->setReadOnly(true);
    mainLayout_->addWidget(logTextEdit_);

    // 连接设置
    QHBoxLayout *connectionLayout = new QHBoxLayout();
    hostLineEdit_ = new QLineEdit("127.0.0.1", this);
    portLineEdit_ = new QLineEdit("8080", this);
    connectButton_ = new QPushButton("Connect", this);
    disconnectButton_ = new QPushButton("Disconnect", this);
    disconnectButton_->setEnabled(false);

    connectionLayout->addWidget(hostLineEdit_);
    connectionLayout->addWidget(portLineEdit_);
    connectionLayout->addWidget(connectButton_);
    connectionLayout->addWidget(disconnectButton_);
    mainLayout_->addLayout(connectionLayout);

    // 消息发送
    QHBoxLayout *messageLayout = new QHBoxLayout();
    messageLineEdit_ = new QLineEdit(this);
    sendButton_ = new QPushButton("Send", this);
    sendButton_->setEnabled(false);

    messageLayout->addWidget(messageLineEdit_);
    messageLayout->addWidget(sendButton_);
    mainLayout_->addLayout(messageLayout);

    // 服务器控制
    QHBoxLayout *serverLayout = new QHBoxLayout();
    startServerButton_ = new QPushButton("Start Server", this);
    stopServerButton_ = new QPushButton("Stop Server", this);
    stopServerButton_->setEnabled(false);

    serverLayout->addWidget(startServerButton_);
    serverLayout->addWidget(stopServerButton_);
    mainLayout_->addLayout(serverLayout);
}

void MainWindow::setupConnections()
{
    // 客户端信号连接
    connect(client_, &libuv_net_qt::QtClient::connected, this, &MainWindow::handleClientConnected);
    connect(client_, &libuv_net_qt::QtClient::disconnected, this, &MainWindow::handleClientDisconnected);
    connect(client_, &libuv_net_qt::QtClient::error, this, &MainWindow::handleClientError);
    connect(client_, &libuv_net_qt::QtClient::messageReceived, this, &MainWindow::handleClientMessage);

    // 服务器信号连接
    connect(server_, &libuv_net_qt::QtServer::started, this, &MainWindow::handleServerStarted);
    connect(server_, &libuv_net_qt::QtServer::stopped, this, &MainWindow::handleServerStopped);
    connect(server_, &libuv_net_qt::QtServer::error, this, &MainWindow::handleServerError);
    connect(server_, &libuv_net_qt::QtServer::messageReceived, this, &MainWindow::handleServerMessage);

    // 按钮信号连接
    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectButton_, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(sendButton_, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(startServerButton_, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);
    connect(stopServerButton_, &QPushButton::clicked, this, &MainWindow::onStopServerClicked);
}

void MainWindow::onConnectClicked()
{
    bool ok;
    quint16 port = portLineEdit_->text().toUShort(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, "Error", "Invalid port number");
        return;
    }

    client_->connect(hostLineEdit_->text(), port);
}

void MainWindow::onDisconnectClicked()
{
    client_->disconnect();
}

void MainWindow::onSendClicked()
{
    QByteArray data = messageLineEdit_->text().toUtf8();
    client_->send(data);
    messageLineEdit_->clear();
}

void MainWindow::onStartServerClicked()
{
    bool ok;
    quint16 port = portLineEdit_->text().toUShort(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, "Error", "Invalid port number");
        return;
    }

    server_->start(hostLineEdit_->text(), port);
}

void MainWindow::onStopServerClicked()
{
    server_->stop();
}

void MainWindow::handleClientConnected()
{
    logTextEdit_->append("Client connected");
    connectButton_->setEnabled(false);
    disconnectButton_->setEnabled(true);
    sendButton_->setEnabled(true);
}

void MainWindow::handleClientDisconnected()
{
    logTextEdit_->append("Client disconnected");
    connectButton_->setEnabled(true);
    disconnectButton_->setEnabled(false);
    sendButton_->setEnabled(false);
}

void MainWindow::handleClientError(const QString &message)
{
    logTextEdit_->append("Client error: " + message);
}

void MainWindow::handleClientMessage(const QByteArray &data)
{
    logTextEdit_->append("Received: " + QString::fromUtf8(data));
}

void MainWindow::handleServerStarted()
{
    logTextEdit_->append("Server started");
    startServerButton_->setEnabled(false);
    stopServerButton_->setEnabled(true);
}

void MainWindow::handleServerStopped()
{
    logTextEdit_->append("Server stopped");
    startServerButton_->setEnabled(true);
    stopServerButton_->setEnabled(false);
}

void MainWindow::handleServerError(const QString &message)
{
    logTextEdit_->append("Server error: " + message);
}

void MainWindow::handleServerMessage(const QByteArray &data)
{
    logTextEdit_->append("Server received: " + QString::fromUtf8(data));
}