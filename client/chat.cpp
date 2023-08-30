#include "chat.h"
#include "ui_chat.h"
#include <QMessageBox>
#include <QDateTime>
#include "userinfo.h"
#include "client.h"
#include "sendfile.h"
#include "receivefile.h"

extern userInfo user;
extern userInfo otherUser;
extern bool isOpenChat;
extern QString hostIP;
extern int hostPort;

chat::chat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::chat)
{
    ui->setupUi(this);

    ui->showLabel->setText("您正在与" + otherUser.name + "进行对话");
    isOpenChat = true;

    tcpSocket = new QTcpSocket();
    timer = new QTimer();

    timer->start(500);
    connect(timer, SIGNAL(timeout()), this, SLOT(chatHistory()));
}

chat::~chat()
{
    isOpenChat = false;
    timer->stop();
    delete ui;
}

void chat::getChatHistory()
{
    tcpSocket->abort();
    tcpSocket->connectToHost(hostIP, hostPort);

    if (!tcpSocket->waitForConnected(30000))
    {
        QString errorMessage = "连接服务器失败，请检查网络连接或稍后再试。";
        QMessageBox::critical(this, "连接错误", errorMessage);

        this->close();
    }

    else
    {   // 连接服务器成功
        QString chatHistoryMessage = QString("chatHistory##%1##%2").arg(user.id).arg(otherUser.id);
        tcpSocket->write(chatHistoryMessage.toUtf8());
        tcpSocket->flush();

        connect(tcpSocket, &QTcpSocket::readyRead, [=]()
        {
            QByteArray buffer = tcpSocket->readAll();

            if (QString(buffer).section("##", 0, 0) == QString("chatHistoryNotFound"))
            {
                ui->historyTextBrowser->setText("无聊天记录");
            }

            else if (QString(buffer).section("##", 0, 0) == QString("canGetChatHistory"))
            {
                QString chatHistoryShow = "";
                int historyNum = QString(buffer).section("##", 1, 1).toInt();

                for (int i = 0; i < historyNum; i++)
                {
                    QString timeString = QString(buffer).section("##", 2 + 3 * i, 2 + 3 * i);
                    int id = QString(buffer).section("##", 3 + 3 * i, 3 + 3 * i).toInt();
                    QString message = QString(buffer).section("##", 4 + 3 * i, 4 + 3 * i);

                    QDateTime time = QDateTime::fromString(timeString, "yyyy-MM-dd hh:mm:ss.zzz");
                    QString timeShow = time.toString("MM-dd hh::mm::ss");

                    QString idShow = "";

                    if (id == user.id)
                        idShow = "我: ";

                    else
                        idShow = otherUser.name + ": ";

                    chatHistoryShow = "(" + timeShow + ") " + idShow + message + "\n" + chatHistoryShow;
                }

                ui->historyTextBrowser->setText(chatHistoryShow);
            }   // end of canGetChatHistory
        }); // end of readyRead
    }   // end of 连接服务器成功
}

void chat::on_sendToolButton_clicked()
{
    if (ui->chatLineEdit->text() == "")
        QMessageBox::warning(this, "警告", "不能发送空消息");

    else
    {
        tcpSocket->abort();
        tcpSocket->connectToHost(hostIP, hostPort);

        if (!tcpSocket->waitForConnected(30000))
        {
            QString errorMessage = "连接服务器失败，请检查网络连接或稍后再试。";
            QMessageBox::critical(this, "连接错误", errorMessage);

            this->close();
            user.islogin = false;
            client *loginWindow = new client();
            loginWindow->show();
        }

        else
        {   // 连接服务器成功
            QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
            QString message = ui->chatLineEdit->text();

            QString chatSendMessage = QString("chatSend##%1##%2##%3##%4").arg(time).arg(user.id).arg(otherUser.id).arg(message);

            tcpSocket->write(chatSendMessage.toUtf8());
            tcpSocket->flush();

            ui->chatLineEdit->clear();
        }
    }
}

void chat::on_closeToolButton_clicked()
{
    isOpenChat = false;
    this->close();
    timer->stop();
}

void chat::closeEvent(QCloseEvent *event)
{
    isOpenChat = false;
    timer->stop();
}

void chat::on_sendFileToolButton_clicked()
{
    if (otherUser.islogin == true)
    {
        tcpSocket->abort();
        tcpSocket->connectToHost(hostIP, hostPort);

        if (!tcpSocket->waitForConnected(30000))
        {
            QString errorMessage = "连接服务器失败，请检查网络连接或稍后再试。";
            QMessageBox::critical(this, "连接错误", errorMessage);

            this->close();
            user.islogin = false;
            client *loginWindow = new client();
            loginWindow->show();
        }

        else
        {   // 连接服务器成功
            QString sendFileMessage = QString("sendFile##%1##%2").arg(user.id).arg(otherUser.name);
            tcpSocket->write(sendFileMessage.toUtf8());
            tcpSocket->flush();
        }

        sendFile *sendFileWindow = new sendFile();
        sendFileWindow->show();
    }

    else
    {
        QMessageBox::critical(this, "错误",  otherUser.name + "未在线，无法发送文件");
    }
}

void chat::on_receiveFileToolButton_clicked()
{
    receiveFile *receiveFileWindow = new receiveFile();
    receiveFileWindow->show();
}
