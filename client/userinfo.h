#ifndef USERINFO_H
#define USERINFO_H

#include <QString>

class userInfo
{
public:
    int id;
    QString name;
    bool islogin;
    QString ip;
    int port;

    userInfo();
};

#endif // USERINFO_H
