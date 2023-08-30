#ifndef SIGNUP_H
#define SIGNUP_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>

namespace Ui {
class signUp;
}

class signUp : public QWidget
{
    Q_OBJECT

public:
    explicit signUp(QWidget *parent = nullptr);
    ~signUp();

private slots:
    void on_commitPushButton_clicked();

    void on_returnPushButton_clicked();

private:
    Ui::signUp *ui;
    QTcpSocket *tcpSocket;
};

#endif // REGISTER_H
