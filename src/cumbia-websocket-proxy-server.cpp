#include "cumbia-websocket-proxy-server.h"
#include "cuwsproxyreader.h"
#include "cuwssourcevalidator.h"

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
#include <qustring.h>
#include <QtDebug>

#include <QTcpSocket>
#include <QTcpServer>
#include <QNetworkReply>
#include <QWebSocket>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

CuWsProxyServer::CuWsProxyServer(CumbiaPool *cumbia_pool, QObject *parent,
                                 const QString &ws_address, quint16 ws_port, quint16 http_port) :
    QObject(parent)
{
    m_webso_url = QUrl(ws_address);
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
    m_tcps->listen(QHostAddress::Any, http_port);


    connect(m_tcps, &QTcpServer::newConnection, this, &CuWsProxyServer::onNewConnection);
    connect(m_tcps, SIGNAL(acceptError(QAbstractSocket::SocketError)), this,
            SLOT(onAcceptError(QAbstractSocket::SocketError)));

    // socket server
    m_ws_server = new QWebSocketServer("cumbia-websocket-proxy-server", QWebSocketServer::NonSecureMode, this);
    connect(m_ws_server, SIGNAL(newConnection()), this, SLOT(onWsServerNewConnection()));
    connect(m_ws_server, SIGNAL(closed()), this, SLOT(onWsServerClosed()));
    connect(m_ws_server, SIGNAL(serverError(QWebSocketProtocol::CloseCode)), this, SLOT(onWsServerError(QWebSocketProtocol::CloseCode)));
    m_ws_server->listen(QHostAddress(ws_address), ws_port);

    qDebug() << __PRETTY_FUNCTION__ << "listening on " << m_tcps->serverAddress().toString() << m_tcps->serverPort() << m_ws_server->serverUrl();
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
    QByteArray reply("HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 12\r\n\r\nHello World!");
    int b_w = so->write(reply);
    qDebug() << __PRETTY_FUNCTION__ << "got " << ba << "From soxkezz " << so << "and rewritten theb back byste " << b_w;

    // "GET /tango://hokuto:20000/test/device/1/double_scalar/label HTTP/1.1\r\nHost: localhost:12702\r\nConnection: Keep-Alive\r\n ...
    QList<QByteArray> parts = ba.split(' ');
    QString src;
    if(parts.size() > 1) {
        src = QString(parts[1]);
        if(src.startsWith('/'))
            src.remove(0, 1);
        CuWsSourceValidator validator;
        if(validator.isValid(src)) {
            CuWsProxyReader *r = findChild<CuWsProxyReader *>(src);
            if(!r) {
                r = new CuWsProxyReader(this, cu_pool, m_ctrl_factory_pool);
                r->setObjectName(src);
                connect(r, SIGNAL(newData(CuData)), this, SLOT(onNewData(CuData)));
                r->setSource(src);
            }
        }
    }
    else
        printf("i - discarding source %s [CuWsProxyServer::onReadyRead]\n", qstoc(src));
}

void CuWsProxyServer::onNewData(const CuData &data) {
    printf("\e[1;32mCuWsProxyServer.onUpdate: %s\e[0m\n", data.toString().c_str());
    QuString src(data, "src");
    foreach(QWebSocket* so, m_so_map.values(src)) {
        qDebug() << __PRETTY_FUNCTION__ << "DUCKING YEA TO " << so << src;
        so->sendTextMessage("DUCK YEA");
    }
    //    QWebSocket *so = new QWebSocket();
    //    so->open(m_webso_url);
    //    qint64 bw = so->sendTextMessage("DADUCK");
    //    qDebug() << __PRETTY_FUNCTION__ << "Sen" << bw << "byste";
}

void CuWsProxyServer::onWebSoErr(QAbstractSocket::SocketError e)
{
    QWebSocket *so = qobject_cast<QWebSocket *>(sender());
    perr("CuWsProxyServer.onWebSoErr: socket error %s peer name %s port %d address %s", qstoc(so->errorString()),
         qstoc(so->peerName()), so->localPort(), qstoc(so->localAddress().toString()));
}

void CuWsProxyServer::onWsServerNewConnection() {
    QWebSocket *pSocket = m_ws_server->nextPendingConnection();
    connect(pSocket, &QWebSocket::textMessageReceived, this, &CuWsProxyServer::processTextMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &CuWsProxyServer::onWebsoDisconnected);
    qDebug() << __PRETTY_FUNCTION__ << "+++++++++++++++++ new client connected from " << pSocket->peerName() << pSocket->peerAddress() << pSocket->peerPort();

}

void CuWsProxyServer::onWsServerError(QWebSocketProtocol::CloseCode closeCode) {
    qDebug() << __PRETTY_FUNCTION__ << closeCode;
}

void CuWsProxyServer::onWsServerClosed() {
    qDebug() << __PRETTY_FUNCTION__;
}

void CuWsProxyServer::processTextMessage(const QString &msg) {
    qDebug() << __PRETTY_FUNCTION__ << "received this" << msg;
    QRegularExpression re("SUBSCRIBE\\s+(.*)");
    QRegularExpressionMatch ma = re.match(msg);
    if(ma.hasMatch() && ma.capturedTexts().size() == 2) {
        QString src = ma.capturedTexts()[1];
        CuWsSourceValidator validator;
        if(validator.isValid(src)) {
            CuWsProxyReader *r = findChild<CuWsProxyReader *>(src);
            if(!r) {
                r = new CuWsProxyReader(this, cu_pool, m_ctrl_factory_pool);
                r->setObjectName(src);
                connect(r, SIGNAL(newData(CuData)), this, SLOT(onNewData(CuData)));
                r->setSource(src);
            }
            QWebSocket *so = qobject_cast<QWebSocket *>(sender());
            m_so_map.insert(src, so);
        }
    }
}

void CuWsProxyServer::onWebsoDisconnected() {
    qDebug() << __PRETTY_FUNCTION__;

}
