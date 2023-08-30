#include "home.h"
#include "ui_home.h"
#include <QMessageBox>
#include "userinfo.h"
#include "client.h"
#include "chat.h"
#include "sendfile.h"
#include "receivefile.h"
#include <qinputdialog.h>

QString hostIP = " 10.198.96.17";
int hostPort = 8888;

extern userInfo user;
userInfo otherUser;
QList <QString> friendNameList;
QList <QString> friendIPList;
QList <QString> friendStatusList;
QList <QString> unreadFriendMessageList;
QList <QString> unreadFriendFileList;

bool isOpenChat = false;
int friendCount = -1;
int onlineFriendCount = -1;
int unreadFriendMessageCount = -1;
int unreadFriendFileCount = -1;


home::home(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::home)
{
    ui->setupUi(this);
    ui->helloLabel->setText("Hello, " + user.name);

    /*
     * 创建了一个QTimer对象，并将其设置为每500毫秒触发一次定时器事件
     * 当定时器超时timeout的时候，会自动调用槽函数updateFriendList
     * 这个定时器的作用是定期更新好友列表或执行一些其他操作
     */
    timer = new QTimer();
    timer->start(500);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateFriendList()));
}

home::~home()
{
    timer->stop();
    delete ui;
}

void home::updateFriendList()
{
    if (isOpenChat)
    {
        ui->startChatToolButton->setEnabled(false);
        ui->sendMessageToolButton->setEnabled(false);
    }

    else if (!isOpenChat && friendNameList.length() == 0)
    {
        ui->startChatToolButton->setEnabled(false);
        ui->sendMessageToolButton->setEnabled(true);
    }

    else if(!isOpenChat && friendNameList.length() != 0)
    {
        ui->startChatToolButton->setEnabled(true);
        ui->sendMessageToolButton->setEnabled(true);
    }

    qDebug() << friendNameList.length();

    tcpSocket = new QTcpSocket();
    tcpSocket->abort();
    tcpSocket->connectToHost(hostIP, hostPort);

    if (!tcpSocket->waitForConnected(3000))
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
        QString getFriendListMessage = QString("getFriendList##%1").arg(user.id);
        tcpSocket->write(getFriendListMessage.toUtf8());
        tcpSocket->flush();

        connect(tcpSocket, &QTcpSocket::readyRead, [=]()
        {
            QByteArray buffer = tcpSocket->readAll();

            if (QString(buffer).section("##", 0, 0) == QString("friendListNotFound"))
            {
                if (friendCount == -1)
                {
                    friendCount = 0;
                    ui->listWidget->clear();
                    ui->listWidget->insertItem(0, "您还未添加好友");
                }

                ui->startChatToolButton->setEnabled(false);
                ui->deleteFriendToolButton->setEnabled(false);
            }

            else if (QString(buffer).section("##", 0, 0) == QString("canGetFriendList"))
            {
                int new_friendCount = QString(buffer).section("##", 1, 1).toInt();
                int new_onlineFriendCount = QString(buffer).section("##", 2, 2).toInt();
                int new_unreadFriendMessageCount = QString(buffer).section("##", 3, 3).toInt();
                int new_unreadFriendFileCount = QString(buffer).section("##", 4, 4).toInt();

                if ( friendCount == -1
                    || onlineFriendCount == -1
                    || unreadFriendMessageCount == -1
                    || unreadFriendFileCount == -1
                    || new_friendCount != friendCount
                    || new_onlineFriendCount != onlineFriendCount
                    || new_unreadFriendFileCount != unreadFriendFileCount
                    || new_unreadFriendMessageCount != unreadFriendMessageCount
                   )
                {   // 更新好友消息列表

                    friendCount = new_friendCount;
                    onlineFriendCount = new_onlineFriendCount;
                    unreadFriendMessageCount = new_unreadFriendMessageCount;
                    unreadFriendFileCount = new_unreadFriendFileCount;

                    ui->listWidget->clear();

                    friendNameList.clear();
                    friendStatusList.clear();
                    friendIPList.clear();
                    unreadFriendMessageList.clear();
                    unreadFriendFileList.clear();

                    for (int i = 0; i < new_friendCount; i++)
                    {
                        QString friendName = QString(buffer).section("##", 5 + i * 5, 5 + i * 5);
                        QString friendStatus = QString(buffer).section("##", 6 + i * 5, 6 + i * 5);
                        QString friendIP = QString(buffer).section("##", 7 + i * 5, 7 + i * 5);
                        QString unreadFriendMessage = QString(buffer).section("##", 8 + i * 5, 8 + i * 5);
                        QString unreadFriendFile = QString(buffer).section("##", 9 + i * 5, 9 + i * 5);

                        friendNameList.append(friendName);
                        friendStatusList.append(friendStatus);
                        friendIPList.append(friendIP);
                        unreadFriendMessageList.append(unreadFriendMessage);
                        unreadFriendFileList.append(unreadFriendFile);

                        if (friendStatus == '1')
                        {   // 该好友在线中

                            if (unreadFriendMessage == '1')
                            {   // 该好友在线中，但未读该好友发来的消息

                                if (unreadFriendFile == '1')
                                {
                                    QString notice = friendName + " (在线中, 想给您发文件, 有新的聊天讯息)";
                                    ui->listWidget->insertItem(i, notice);
                                }

                                else
                                {
                                    QString notice = friendName + " (在线中, 有新的聊天讯息)";
                                    ui->listWidget->insertItem(i, notice);
                                }
                            }

                            else
                            {   // 该好友在线中，已读了该好友发来的文本消息

                                if (unreadFriendFile == '1')
                                {
                                    QString notice = friendName + " (在线中, 想给您发文件)";
                                    ui->listWidget->insertItem(i, notice);
                                }

                                else
                                {
                                    QString notice = friendName + " (在线中)";
                                    ui->listWidget->insertItem(i, notice);
                                }
                            }
                        }

                        else
                        {   // 该好友已离线

                            if (unreadFriendMessage == '1')
                            {
                                QString notice = friendName + " (已离线, 有新的聊天讯息)";
                                ui->listWidget->insertItem(i, notice);
                            }

                            else
                            {
                                QString notice = friendName + " (已离线)";
                                ui->listWidget->insertItem(i, notice);
                            }
                        }
                    }   // end of for-loop

                    ui->deleteFriendToolButton->setEnabled(true);

                    if(!isOpenChat)
                    {
                        ui->startChatToolButton->setEnabled(true);
                    }
                }   // end of 更新好友消息列表
            }   // end of canGetFriendList
        }); // end of readyRead
    }   // end of 连接服务器成功
}

