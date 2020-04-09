#ifndef LINKPOOL_H
#define LINKPOOL_H

#include <QString>
#include <QMap>
#include <QList>
#include <cuwsproxy.h>

class QWebSocket;

class LinkInfo {
public:
    LinkInfo(CuWsProxy *proxy, const QString &src, const QString& atype);
    LinkInfo();

    bool isEmpty() const;
    int socketCount() const;
    int addSocket(QWebSocket *so);
    int removeSocket(QWebSocket *so);
    bool hasSocket(QWebSocket *so) const;
    QList<QWebSocket *> sockets() const;

    CuWsProxy *proxy;
    QString name;
    QString src;
    QString atype;


private:
    QList<QWebSocket *>m_sockets;

};

class LinkPool
{
public:
    LinkPool();

    LinkInfo& find(const QString& name);
    CuWsProxy* findProxy(const QString& name);

    void add(const QString &src, const QString &atype, CuWsProxy *proxy, QWebSocket *so);

    LinkInfo &removeSocket(const QString &src, const QString &atype, QWebSocket *so);
    QList<LinkInfo> removeBySocket(QWebSocket *so);
    LinkInfo remove(const QString& name, QWebSocket *so);

    void removeAllSocketless();

private:
    QMap<QString, LinkInfo > m_linkmap;
    LinkInfo m_empty_linkinfo;
};

#endif // LINKPOOL_H
