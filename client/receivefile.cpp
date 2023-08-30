#include "receivefile.h"
#include "ui_receivefile.h"
#include <QMessageBox>
#include <QFileDialog>
#include "userinfo.h"
#include <qdir.h>

extern userInfo user;
extern userInfo otherUser;
extern bool isOpenChat;
extern QString hostIP;
extern int hostPort;

receiveFile::receiveFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::receiveFile)
{
    ui->setupUi(this);

    ui->progressBar->setValue(0);
    isOpenChat = true;

    ui->messageLabel->setText("等待与 " + otherUser.name + " 连接");

    port = 9988;
    isStart = true;

    tcpSocket = new QTcpSocket(this);
    tcpSocket->connectToHost(otherUser.ip, port);

    connect(tcpSocket, &QTcpSocket::connected, [=]()
    {
        ui->messageLabel->setText("连接成功， 等待 " + otherUser.name + " 发送");
    });

    connect(tcpSocket, &QTcpSocket::readyRead, [=]()
    {
        QByteArray buf = tcpSocket->readAll();

        if (isStart)
        {
            isStart = false;
            fileName = QString(buf).section("##", 0, 0);
            fileSize = QString(buf).section("##", 1, 1).toInt();

            ui->messageLabel->setText("发现文件 " + fileName);
            receiveSize = 0;

            file.setFileName("./" + fileName);

            bool isOK = file.open(QIODevice::WriteOnly | QIODevice::Append);

            if (isOK == false)
            {
                tcpSocket->disconnectFromHost();
                tcpSocket->close();
                QMessageBox::information(this, "错误", "打开文件错误");
                return;
            }

            ui->progressBar->setMinimum(0);
            ui->progressBar->setMaximum(fileSize);
            ui->progressBar->setValue(0);
        }

        else
        {
            ui->messageLabel->setText("正在接收文件");

            quint64 len = file.write(buf);

            if (len > 0)
                receiveSize += len;

            ui->progressBar->setValue(receiveSize);

            if (receiveSize == fileSize)
            {
                file.close();
                ui->messageLabel->setText("成功接受文件");

                //QWidget *widget = new QWidget();
                QString saveFileName = QFileDialog::getSaveFileName(this, "保存文件", "./" + fileName, nullptr);

                this->close();
                ui->progressBar->setValue(0);
            }
        }
    });
}

receiveFile::~receiveFile()
{
    delete ui;
}

void receiveFile::closeEvent(QCloseEvent *event)
{
    tcpSocket = new QTcpSocket();
    tcpSocket->abort();
    tcpSocket->connectToHost(otherUser.ip, port);

    QString readySendMessage = QString("readySend");
    tcpSocket->write(readySendMessage.toUtf8());
    tcpSocket->flush();

    tcpSocket->disconnectFromHost();
    tcpSocket->close();

    tcpSocket = new QTcpSocket();
    tcpSocket->abort();
    tcpSocket->connectToHost(hostIP, hostPort);

    if (!tcpSocket->waitForConnected(30000))
    {
//        QString errorMessage = "连接服务器失败，请检查网络连接或稍后再试。";
//        QMessageBox::critical(this, "连接错误", errorMessage);

        this->close();
        user.islogin = false;
    }

    else
    {   // 连接服务器成功
        QString sendFileSuccessMessage = QString("sendFileSuccess##%1##%2").arg(user.id).arg(otherUser.name);
        tcpSocket->write(sendFileSuccessMessage.toUtf8());
        tcpSocket->flush();

        isOpenChat = false;
    }

    tcpSocket->disconnectFromHost();
    tcpSocket->close();
}