void home::on_sendMessageToolButton_clicked()
{
    if (ui->friendNamelineEdit->text() == "")
    {
        QMessageBox::critical(this, "错误", "用户名不能为空");
        ui->friendNamelineEdit->clear();
        ui->friendNamelineEdit->setFocus();
    }

    else if (ui->friendNamelineEdit->text() == user.name)
    {
        QMessageBox::critical(this, "错误", "不能给自己发送消息");
        ui->friendNamelineEdit->clear();
        ui->friendNamelineEdit->setFocus();
    }

    else
    {
        tcpSocket = new QTcpSocket();
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
        {
            QString sendMessage = QString("sendMessage##%1##%2").arg(user.id).arg(ui->friendNamelineEdit->text());
            tcpSocket->write(sendMessage.toUtf8());
            tcpSocket->flush();

            connect(tcpSocket, &QTcpSocket::readyRead, [=]()
            {
                QByteArray buffer = tcpSocket->readAll();

                if (QString(buffer).section("##", 0, 0) == QString("canSendMessage"))
                {
                    otherUser.id = QString(buffer).section("##", 1, 1).toInt();
                    otherUser.name = ui->friendNamelineEdit->text();

                    ui->friendNamelineEdit->clear();
                    chat *chatWindow = new chat();
                    chatWindow->show();
                }

                else if (QString(buffer).section("##", 0, 0) == QString("sendMessageError"))
                {
                    QMessageBox::critical(this, "错误", "查无此人");
                    ui->friendNamelineEdit->clear();
                    ui->friendNamelineEdit->setFocus();
                }
            }); // end of readyRead
        }   // end of 服务器连接成功
    }   // end of senderName不违法
}


void home::on_logoutToolButton_clicked()
{
    this->close();
    user.islogin = false;
    client *loginWindow = new client();
    loginWindow->show();
}

