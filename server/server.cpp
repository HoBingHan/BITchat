#include "server.h"
#include "ui_server.h"
#include <QSqlQuery>
#include <QSqlError>

int currentCount;   // 当前服务器与客户端建立连接的数量

server::server(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::server)
{
    ui->setupUi(this);

    tcpServer = new QTcpServer(this);

    ui->listWidget->clear();
    ui->listWidget->insertItem(0, "当前没有在线用户");

    for (int i = 0; i < N; i++)
        tcpSocket[i] = new QTcpSocket(this);

    tcpServer->listen(QHostAddress::Any, 8888);

    // 创建一个数据库连接对象,设置数据库连接类型为QSQLite
    db = QSqlDatabase::addDatabase("QSQLITE");

    // 设置数据库名称
    db.setDatabaseName("./people.db");

    // 打开数据库连接，使得可以执行SQL相关操作
    db.open();

    QString sqlString = "CREATE TABLE if not exists people"
                        "("
                            "id INTEGER NOT NULL UNIQUE,"
                            "name TEXT NOT NULL UNIQUE,"
                            "password TEXT NOT NULL,"
                            "ip TEXT,"
                            "islogin INTEGER NOT NULL,"
                            "PRIMARY KEY(id AUTOINCREMENT)"
                        ")";

    // 创建一个QSqlQuery对象，用于执行SQL查询和操作数据库表
    QSqlQuery sqlQuery;
    sqlQuery.exec(sqlString);

    // 关闭数据库连接
    db.close();

    /*
     * server监听来自客户端的请求连接，
     * 当有新的连接请求到来时，
     * server则发出newConnection信号，
     * 执行lambda表达式中的代码块
    */
    connect(tcpServer, &QTcpServer::newConnection, [=]()
    {
        // server尝试和该客户端建立起链接，返回一个已连接的客户端QTcpSocket
        tcpSocket[0] = tcpServer->nextPendingConnection();

        currentCount++;

        /*
         * 获取客户端的ip和port
         * peerAddress()返回QHostAddress
         * toString()将对象类型转换为字符串
         * section(":", 3, 3)以":"作为分隔符，获取ip地址(x.x.x.x)的第四个分段
        */
        QString ip = tcpSocket[0]->peerAddress().toString().section(":", 3, 3);
        int port = tcpSocket[0]->peerPort();

        // 输出[ip : port]
        QString str = QString("[%1 : %2]").arg(ip).arg(port);
        qDebug() << str;

        /*
         * 当套接字接收到新的数据时，就会发出readyRead信号
         * 根据接收到客户端发送的信息，执行以下不同的操作
        */
        connect(tcpSocket[0], &QTcpSocket::readyRead, [=]()
        {
            //读取缓冲区数据
            QByteArray buffer = tcpSocket[0]->readAll();
            QString identifier = QString(buffer).section("##", 0, 0);


            // buffer = "login##[userName]##[userPassword]"

            if ("login" == identifier)
            {
                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;

                // ":name"是一个命名绑定占位符，可以通过bindValue将其替换为具体的值
                sqlQuery.prepare("SELECT * FROM people WHERE name = :name");

                // 将":name"绑定到buffer中提取的用户名部分
                sqlQuery.bindValue(":name", QString(buffer).section("##", 1, 1));
                sqlQuery.exec();

                /*
                 * 检查SQL查询是否找到与提供的用户名匹配的行
                 * 如果查询没有结果，sqlQuery.next()返回false，否则返回true
                */

                if(!sqlQuery.next())
                {   // 查询不到该用户
                    /*
                     * 通过已建立的套接字tcpSocket向客户端发送一条消息
                     * write( QByteArray& )
                     * QString("消息文本").toUtf8()将字符串转换为UTF-8编码的字节流
                     */
                    tcpSocket[0]->write(QString("loginError##userNotFound").toUtf8());

                    /*
                     * 通过已建立的套接字tcpSocket刷新输出缓冲区
                     * 它用于将输出缓冲区中的数据立即发送到连接的客户端
                     * 在网络通信中，写入到套接字的数据通常会暂时存储在输出缓冲区中，
                     * 直到缓冲区被填满或者显式调用flush方法时，才会发送到实际的网络连接
                    */
                    tcpSocket[0]->flush();

                    db.close();
                }   // end of userNotFound

                else
                {   // 存在该用户
                    // 从数据库查询结果中获取该用户的id和password
                    int userId = sqlQuery.value(0).toInt();
                    QString userPassword = sqlQuery.value(2).toString();

                    if (userPassword == QString(buffer).section("##", 2, 2))
                    {   // 登录成功
                        tcpSocket[0]->write(QString("loginSuccess##%1").arg(userId).toUtf8());

                        // 更新用户的ip地址，也将其islogin设置为1
                        sqlQuery.prepare("UPDATE people SET ip = :ip, islogin = 1 WHERE name = :name");
                        sqlQuery.bindValue(":ip", ip);
                        sqlQuery.bindValue(":name", QString(buffer).section("##", 1, 1));
                        sqlQuery.exec();

                        // 将数据从套接字发送到客户端
                        tcpSocket[0]->flush();

                        // 更新服务器界面
                        ui->listWidget->clear();

                        sqlQuery.prepare("SELECT * FROM people WHERE islogin = 1");
                        sqlQuery.exec();

                        if (sqlQuery.next())
                        {   // 查询得到上线用户
                            QString onlineUserId = sqlQuery.value(0).toString();
                            QString onlineUserName = sqlQuery.value(1).toString();
                            QString onlineUserIP = sqlQuery.value(3).toString();

                            QString str = QString("userId: %1, userName: %2, userIP: %3").arg(onlineUserId).arg(onlineUserName).arg(onlineUserIP);
                            ui->listWidget->addItem(str);

                            while (sqlQuery.next())
                            {
                                QString onlineUserId = sqlQuery.value(0).toString();
                                QString onlineUserName = sqlQuery.value(1).toString();
                                QString onlineUserIP = sqlQuery.value(3).toString();

                                QString str = QString("userId: %1, userName: %2, userIP: %3").arg(onlineUserId).arg(onlineUserName).arg(onlineUserIP);
                                ui->listWidget->addItem(str);
                            }
                        }   // end of userIsLoginExists

                        else
                        {   // 查询不到在线用户
                            ui->listWidget->insertItem(0, "当前没有在线用户");
                        }

                        db.close();
                    }   // end of loginSuccess

                    else
                    {   // 密码错误
                        tcpSocket[0]->write(QString("loginError##wrongPassword").toUtf8());
                        tcpSocket[0]->flush();
                        db.close();
                    }
                }   // end of userExists
            }   // end of login


            // buffer = "register##[userName]##[userPassword]"

            else if("register" == identifier)
            {
                QString userName = QString(buffer).section("##", 1, 1);
                QString userPassword = QString(buffer).section("##", 2, 2);

                db.setDatabaseName("./people.db");
                db.open();

                // 注册用户的时候需要对名字判重
                QSqlQuery sqlQuery;
                sqlQuery.prepare("SELECT * FROM people WHERE name = :name");
                sqlQuery.bindValue(":name", userName);
                sqlQuery.exec();

                if (!sqlQuery.next())
                {   //没有重名，新用户可以注册

                    // 清除当前查询结果，以便重新使用它来执行新的操作
                    sqlQuery.clear();
                    sqlQuery.prepare("INSERT INTO people VALUES (null, :name, :password, null, 0)");
                    sqlQuery.bindValue(":name", userName);
                    sqlQuery.bindValue(":password", userPassword);
                    sqlQuery.exec();

                    sqlQuery.clear();
                    sqlQuery.prepare("SELECT * FROM people WHERE name = :name");
                    sqlQuery.bindValue(":name", userName);
                    sqlQuery.exec();
                    sqlQuery.next();

                    // 获取新建用户的id
                    int newUserId = sqlQuery.value(0).toInt();

                    /*
                     * 为新用户创建一个有关他的朋友列表的数据表，表名为 "user" + newUserId + "_friendList"
                     * user[userId]_friendList(id, name, unreadFriendMessage, unreadFriendFile)
                     * id:他的朋友id
                     * name:他的朋友name
                     * unreadFriendMessage:表示他对朋友消息是否未读，‘1’表示他未读朋友，‘0’表示他已读朋友
                     * unreadFriendFile:表示他对朋友送过来文件是否未读，‘1’表示他未读朋友，‘0’表示他已读朋友
                    */
                    QString sqlString = "CREATE TABLE if not exists "
                                        "user" + QString::number(newUserId) + "_friendList "
                                        "("
                                            "id INTEGER UNIQUE,"
                                            "name TEXT,"
                                            "unreadFriendMessage INTEGER,"
                                            "unreadFriendFile INTEGER"
                                        ")";

                    sqlQuery.exec(sqlString);

                    tcpSocket[0]->write(QString("registerSuccess").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }   // end of userCanRegister

                else
                {   // 有重名，新用户不能注册
                    tcpSocket[0]->write(QString("registerError##nameConflict").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }
            }   // end of register


            // buffer = "sendMessage##[senderId]##[receiverName]"

            else if("sendMessage" == identifier)
            {
                QString receiverName = QString(buffer).section("##", 2, 2);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                sqlQuery.prepare("SELECT * FROM people WHERE name = :name");
                sqlQuery.bindValue(":name", receiverName);
                sqlQuery.exec();

                if (sqlQuery.next())
                {   // 接收者有这个人存在，可以发消息

                    int receiverId = sqlQuery.value(0).toInt();
                    tcpSocket[0]->write(QString("canSendMessage##%1").arg(receiverId).toUtf8());
                    tcpSocket[0]->flush();

                    // 发消息前，先把关于他们的聊天数据表准备好
                    int senderId = QString(buffer).section("##", 1, 1).toInt();

                    /*
                     * 如果senderId小于receiverId，
                     * 则创建表名为 [senderId]_chat_[receiverId]，
                     * 否则创建表名为 [receiverId]_chat_[senderId]。
                     * 这是为了确保不论谁发起了聊天，都只创建一个唯一的聊天数据表来存储他们之间的消息记录
                     *
                     * [userA_Id]_chat_[userB_Id](time, id, message)
                    */
                    if (senderId < receiverId)
                    {
                        QString sqlString = "CREATE TABLE if not exists " +
                                            QString::number(senderId) + "_chat_" + QString::number(receiverId) +
                                            "("
                                                "time DATETIME NOT NULL UNIQUE,"
                                                "id INTEGER,"
                                                "message TEXT,"
                                                "PRIMARY KEY(time)"
                                            ")";

                        sqlQuery.exec(sqlString);
                    }

                    else
                    {
                        QString sqlString = "CREATE TABLE if not exists " +
                                            QString::number(receiverId) + "_chat_" + QString::number(senderId) +
                                            "("
                                                "time DATETIME NOT NULL UNIQUE,"
                                                "id INTEGER,"
                                                "message TEXT,"
                                                "PRIMARY KEY(time)"
                                            ")";

                        sqlQuery.exec(sqlString);
                    }

                    db.close();
                }   // end of senderCanSendMessage

                else
                {   // 查无此人，无法对话
                    tcpSocket[0]->write(QString("sendMessageError").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }
            }   // end of sendMessage


            // buffer = "chatHistory##[viewerId]##[targetId]"

            else if("chatHistory" == identifier)
            {
                int viewerId = QString(buffer).section("##", 1, 1).toInt();
                int targetId=  QString(buffer).section("##", 2, 2).toInt();

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                QString sqlString = "";

                if (viewerId < targetId)
                {
                    sqlString = "SELECT * FROM " +
                                QString::number(viewerId) + "_chat_" + QString::number(targetId) +
                                "ORDER BY time DESC limit 30";
                }

                else
                {
                    sqlString = "SELECT * FROM " +
                                QString::number(targetId) + "_chat_" + QString::number(viewerId) +
                                "ORDER BY time DESC limit 30";
                }

                sqlQuery.exec(sqlString);

                if (sqlQuery.next())
                {   // 历史聊天记录存在
                    QDateTime time = sqlQuery.value(0).toDateTime();
                    QString timeString = time.toString("yyyy-MM-dd hh:mm:ss.zzz");
                    QString history = "##" + timeString +
                                      "##" + sqlQuery.value(1).toString() +
                                      "##" + sqlQuery.value(2).toString();

                    int historyCount = 1;

                    while (sqlQuery.next())
                    {
                        QDateTime time = sqlQuery.value(0).toDateTime();
                        QString timeString = time.toString("yyyy-MM-dd hh:mm:ss.zzz");

                        history = history +
                                  "##" + timeString +
                                  "##" + sqlQuery.value(1).toString() +
                                  "##" + sqlQuery.value(2).toString();

                        historyCount++;
                    }

                    history = "canGetChatHistory##" + QString::number(historyCount) + history;

                    tcpSocket[0]->write(history.toUtf8());
                    tcpSocket[0]->flush();

                    db.setDatabaseName("./people.db");
                    db.open();

                    /*
                     * user[viewerId]_friendList(targetId, targetName, unreadFriendMessage, unreadFriendFile)
                     * viewer:聊天记录查看者
                     * target:聊天对象
                     * 打开聊天记录之后，viewer已经已读了target的所有消息
                     * 因此在viewer的朋友列表里，找到target
                     * 把viewer对target的未读参数unreadFriendMessage设置为0
                     * 表示viewer已读了target
                     */
                    QSqlQuery sqlQuery;
                    QString sqlString = "UPDATE user" + QString::number(viewerId) + "_friendList "
                                        "SET unreadFriendMessage = 0 WHERE id = :id";

                    sqlQuery.prepare(sqlString);
                    sqlQuery.bindValue(":id", targetId);
                    sqlQuery.exec();
                    db.close();
                }   // end of chatHistoryExists

                else
                {   // 没有历史记录
                    tcpSocket[0]->write(QString("chatHistoryNotFound").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }
            }   // end of chatHistory


            // buffer = "chatSend##[time]##[senderId]##[receiverId]##[message]"

            else if("chatSend" == identifier)
            {
                QString timeString = QString(buffer).section("##", 1, 1);
                QDateTime time = QDateTime::fromString(timeString, "yyyy-MM-dd hh:mm:ss.zzz");
                int senderId = QString(buffer).section("##", 2, 2).toInt();
                int receiverId = QString(buffer).section("##", 3, 3).toInt();
                QString message = QString(buffer).section("##", 4, 4);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                QString sqlString = "";

                if (senderId < receiverId)
                {
                    sqlString = "INSERT INTO " +
                                QString::number(senderId) + "_chat_" + QString::number(receiverId) +
                                "VALUES (:time, :id, :message)";
                }

                else
                {
                    sqlString = "INSERT INTO " +
                                QString::number(receiverId) + "_chat_" + QString::number(senderId) +
                                "VALUES (:time, :id, :message)";
                }

                sqlQuery.prepare(sqlString);
                sqlQuery.bindValue(":time", time);
                sqlQuery.bindValue(":id", senderId);
                sqlQuery.bindValue(":message", message);
                sqlQuery.exec();

                /* user[receiverId]_friendList(senderId, senderName, unreadFriendMessage, unreadFriendFile)
                 * sender发送消息给receiver，receiver是未读sender的消息
                 * 因此在receiver的朋友列表中，找到sender
                 * 把receiver对sender的未读参数unreadFriendMessage设置为1
                 * 表示receiver未读sender
                 */

                sqlString = "UPDATE user" + QString::number(receiverId) + "_friendList "
                            "SET unreadFriendMessage = 1 WHERE id = :id";

                sqlQuery.clear();
                sqlQuery.prepare(sqlString);
                sqlQuery.bindValue(":id", senderId);
                sqlQuery.exec();
                db.close();
            }   // end of chatSend


            // buffer = "logout##[userId]"

            else if("logout" == identifier)
            {
                QString userId = QString(buffer).section("##", 1, 1);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                sqlQuery.prepare("UPDATE people SET islogin = 0 WHERE id = :id");
                sqlQuery.bindValue(":id", userId);
                sqlQuery.exec();

                // 更新服务器界面
                ui->listWidget->clear();

                sqlQuery.prepare("SELECT * FROM people WHERE islogin = 1");
                sqlQuery.exec();

                if (sqlQuery.next())
                {   // 查询得到上线用户
                    QString userId = sqlQuery.value(0).toString();
                    QString userName = sqlQuery.value(1).toString();
                    QString userIP = sqlQuery.value(3).toString();

                    QString str = QString("userId: %1, userName: %2, userIP: %3").arg(userId).arg(userName).arg(userIP);
                    ui->listWidget->addItem(str);

                    while (sqlQuery.next())
                    {
                        QString userId = sqlQuery.value(0).toString();
                        QString userName = sqlQuery.value(1).toString();
                        QString userIP = sqlQuery.value(3).toString();

                        QString str = QString("userId: %1, userName: %2, userIP: %3").arg(userId).arg(userName).arg(userIP);
                        ui->listWidget->addItem(str);
                    }
                }   // end of userIsLoginExists

                else
                {   // 查询不到在线用户
                    ui->listWidget->insertItem(0, "当前没有在线用户");
                }

                db.close();
            }   // end of logout


            // buffer = "getFriendList##[userId]"

            else if("getFriendList" == identifier)
            {
                QString userId = QString(buffer).section("##", 1, 1);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;

                /*
                 * user[userId]_friendList(id, name, unreadFriendMessage, unreadFriendFile)
                 * 用户的朋友的讯息通知列表，获取用户的每位朋友的讯息状态
                 */
                QString sqlString = "SELECT * FROM "
                                    "user" + userId + "_friendList "
                                    "DESC";

                sqlQuery.exec(sqlString);

                if (sqlQuery.next())
                {   // 找到该用户的朋友

                    QString friendName = sqlQuery.value(1).toString();
                    QString unreadFriendMessage = sqlQuery.value(2).toString();
                    QString unreadFriendFile = sqlQuery.value(3).toString();


                    // 创建QList容器
                    QList <QString> friendNameList;             // 存储user的朋友列表里的朋友名称
                    QList <QString> unreadFriendMessageList;    // 存储user对朋友的未读消息状态，1:他未读朋友，0:已读
                    QList <QString> unreadFriendFileList;       // 存储user对朋友的未读文件状态，1:他未读朋友，0:已读

                    int unreadFriendMessageCount = 0;   // 统计user未读朋友信息的数量，初始化
                    int unreadFriendFileCount = 0;      // 统计user未读朋友文件的数量，初始化

                    friendNameList.append(friendName);

                    if (unreadFriendMessage == '1')
                        unreadFriendMessageCount++;
                    unreadFriendMessageList.append(unreadFriendMessage);

                    if (unreadFriendFile == '1')
                        unreadFriendFileCount++;
                    unreadFriendFileList.append(unreadFriendFile);

                    while (sqlQuery.next())
                    {
                        QString friendName = sqlQuery.value(1).toString();
                        QString unreadFriendMessage = sqlQuery.value(2).toString();
                        QString unreadFriendFile = sqlQuery.value(3).toString();

                        friendNameList.append(friendName);

                        if (unreadFriendMessage == '1')
                            unreadFriendMessageCount++;
                        unreadFriendMessageList.append(unreadFriendMessage);

                        if (unreadFriendFile == '1')
                            unreadFriendFileCount++;
                        unreadFriendFileList.append(unreadFriendFile);
                    }

                    QString friends = "";
                    int onlineFriendCount = 0;          // 统计user的朋友在线的数量，初始化

                    for (int i = 0; i < friendNameList.length(); i++)
                    {   // 遍历user的朋友

                        sqlQuery.prepare("SELECT * FROM people WHERE name = :name");
                        sqlQuery.bindValue(":name", friendNameList.at(i));
                        sqlQuery.exec();
                        sqlQuery.next();

                        QString friendIP = sqlQuery.value(3).toString();

                        if (sqlQuery.value(4).toInt() == 1)
                        {   // islogin = 1
                            onlineFriendCount++;

                            friends = "##" + friendNameList.at(i) +
                                      "##1##" + friendIP +
                                      "##" + unreadFriendMessageList.at(i) +
                                      "##" + unreadFriendFileList.at(i) +
                                      friends;
                        }

                        else
                        {   // islogin = 0
                            friends = "##" + friendNameList.at(i) +
                                      "##0##" + friendIP +
                                      "##" + unreadFriendMessageList.at(i) +
                                      "##" + unreadFriendFileList.at(i) +
                                      friends;
                        }
                    }   // end of for-loop

                    friends = "canGetFriendList##" +
                              QString::number(friendNameList.length()) +
                              "##" + QString::number(onlineFriendCount) +
                              "##" + QString::number(unreadFriendMessageCount) +
                              "##" + QString::number(unreadFriendFileCount) +
                              friends;

                    tcpSocket[0]->write(friends.toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }   // end of friendListExists

                else
                {   // 用户的朋友列表里还未添加好友
                    tcpSocket[0]->write(QString("friendListNotFound").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }
            }   // end of getFriendList


            // buffer = "addFriend##[adderId]##[targetName]"

            else if("addFriend" == identifier)
            {
                int adderId = QString(buffer).section("##", 1, 1).toInt();
                QString targetName = QString(buffer).section("##", 2, 2);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                sqlQuery.prepare("SELECT * FROM people WHERE name = :name");
                sqlQuery.bindValue(":name", targetName);
                sqlQuery.exec();

                if (sqlQuery.next())
                { // 添加对象存在，可以添加作为好友

                    int targetId = sqlQuery.value(0).toInt();

                    sqlQuery.clear();

                    QString sqlString = "INSERT INTO "
                                        "user" + QString::number(adderId) + "_friendList "
                                        "VALUES (:id, :name, 0, 0)";

                    sqlQuery.prepare(sqlString);
                    sqlQuery.bindValue(":id", targetId);
                    sqlQuery.bindValue(":name", targetName);
                    sqlQuery.exec();

                    tcpSocket[0]->write(QString("addFriendSuccess").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }   // end of addTargetExists

                else
                {   // 查无此人，无法添加
                    tcpSocket[0]->write(QString("addFriendError").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }
            }   // end of addFriend


            // buffer = "deleteFriend##[deleterId]##[targetName]"

            else if("deleteFriend" == identifier)
            {
                int deleterId = QString(buffer).section("##", 1, 1).toInt();
                QString targetName = QString(buffer).section("##", 2, 2);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                QString sqlString = "DELETE FROM "
                                    "user" + QString::number(deleterId) + "_friendList "
                                    "WHERE name = :name";

                sqlQuery.prepare(sqlString);
                sqlQuery.bindValue(":name", targetName);

                if (sqlQuery.exec())
                { // 删除好友成功
                    tcpSocket[0]->write(QString("deleteFriendSuccess").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }

                else
                {   // 删除失败
                    tcpSocket[0]->write(QString("deleteFriendError").toUtf8());
                    tcpSocket[0]->flush();
                    db.close();
                }
            }   // end of deleteFriend


            // buffer = "sendFile##[senderId]##[targetName]"

            else if("sendFile" == identifier)
            {
                int senderId = QString(buffer).section("##", 1, 1).toInt();
                QString targetName = QString(buffer).section("##", 2, 2);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                sqlQuery.prepare("SELECT * FROM people WHERE name = :name");
                sqlQuery.bindValue(":name", targetName);
                sqlQuery.exec();
                sqlQuery.next();

                int targetId = sqlQuery.value(0).toInt();

                /*
                 * user[targetId]_friendList(senderId, senderName, unreadFriendMessage, unreadFriendFile)
                 * 如果sender发送文件成功，target将未读sender
                 * 把target对sender的未读参数unreadFriendFile设置为1
                 */

                QString sqlString = "UPDATE user" + QString::number(targetId) + "_friendList "
                                    "SET unreadFriendFile = 1 WHERE id = :id";

                sqlQuery.prepare(sqlString);
                sqlQuery.bindValue(":id", senderId);
                sqlQuery.exec();
                db.close();
            }   // end of sendFile


            // buffer = "sendFileSuccess##[senderId]##[targetName]"

            else if("sendFileSuccess" == identifier)
            {
                int senderId = QString(buffer).section("##", 1, 1).toInt();
                QString targetName = QString(buffer).section("##", 2, 2);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;

                /*
                 * user[senderId]_friendList(targetId, targetName, unreadFriendMessage, unreadFriendFile)
                 * 如果sender发送文件成功，sender将已读target
                 * 把sender对target的未读参数unreadFriendFile设置为0
                 */

                QString sqlString = "UPDATE user" + QString::number(senderId) + "_friendList "
                                    "SET unreadFriendFile = 0 WHERE name = :name";

                sqlQuery.prepare(sqlString);
                sqlQuery.bindValue(":name", targetName);
                sqlQuery.exec();
                db.close();
            }   // end of sendFileSuccess


            // buffer = "sendFileFailed##[senderId]##[targetName]"

            else if("sendFileFailed" == identifier)
            {
                int senderId = QString(buffer).section("##", 1, 1).toInt();
                QString targetName = QString(buffer).section("##", 2, 2);

                db.setDatabaseName("./people.db");
                db.open();

                QSqlQuery sqlQuery;
                sqlQuery.prepare("SELECT * FROM people WHERE name = :name");
                sqlQuery.bindValue(":name", targetName);
                sqlQuery.exec();
                sqlQuery.next();

                int targetId = sqlQuery.value(0).toInt();

                /*
                 * user[targetId]_friendList(senderId, senderName, unreadFriendMessage, unreadFriendFile)
                 * 如果sender发送文件失败，target将收不到
                 * target对sender的未读文件参数状态unreadFriendFile设置为0
                 */

                QString sqlString = "UPDATE user" + QString::number(targetId) + "_friendList "
                                    "SET unreadFriendFile = 0 WHERE id = :id";

                sqlQuery.prepare(sqlString);
                sqlQuery.bindValue(":id", senderId);
                sqlQuery.exec();
                db.close();
            }   // end of sendFileFailed
        }); // end of readyRead
    }); // end of newConnection
}

server::~server()
{
    //关闭服务器连接，新的客户端连接不再被接受
    tcpServer->close();

    /*
     * 在当前事件循环结束后，异步(不会同步)删除对象，以释放其占用的资源
     * 这是一种安全的方式来删除对象，
     * 因为在删除之前会确保对象的所有相关操作都已经完成
    */
    tcpServer->deleteLater();

    delete ui;
}
