#ifndef SENDFILE_H
#define SENDFILE_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QFile>
#include <QCloseEvent>

namespace Ui {
class sendFile;
}

class sendFile : public QWidget
{
    Q_OBJECT

public:
    explicit sendFile(QWidget *parent = nullptr);
    ~sendFile();
    void sendData();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_selectFilePushButton_clicked();

    void on_sendfilePushButton_clicked();

private:
    Ui::sendFile *ui;
    QTcpServer *tcpServer;
    QTcpSocket *tcpSocket;
    QFile file;
    QTimer *timer;
    QString fileName;
    quint64 fileSize;
    quint64 sendSize;
};

#endif // SENDFILE_H