void home::on_addFriendToolButton_clicked()
{
    //QInputDialog

    bool OK;
    QString addFriendName = QInputDialog::getText(this, "增添联系人", "请输入对方名称", QLineEdit::Normal, 0, &OK);

    if (OK && !addFriendName.isEmpty())
    {
        if (addFriendName == user.name)
            QMessageBox::critical(this, "错误", "不能添加自己");

        else
        {
            tcpSocket = new QTcpSocket();
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
            {
                QString addFriendMessage = QString("addFriend##%1##%2").arg(user.id).arg(addFriendName);
                tcpSocket->write(addFriendMessage.toUtf8());
                tcpSocket->flush();

                connect(tcpSocket, &QTcpSocket::readyRead, [=]()
                {
                    QByteArray buffer = tcpSocket->readAll();

                    if (QString(buffer).section("##", 0, 0) == QString("addFriendSuccess"))
                    {
                        onlineFriendCount = -1; // 添加好友，刷新朋友消息列表
                        QMessageBox::information(this, "", "添加成功");
                    }

                    else if (QString(buffer).section("##", 0, 0) == QString("addFriendError"))
                    {
                        QMessageBox::critical(this, "错误", "查无此人");
                    }
                }); // end of readyRead
            }   // end of 服务器连接成功
        } // end of 用户名不为自己
    }
}

void home::on_deleteFriendToolButton_clicked()
{
    if (ui->listWidget->currentRow() == -1)
        QMessageBox::critical(this, "错误", "您未选择联系人");

    else
    {
        QString friendName = friendNameList.at(ui->listWidget->currentRow());

        QString strInfo = "您将删除好友: " + friendName + "\n确定吗?";
        int result = QMessageBox::question(this, "提示", strInfo, QMessageBox::Cancel, QMessageBox::Yes);

        if (result == QMessageBox::Yes)
        {
            tcpSocket = new QTcpSocket();
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
            {
                QString deleteFriendMessage = QString("deleteFriend##%1##%2").arg(user.id).arg(friendName);
                tcpSocket->write(deleteFriendMessage.toUtf8());
                tcpSocket->flush();

                connect(tcpSocket, &QTcpSocket::readyRead, [=]()
                {
                    QByteArray buffer = tcpSocket->readAll();

                    if (QString(buffer).section("##", 0, 0) == QString("deleteFriendSuccess"))
                    {
                        updateFriendList();
                    }
                }); // end of readyRead
            }   // end of 服务器连接成功
        }   // end of Yes result
    }
}

void home::on_startChatToolButton_clicked()
{
    if (ui->listWidget->currentRow() == -1)
        QMessageBox::critical(this, "错误", "您未选择联系人");

    else
    {
        QString friendName = friendNameList.at(ui->listWidget->currentRow());

        tcpSocket = new QTcpSocket();
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
        {
            QString sendMessage = QString("sendMessage##%1##%2").arg(user.id).arg(friendName);
            tcpSocket->write(sendMessage.toUtf8());
            tcpSocket->flush();

            connect(tcpSocket, &QTcpSocket::readyRead, [=]()
            {
                QByteArray buffer = tcpSocket->readAll();

                if (QString(buffer).section("##", 0, 0) == QString("canSendMessage"))
                {
                    otherUser.id = QString(buffer).section("##", 1, 1).toInt();
                    otherUser.name = friendName;

                    ui->startChatToolButton->setEnabled(false);
                    chat *chatWindow = new chat();
                    chatWindow->show();
                }
            }); // end of readyRead
        }   // end of 服务器连接成功
    }
}

void home::on_refreshToolButton_clicked()
{

    friendCount = -1;
    onlineFriendCount = -1;
    unreadFriendMessageCount = -1;
    unreadFriendFileCount = -1;
}

void home::closeEvent(QCloseEvent *event)
{
    timer->stop();
    tcpSocket = new QTcpSocket();
    tcpSocket->abort();
    tcpSocket->connectToHost(hostIP, hostPort);

    if (!tcpSocket->waitForConnected(30000))
    {
        this->close();
        user.islogin = false;
    }

    else
    {
        QString logoutMessage = QString("logout##%1").arg(user.id);
        tcpSocket->write(logoutMessage.toUtf8());
        tcpSocket->flush();

        QMessageBox::information(this, "", "下线成功");
    }
}
