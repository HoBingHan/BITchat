#include "client.h"
#include "ui_client.h"
#include <QGraphicsDropShadowEffect>
#include <QtNetwork>
#include <QMessageBox>
#include "userinfo.h"
#include "signUp.h"
#include "home.h"

extern userInfo user;
extern QString hostIP;
extern int hostPort;

client::client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::client)
{
    ui->setupUi(this);

    //设置图片阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(-3, 0);
    shadow->setColor(QColor("#888888"));
    shadow->setBlurRadius(30);
    ui->backgroundLabel->setGraphicsEffect(shadow);

    tcpSocket = new QTcpSocket(this);
}

client::~client()
{
    delete tcpSocket;
    delete ui;
}


void client::on_loginPushButton_clicked()
{
    if (ui->userNameLineEdit->text() == "")
    {
        QMessageBox::critical(this, "WARNING", "Username cannot be empty\n用户名不能为空");
        ui->userNameLineEdit->clear();
        ui->userNameLineEdit->setFocus();
    }

    else if (ui->passwordLineEdit->text() == "")
    {
        QMessageBox::critical(this, "WARNING", "Password cannot be empty\n密码不能为空");
        ui->passwordLineEdit->clear();
        ui->passwordLineEdit->setFocus();
    }

    else
    {
        // 中断当前的tcp套接字连接，并重新连接
        tcpSocket->abort();

        //连接服务器(ip地址，端口号)
        tcpSocket->connectToHost(hostIP, hostPort);

        QString ip = tcpSocket->peerAddress().toString().section(":", 3, 3);
        int port = tcpSocket->peerPort();

//        // 输出[ip : port]
//        QString str = QString("ConnectingToHost...[%1 : %2]").arg(ip).arg(port);
//        qDebug() << str;

        // 30秒内等待套接字成功连接到服务器，成功返回true，超时或者连接失败返回false
        if(!tcpSocket->waitForConnected(30000))
        {
            QString errorMessage = "连接服务器失败，请检查网络连接或稍后再试。";
            QMessageBox::critical(this, "连接错误", errorMessage);

            QString str = QString("ConnectFailed...[%1 : %2]").arg(ip).arg(port);
            qDebug() << str;
        }

        else
        {   // 服务器连接成功

            QString userName = ui->userNameLineEdit->text();
            QString userPassword = ui->passwordLineEdit->text();

            QString loginMessage = QString("login##%1##%2").arg(userName).arg(userPassword);
            tcpSocket->write(loginMessage.toUtf8());
            tcpSocket->flush();

//            QString str = QString("Connected...[%1 : %2]").arg(ip).arg(port);
//            qDebug() << str;

            connect(tcpSocket, &QTcpSocket::readyRead, [=]()
            {
                QByteArray buffer = tcpSocket->readAll();

                if (QString(buffer).section("##", 0, 0) == QString("loginSuccess"))
                {   // 登陆成功

                    user.id = QString(buffer).section("##", 1, 1).toInt();
                    user.name = ui->userNameLineEdit->text();
                    user.islogin = true;

                    this->close();
                    home *homeWindow = new home();
                    homeWindow->show();
                }

                else if (QString(buffer).section("##", 0, 0) == QString("loginError"))
                {   // 登录失败

                    if (QString(buffer).section("##", 1, 1) == QString("userNotFound"))
                    {   // 用户不存在

                        QString errorMessage = "用户不存在，请检查用户名或重新注册。";
                        QMessageBox::critical(this, "错误", errorMessage);

                        ui->userNameLineEdit->clear();
                        ui->passwordLineEdit->clear();
                        ui->userNameLineEdit->setFocus();
                    }

                    else if (QString(buffer).section("##", 1, 1) == QString("wrongPassword"))
                    {   // 密码错误

                        QString errorMessage = "密码错误，请输入正确的密码。";
                        QMessageBox::critical(this, "错误", errorMessage);
                        ui->passwordLineEdit->clear();
                        ui->passwordLineEdit->setFocus();
                    }
                }   // end of 登录失败
            }); // end of readyRead
        }   // end of 服务器连接成功
    }   // end of 用户名和密码都不为空
}

void client::on_registerPushButton_clicked()
{
    this->close();
    signUp *signUpWindow = new signUp();
    signUpWindow->show();
}
