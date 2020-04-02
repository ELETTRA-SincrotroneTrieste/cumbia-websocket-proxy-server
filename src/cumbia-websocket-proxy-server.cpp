#include "cumbia-websocket-proxy-server.h"
#include "cuwsproxyreader.h"
#include "cuwssourcevalidator.h"
#include "cuwsdatatojson.h"

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
                                 const QString &ws_address, quint16 ws_port) :
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
    m_ws_server = new QWebSocketServer("cumbia-websocket-proxy-server", QWebSocketServer::NonSecureMode, this);
    connect(m_ws_server, SIGNAL(newConnection()), this, SLOT(onCliConnected()));
    connect(m_ws_server, SIGNAL(closed()), this, SLOT(onWsServerClosed()));
    connect(m_ws_server, SIGNAL(serverError(QWebSocketProtocol::CloseCode)), this, SLOT(onWsServerError(QWebSocketProtocol::CloseCode)));
    m_ws_server->listen(QHostAddress(ws_address), ws_port);
    printf("websocket server listening on %s %d URL \e[1;32m%s\e[0m [CuWsProxyServer.CuWsProxyServer]\n",
           qstoc(m_ws_server->serverAddress().toString()), m_ws_server->serverPort(), qstoc(m_ws_server->serverUrl().toString()));
}

CuWsProxyServer::~CuWsProxyServer() {
    if(m_ws_server->isListening())
        m_ws_server->close();
    delete m_ws_server;
}


void CuWsProxyServer::onAcceptError(QAbstractSocket::SocketError e) {
    perr("CuWsProxyServer.onAcceptError: %d", e);
}

void CuWsProxyServer::onNewData(const CuData &data) {
    QuString src(data, "src");
    foreach(QWebSocket* so, m_so_map.values(src)) {
        CuWsDataToJson d2j;
        so->sendTextMessage(d2j.toJson(data));
    }
}

void CuWsProxyServer::onWebSoErr(QAbstractSocket::SocketError e)
{
    QWebSocket *so = qobject_cast<QWebSocket *>(sender());
    perr("socket error %s peer name %s port %d address %s [CuWsProxyServer.onWebSoErr]", qstoc(so->errorString()),
         qstoc(so->peerName()), so->localPort(), qstoc(so->localAddress().toString()));
}

void CuWsProxyServer::onWsServerError(QWebSocketProtocol::CloseCode closeCode) {
    QWebSocket *so = qobject_cast<QWebSocket *>(sender());
    perr("websocket server error %s peer name %s port %d address %s code %d [CuWsProxyServer.onWsServerError]", qstoc(so->errorString()),
         qstoc(so->peerName()), so->localPort(), qstoc(so->localAddress().toString()), closeCode);
}

void CuWsProxyServer::onWsServerClosed() {
    QWebSocket *so = qobject_cast<QWebSocket *>(sender());
    printf("websocket server closed %s peer name %s port %d address %s [CuWsProxyServer.onWsServerClosed]\n", qstoc(so->errorString()),
           qstoc(so->peerName()), so->localPort(), qstoc(so->localAddress().toString()));
}

void CuWsProxyServer::processTextMessage(const QString &msg) {
    qDebug() << __PRETTY_FUNCTION__ << msg;
    QWebSocket *so = qobject_cast<QWebSocket *>(sender());
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
    else {
        re.setPattern("UNSUBSCRIBE\\s+(.*)");
        ma = re.match(msg);
        if(ma.hasMatch() && ma.capturedTexts().size() == 2) {
            QString src = ma.capturedTexts()[1];
            m_so_map.remove(src, so);
            CuWsProxyReader *r = findChild<CuWsProxyReader *>(src);
            if(r && !m_so_map.contains(src)) {
                delete r;
            }
            else if(!r) {
                perr("reader \"%s\" not found [CuWsProxyServer::processTextMessage]", qstoc(src));
            }
        }
    }
}

void CuWsProxyServer::onCliConnected() {
    QWebSocket *so = m_ws_server->nextPendingConnection();
    connect(so, &QWebSocket::textMessageReceived, this, &CuWsProxyServer::processTextMessage);
    connect(so, &QWebSocket::disconnected, this, &CuWsProxyServer::onCliDisconnected);
    printf("\e[0;32m+\e[0m client %s %s:%d connected [CuWsProxyServer.onCliDisconnected]\n",
           qstoc(so->peerName()), qstoc(so->peerAddress().toString()), so->peerPort());
}

void CuWsProxyServer::onCliDisconnected() {
    QWebSocket *so = qobject_cast<QWebSocket *>(sender());
    printf("\e[0;35m-\e[0m client %s %s:%d disconnected [CuWsProxyServer.onCliDisconnected]\n",
           qstoc(so->peerName()), qstoc(so->peerAddress().toString()), so->peerPort());

    // find all sources associated with the socket
    QStringList delsrcs;
    foreach(QString key, m_so_map.keys()) {
        // socket is associated to src and src is linked to only one socket: delete
        if(m_so_map.values(key).contains(so) && m_so_map.values(key).size() == 1)
            delsrcs << key;
    }
    foreach(QString key, delsrcs) {
        CuWsProxyReader *r = findChild<CuWsProxyReader *>(key);
        if(r) {
            printf("stopping reader \"%s\" which is still active [CuWsProxyServer.onCliDisconnected]\n", qstoc(key));
            delete r;
        }
    }
    QMutableMapIterator<QString, QWebSocket *> mi(m_so_map);
    while (mi.hasNext()) {
        mi.next();
        if(mi.value() == so)
            mi.remove();
    }
}
