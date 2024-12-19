#include "login.h"
#include "ui_login.h"
#include "mainwindow.h"

login::login(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::login)
{
    ui->setupUi(this);
}

login::~login()
{
    delete ui;
}

void login::on_pushButton_clicked()
{
    quint16 socketNum=ui->socketEdit->text().toUShort();
    MainWindow* mainwindow=new MainWindow();
    mainwindow->show();
    mainwindow->onStartServer(socketNum);
    this->close();

}//开始监听按钮

