#ifndef SERVER_H
#define SERVER_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QDateTime>
#include <QSqlQuery>

const int N = 30;   // 设置30个通信口

QT_BEGIN_NAMESPACE
namespace Ui { class server; }
QT_END_NAMESPACE

class server : public QWidget
{
    Q_OBJECT

public:
    server(QWidget *parent = nullptr);
    ~server();

private:
    Ui::server *ui;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket[N];
    QSqlDatabase db;
};
#endif // SERVER_H
