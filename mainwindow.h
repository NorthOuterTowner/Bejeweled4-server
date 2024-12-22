#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QLabel>
#include <QListWidget>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void onStartServer(quint16 port);      // 启动服务器

private slots:
    void onNewConnection();    // 新客户端连接
    void onSendData(QString,QTcpSocket*);         // 发送
    void onReadyRead();        // 读取数据
    void onDisconnected();     // 客户端断开

private:
    Ui::MainWindow *ui;
    QTcpServer *tcpServer;
    QMap<QTcpSocket*, QString> clientSockets; // 保存连接的客户端信息
    QMap<QTcpSocket*, QListWidgetItem*> clientSocketLabels; // 保存连接的客户端信息
    QMap<QString, QString> accountData;       // 保存账户和密码
};

#endif // MAINWINDOW_H
