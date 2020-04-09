#ifndef CuWsProxyServer_H
#define CuWsProxyServer_H

#include <QObject>
#include <qulogimpl.h>
#include <cucontrolsfactorypool.h>
#include <cumbiatango.h>
#include <cumbiaepics.h>
#include <cudatalistener.h>
#include <QAbstractSocket>
#include <QMultiMap>
#include <QWebSocket>
#include <QWebSocketServer>

#include "linkpool.h"

class QTcpServer;
class CuData;
class CumbiaPool;


class CuWsProxyServer : public QObject
{
    Q_OBJECT

public:
    explicit CuWsProxyServer(CumbiaPool *cu_p, QObject *parent, const QString &ws_address, quint16 ws_port = 12702);
    ~CuWsProxyServer();

protected slots:
    void onAcceptError(QAbstractSocket::SocketError e);
    void onNewData(const CuData &data, const char* atype);
    void onWebSoErr(QAbstractSocket::SocketError e);

    //
    void onCliConnected();
    void onWsServerError(QWebSocketProtocol::CloseCode closeCode);
    void onWsServerClosed();
    void processTextMessage(const QString &msg);
    void onCliDisconnected();

private:

    CumbiaPool *cu_pool;
    QuLogImpl m_log_impl;
    CuControlsFactoryPool m_ctrl_factory_pool;
    QWebSocketServer *m_ws_server;
    QUrl m_webso_url;
//    QMultiMap <QString, QWebSocket *> m_so_map;

    LinkPool m_linkpoo;
};


#endif // CuWsProxyServer_H
