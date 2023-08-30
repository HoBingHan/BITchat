#include "sendfile.h"
#include "ui_sendfile.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include "userinfo.h"

extern userInfo user;
extern userInfo otherUser;
extern bool isOpenChat;
extern QString hostIP;
extern int hostPort;

sendFile::sendFile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::sendFile)
{
    ui->setupUi(this);

    isOpenChat = true;

    connect(timer, &QTimer::timeout, [=]()
    {
        timer->stop();
        sendData();
    });

    ui->messageLabel->setText("您即将给 " + otherUser.name + " 发送文件");

    int sendPort = 9988;

    tcpServer = new QTcpServer(this);
    tcpServer->listen(QHostAddress::Any, sendPort);

    connect(tcpServer, &QTcpServer::newConnection, [=]()
    {
        tcpSocket = tcpServer->nextPendingConnection();

        QString ip = tcpSocket->peerAddress().toString().section(":", 3, 3);
        QString showMessage = ip + " 已连接";
        ui->messageLabel->setText(showMessage);

        ui->sendfilePushButton->setEnabled(true);

        connect(tcpSocket, &QTcpSocket::readyRead, [=]()
        {
            QByteArray buffer = tcpSocket->readAll();

            if(QString(buffer) == "readySend")
            {
                this->close();
            }
        });
    });

    ui->sendfilePushButton->setEnabled(false);
}

sendFile::~sendFile()
{
    delete ui;
}

void sendFile::sendData()
{
    quint64 len = 0;

    do
    {
        char buf[1024] = {0};
        len = 0;

        len = file.read(buf, sizeof(buf));

        len = tcpSocket->write(buf, len);
        sendSize += len;
    } while(len > 0);

    QMessageBox::information(this, "成功", "发送完毕");
    this->close();
}

void sendFile::on_selectFilePushButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", "../");

    if (filePath.isEmpty())
    {
        QMessageBox::critical(this, "错误", "文件路径无效\n" + filePath);
        return;
    }

    fileName.clear();
    fileSize = 0;
    sendSize = 0;

    QFileInfo info(filePath);
    fileName = info.fileName();
    fileSize = info.size();

    file.setFileName(filePath + "Copy");

    bool isOK = file.open(QIODevice::ReadOnly)
;
    if(isOK)
    {
        ui->chooseFileLabel->setText("选择文件成功: " + filePath);
        ui->selectFilePushButton->setEnabled(false);
    }

    else
    {
        QMessageBox::critical(this, "错误", "以只读方式打开文件失败");
        return;
    }
}

void sendFile::on_sendfilePushButton_clicked()
{
    QString head = QString("%1##%2").arg(fileName).arg(fileSize);

    quint64 len = tcpSocket->write(head.toUtf8());

    if (len > 0)
    {
        timer->start(20);
    }

    else
    {
        QMessageBox::information(this, "错误", "头部信息发送失败");
        file.close();
        return;
    }
}

void sendFile::closeEvent(QCloseEvent *event)
{
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
        QString sendFileFailedMessage = QString("sendFileFailed##%1##%2").arg(user.id).arg(otherUser.name);
        tcpSocket->write(sendFileFailedMessage.toUtf8());
        tcpSocket->flush();

        isOpenChat = false;
    }

    tcpSocket->disconnectFromHost();
    tcpSocket->close();
    tcpServer->disconnect();
    tcpServer->close();
}
