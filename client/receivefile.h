#ifndef RECEIVEFILE_H
#define RECEIVEFILE_H

#include <QWidget>
#include <QTcpSocket>
#include <QFile>
#include <QCloseEvent>

namespace Ui {
class receiveFile;
}

class receiveFile : public QWidget
{
    Q_OBJECT

public:
    explicit receiveFile(QWidget *parent = nullptr);
    ~receiveFile();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::receiveFile *ui;
    QTcpSocket *tcpSocket;
    QFile file;
    QString fileName;
    quint64 fileSize;
    quint64 receiveSize;
    bool isStart;
    QString ip;
    quint16 port;
};

#endif // RECEIVEFILE_H
