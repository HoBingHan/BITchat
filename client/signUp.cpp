#include "signUp.h"
#include "ui_signUp.h"
#include <QMessageBox>
#include "userinfo.h"
#include "client.h"

extern userInfo user;
extern QString hostIP;
extern int hostPort;

signUp::signUp(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::signUp)
{
    ui->setupUi(this);

    tcpSocket=  new QTcpSocket();
}

signUp::~signUp()
{
    delete ui;
}

void signUp::on_commitPushButton_clicked()
{
    if (ui->userNameLineEdit->text() == "")
    {
        QString errorMessage = "用户名不能为空，请填写用户名。";
        QMessageBox::critical(this, "错误", errorMessage);
        ui->userNameLineEdit->clear();
        ui->userNameLineEdit->setFocus();
    }

    else if (ui->passwordLineEdit->text() == "" || ui->confirmPasswordLineEdit->text() == "")
    {
         QString errorMessage = "密码不能为空，请填写密码和确认密码。";
         QMessageBox::critical(this, "错误", errorMessage);
         ui->passwordLineEdit->clear();
         ui->confirmPasswordLineEdit->clear();
         ui->passwordLineEdit->setFocus();
    }

    else if (ui->passwordLineEdit->text() != ui->confirmPasswordLineEdit->text())
    {
        QString errorMessage = "密码不匹配，请确认密码输入一致。";
        QMessageBox::critical(this, "错误", errorMessage);
        ui->passwordLineEdit->clear();
        ui->confirmPasswordLineEdit->clear();
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

            this->close();
            user.islogin = false;
            client *loginWindow = new client();
            loginWindow->show();

            QString str = QString("ConnectFailed...[%1 : %2]").arg(ip).arg(port);
            qDebug() << str;
        }

        else
        {   // 服务器连接成功

            QString userName = ui->userNameLineEdit->text();
            QString userPassword = ui->passwordLineEdit->text();

            QString registerMessage = QString("register##%1##%2").arg(userName).arg(userPassword);
            tcpSocket->write(registerMessage.toUtf8());
            tcpSocket->flush();

//            QString str = QString("Connected...[%1 : %2]").arg(ip).arg(port);
//            qDebug() << str;

            connect(tcpSocket, &QTcpSocket::readyRead, [=]()
            {
                QByteArray buffer = tcpSocket->readAll();

                if (QString(buffer).section("##", 0, 0) == QString("registerSuccess"))
                {   // 注册成功
                    this->close();
                    client *loginWindow = new client();
                    loginWindow->show();
                }

                else if (QString(buffer).section("##", 0, 0) == QString("registerError"))
                {
                    if (QString(buffer).section("##", 1, 1) == QString("nameConflict"))
                    {
                        QString errorMessage = "用户名已存在，请选择其他用户名。";
                        QMessageBox::critical(this, "错误", errorMessage);
                        ui->userNameLineEdit->clear();
                        ui->userNameLineEdit->setFocus();
                    }
                }
            });
        }
    }
}

void signUp::on_returnPushButton_clicked()
{
    this->close();
    client *loginWindow = new client();
    loginWindow->show();
}
