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

class QTcpServer;
class CuData;
class CumbiaPool;


class CuWsProxyServer : public QObject
{
    Q_OBJECT

public:
    explicit CuWsProxyServer(CumbiaPool *cu_p, QObject *parent, const QString &ws_address, quint16 ws_port = 12702, quint16 http_port = 12703);
    ~CuWsProxyServer();

protected slots:
    void onNewConnection();
    void clientDisconnected();
    void onAcceptError(QAbstractSocket::SocketError e);
    void onReadyRead();
    void onNewData(const CuData &data);
    void onWebSoErr(QAbstractSocket::SocketError e);

    //
    void onWsServerNewConnection();
    void onWsServerError(QWebSocketProtocol::CloseCode closeCode);
    void onWsServerClosed();
    void processTextMessage(const QString &msg);
    void onWebsoDisconnected();

private:

    CumbiaPool *cu_pool;
    QuLogImpl m_log_impl;
    CuControlsFactoryPool m_ctrl_factory_pool;
    QTcpServer *m_tcps;
    QWebSocketServer *m_ws_server;
    QUrl m_webso_url;
    QMultiMap <QString, QWebSocket *> m_so_map;
};


#endif // CuWsProxyServer_H
