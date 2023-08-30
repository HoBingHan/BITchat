#ifndef HOME_H
#define HOME_H

#include <QWidget>
#include <QTimer>
#include <QTcpSocket>
#include <QCloseEvent>

namespace Ui {
class home;
}

class home : public QWidget
{
    Q_OBJECT

public:
    explicit home(QWidget *parent = nullptr);
    ~home();
    QTimer *timer;
    void updateFriendList();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_sendMessageToolButton_clicked();

    void on_logoutToolButton_clicked();

    void on_addFriendToolButton_clicked();

    void on_deleteFriendToolButton_clicked();

    void on_startChatToolButton_clicked();

    void on_refreshToolButton_clicked();

private:
    Ui::home *ui;
    QTcpSocket *tcpSocket;
};

#endif // HOME_H
