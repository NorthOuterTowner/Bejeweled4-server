#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    tcpServer = new QTcpServer(this);
    ui->setupUi(this);



}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onStartServer(quint16 port) {
    tcpServer->listen(QHostAddress::AnyIPv4, port);
    connect(tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    ui->portLabel->setText(QString::number(port));
}

void MainWindow::onNewConnection() {
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);

    clientSockets[clientSocket] = "游客";
    ui->listWidget->addItem("用户："+clientSockets[clientSocket]);
}

void MainWindow::onReadyRead() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll();
    QString receivedData = QString::fromUtf8(data);

    QStringList parts = receivedData.split(" ");
    QString request=parts[0];
    if(request=="a"){
        QString username=parts[1];
        clientSockets[clientSocket] = username;
        clientSocketLabels[clientSocket]->setText("用户 "+clientSockets[clientSocket]);
        QMessageBox::information(this, "用户连接", clientSockets[clientSocket]+"连接成功");
    }
    if (parts.size() == 2) {
        QString username = parts[0];
        QString password = parts[1];

        saveAccount(username, password);

        QString response = "账户已保存";
        clientSocket->write(response.toUtf8());
    } else {
        clientSocket->write("数据格式错误");
    }
}

void MainWindow::onDisconnected() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        //ui->logText->append("客户端断开：" + clientSocket->peerAddress().toString());
        clientSockets.remove(clientSocket);
        clientSocket->deleteLater();
    }
}

void MainWindow::saveAccount(const QString &username, const QString &password) {
    accountData[username] = password;

    QFile file("accounts.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << username << ";" << password << "\n";
        file.close();
    }
}
