#include "cumbia-websocket-proxy-server.h"

#include <cumbiapool.h>
#include <cumbiaepics.h>
#include <cumbiatango.h>
#include <cuepcontrolsreader.h>
#include <cuepcontrolswriter.h>
#include <cutcontrolsreader.h>
#include <cutcontrolswriter.h>
#include <cutango-world.h>
#include <cuepics-world.h>
#include <cuthreadfactoryimpl.h>
#include <cuserviceprovider.h>
#include <qthreadseventbridgefactory.h>
#include <cumacros.h>
#include <QtDebug>

#include <QTcpSocket>
#include <QTcpServer>
#include <QNetworkReply>

CuWsProxyServer::CuWsProxyServer(CumbiaPool *cumbia_pool, QObject *parent, const QString& address, quint16 port) :
    QObject(parent)
{
    cu_pool = cumbia_pool;
    // setup Cumbia pool and register cumbia implementations for tango and epics
    CumbiaEpics* cuep = new CumbiaEpics(new CuThreadFactoryImpl(), new QThreadsEventBridgeFactory());
    CumbiaTango* cuta = new CumbiaTango(new CuThreadFactoryImpl(), new QThreadsEventBridgeFactory());
    cu_pool->registerCumbiaImpl("tango", cuta);
    cu_pool->registerCumbiaImpl("epics", cuep);
    m_ctrl_factory_pool.registerImpl("tango", CuTReaderFactory());
    m_ctrl_factory_pool.registerImpl("tango", CuTWriterFactory());
    m_ctrl_factory_pool.registerImpl("epics", CuEpReaderFactory());
    m_ctrl_factory_pool.registerImpl("epics", CuEpWriterFactory());

    CuTangoWorld tw;
    m_ctrl_factory_pool.setSrcPatterns("tango", tw.srcPatterns());
    cu_pool->setSrcPatterns("tango", tw.srcPatterns());
    CuEpicsWorld ew;
    m_ctrl_factory_pool.setSrcPatterns("epics", ew.srcPatterns());
    cu_pool->setSrcPatterns("epics", ew.srcPatterns());

    // log
    cuta->getServiceProvider()->registerService(CuServices::Log, new CuLog(&m_log_impl));
    cuep->getServiceProvider()->registerService(CuServices::Log, new CuLog(&m_log_impl));

    // socket server
    m_tcps = new QTcpServer(this);
    address.isEmpty() ? m_tcps->listen(QHostAddress::Any, port) : m_tcps->listen(QHostAddress(address), port);

    connect(m_tcps, &QTcpServer::newConnection, this, &CuWsProxyServer::onNewConnection);
    connect(m_tcps, SIGNAL(acceptError(QAbstractSocket::SocketError)), this,
            SLOT(onAcceptError(QAbstractSocket::SocketError)));

    qDebug() << __PRETTY_FUNCTION__ << "listening on " << m_tcps->serverAddress().toString() << m_tcps->serverPort();
}

CuWsProxyServer::~CuWsProxyServer()
{
}

void CuWsProxyServer::onNewConnection() {

    qDebug() << __PRETTY_FUNCTION__ << ">> IN has pending" << m_tcps->hasPendingConnections();
    QTcpSocket *clientConnection = m_tcps->nextPendingConnection();
    connect(clientConnection, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(clientConnection, &QAbstractSocket::disconnected, this, &CuWsProxyServer::clientDisconnected);
}

void CuWsProxyServer::clientDisconnected()
{
    qDebug() << __PRETTY_FUNCTION__ << qobject_cast<QTcpSocket *>(sender()) << "client disconnected";
}

void CuWsProxyServer::onAcceptError(QAbstractSocket::SocketError e)
{
    perr("CuWsProxyServer.onAcceptError: %d", e);
}

void CuWsProxyServer::onReadyRead()
{
    QTcpSocket *so = qobject_cast<QTcpSocket *>(sender());
    QByteArray ba;
    while(so->bytesAvailable()) {
        ba += so->readAll();
    }
    QByteArray reply("HTTP/1.1 200 OK\r\n"
                                          "Content-type: text/plain\r\n"
                                          "Content-length: 12\r\n"
                                          "\r\n"
                                          "Hello World!");
    int b_w = so->write(reply);
    qDebug() << __PRETTY_FUNCTION__ << "got " << ba << "From soxkezz " << so << "and rewritten theb back byste " << b_w;

//    so->close();
}
