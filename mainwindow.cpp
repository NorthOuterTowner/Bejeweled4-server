void sqlite_Init();  // 在这里声明 sqlite_Init()

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    tcpServer = new QTcpServer(this);
    ui->setupUi(this);
    // 初始化数据库
    sqlite_Init();
}

void sqlite_Init()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("user.db");
    if (!db.open())
    {
        qDebug() << "打开数据库失败";
    }
    // 创建表格
    QString createsql = QString("create table if not exists data(id integer primary key autoincrement,"
                                "username ntext unique not NULL,"
                                "password ntext not NULL,"
                                "total_score INTEGER DEFAULT 0, "
                                "high_score INTEGER DEFAULT 0)");
    QSqlQuery query;
    if (!query.exec(createsql)) {
        qDebug() << "创建表时出错：" << query.lastError().text();
    }
    else {
        qDebug() << "创建表格成功";
    }
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
    QListWidgetItem* newItem=new QListWidgetItem("用户："+clientSockets[clientSocket]);
    clientSocketLabels[clientSocket]=newItem;
    ui->listWidget->addItem(newItem);
    QMessageBox::information(this, "用户连接", clientSockets[clientSocket]+"连接成功");
}//新用户连接

void MainWindow::onReadyRead() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll();
    QString receivedData = QString::fromUtf8(data);

    QStringList parts = receivedData.split(" ");
    QString request=parts[0];
    if(request=="a"){
        QString username=parts[1];
        QString password=parts[2];
        QString sql = QString("select * from data where username='%1' and password='%2'")
                          .arg(username).arg(password);
        // 创建执行语句对象
        QSqlQuery query(sql);
        // 判断执行结果
        if (!query.next()) {
            qDebug() << "执行查询时出错：" << query.lastError().text();
            QString out="a 0";
            onSendData(out,clientSocket);
        }
        else {
            QString out="a 1";
            onSendData(out,clientSocket);
            clientSockets[clientSocket] = username;
            clientSocketLabels[clientSocket]->setText("用户 "+username);
        }
        //用户登录
    }else if(request=="b"){
        QString username=parts[1];
        QString password=parts[2];
        int totalScore = 0;  // 总分默认值
        int highScore = 0;   // 最高分默认值
        QString sql = QString(
                          "INSERT INTO data(username, password, total_score, high_score) "
                          "VALUES('%1', '%2', %3, %4);"
                          ).arg(username).arg(password).arg(totalScore).arg(highScore);
        QSqlQuery query;
        if(!query.exec(sql))
        {
            qDebug() << "执行查询时出错：" << query.lastError().text();
            QString out="a 0";
            onSendData(out,clientSocket);
        }
        else
        {
            QString out="a 1";
            onSendData(out,clientSocket);
        }
        //注册用户
    }else if(request=="c"){
        QString username=clientSockets[clientSocket];
        QString sql = QString("SELECT total_score, high_score FROM data WHERE username = '%1';").arg(username);
        int curscore=parts[1].toInt();
        QSqlQuery query;
        int totalScore = query.value(0).toInt();  // 获取 total_score 列
        int highScore = query.value(1).toInt();   // 获取 high_score 列
        if (query.exec(sql)) {
            // 检查查询结果
            if (query.next()) {
                if(curscore>highScore){
                    highScore=curscore;
                }
                totalScore+=highScore;
            } else {
                qDebug() << "未找到用户名：" << username;
            }
        } else {
            qDebug() << "执行查询时出错：" << query.lastError().text();
        }
        sql = "UPDATE data SET total_score = :totalScore, high_score = :highScore WHERE username = :username;";
        query.prepare(sql);

        // 绑定参数
        query.bindValue(":username", username);       // 绑定用户名
        query.bindValue(":totalScore", totalScore);   // 绑定总分
        query.bindValue(":highScore", highScore);     // 绑定最高分
        // 执行查询
        if (query.exec()) {
            qDebug() << "用户数据更新成功！";
        } else {
            qDebug() << "执行更新查询时出错：" << query.lastError().text();
        }
        //更新总分与最高分
    }else if(request=="d"){
        QString out="b ";
        QSqlQuery query;

        // 查询总数
        int totalUsernames;
        QString countQuery = "SELECT COUNT(username) AS total_usernames FROM data;";
        if (query.exec(countQuery)) {
            if (query.next()) {
                totalUsernames = query.value("total_usernames").toInt();
                qDebug() << "用户名总数：" << totalUsernames;
                out+=(QString::number(totalUsernames)+" ");
            } else {
                qDebug() << "未能获取用户名总数。";
            }
        } else {
            qDebug() << "查询总数时出错：" << query.lastError().text();
        }

        // 查询所有 username 和 high_score
        QString userQuery = "SELECT username, high_score FROM data;";
        if (query.exec(userQuery)) {
            qDebug() << "用户名和对应的最高分：";
            while (query.next()) {
                QString username = query.value("username").toString();
                int highScore = query.value("high_score").toInt();
                out+=(username+" "+QString::number(highScore)+" ");
            }
        } else {
            qDebug() << "查询用户名和分数时出错：" << query.lastError().text();
        }
        onSendData(out,clientSocket);
    }
    //输送排行信息
}

void MainWindow::onSendData(QString data,QTcpSocket* tcpServer) {
    tcpServer->write(data.toUtf8());
}//发送信息

void MainWindow::onDisconnected() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        QMessageBox::information(this, "用户连接", clientSockets[clientSocket]+"断开连接");
        clientSockets.remove(clientSocket);
        ui->listWidget->takeItem(ui->listWidget->row(clientSocketLabels[clientSocket]));
        clientSocketLabels.remove(clientSocket);
        clientSocket->deleteLater();
    }
}
