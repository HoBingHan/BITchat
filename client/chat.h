#ifndef CHAT_H
#define CHAT_H

#include <QWidget>
#include <QTcpSocket>
#include <QTimer>
#include <QCloseEvent>

namespace Ui {
class chat;
}

class chat : public QWidget
{
    Q_OBJECT

public:
    explicit chat(QWidget *parent = nullptr);
    ~chat();
    QTimer *timer;
    void getChatHistory();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_sendToolButton_clicked();

    void on_closeToolButton_clicked();

    void on_sendFileToolButton_clicked();

    void on_receiveFileToolButton_clicked();

private:
    Ui::chat *ui;
    QTcpSocket *tcpSocket;
};

#endif // CHAT_H
