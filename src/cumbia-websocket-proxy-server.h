#ifndef CuWsProxyServer_H
#define CuWsProxyServer_H

#include <QObject>
#include <qulogimpl.h>
#include <cucontrolsfactorypool.h>
#include <cumbiatango.h>
#include <cumbiaepics.h>
#include <QAbstractSocket>


class QTcpServer;
class CuData;
class CumbiaPool;

class CuWsProxyServer : public QObject
{
    Q_OBJECT

public:
    explicit CuWsProxyServer(CumbiaPool *cu_p, QObject *parent, const QString &address, quint16 port = 12702);
    ~CuWsProxyServer();

protected slots:
    void onNewConnection();
    void clientDisconnected();
    void onAcceptError(QAbstractSocket::SocketError e);
    void onReadyRead();

private:

    CumbiaPool *cu_pool;
    QuLogImpl m_log_impl;
    CuControlsFactoryPool m_ctrl_factory_pool;
    QTcpServer *m_tcps;
};

#endif // CuWsProxyServer_H
