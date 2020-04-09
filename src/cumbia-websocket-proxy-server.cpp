#include "cumbia-websocket-proxy-server.h"
#include "cuwsproxyreader.h"
#include "cuwsproxywriter.h"
#include "cuwsproxyconfig.h"
#include "cuwssourcevalidator.h"
#include "cuwsdatatojson.h"
#include "linkpool.h"

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

void CuWsProxyServer::onNewData(const CuData &data, const char *atype) {
    QuString src(data, "src");
    QString name = QString("%1/%2").arg(atype).arg(src);
    LinkInfo& li=m_linkpoo.find(name);
    if(li.isEmpty())
        perr("no link with name \%s\" was found [CuWsProxyServer::onNewData]", qstoc(name));
    else {
        // one shot actions are removed immediately after their first data update
        // and the proxy is deleted
        QStringList oneshot_actions = QStringList() << "conf" << "writer";
        foreach(QWebSocket *so, li.sockets()) {
            CuWsDataToJson d2j;
            so->sendTextMessage(d2j.toJson(data, atype));
            if(oneshot_actions.contains(atype)) {
                LinkInfo remlink = m_linkpoo.remove(name, so);
                if(!remlink.isEmpty())
                    remlink.proxy->deleteLater();
            }
        }
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
    QWebSocket *so = qobject_cast<QWebSocket *>(sender());
    QRegularExpression re("^(SUBSCRIBE|CONF|WRITE)\\s+(.*)");
    QRegularExpressionMatch ma = re.match(msg);
    if(ma.hasMatch() && ma.capturedTexts().size() == 3) {
        QString src = ma.capturedTexts()[2], mode = ma.capturedTexts()[1].toLower(), name = mode + "/" + src;
        CuWsSourceValidator validator;
        if(validator.isValid(src)) {
            LinkInfo &li = m_linkpoo.find(name);
            CuWsProxy * proxy = li.proxy;
            if(!proxy) {
                if(mode == "subscribe") {
                    proxy = new CuWsProxyReader(this, cu_pool, m_ctrl_factory_pool);
                    qobject_cast<CuWsProxyReader *>(proxy)->setSource(src);
                    m_linkpoo.add(src, mode, proxy, so);
                }
                else if(mode == "conf") {
                    proxy = new CuWsProxyConfig(this, cu_pool, m_ctrl_factory_pool);
                    qobject_cast<CuWsProxyConfig *>(proxy)->setSource(src);
                    m_linkpoo.add(src, mode, proxy, so);
                }
                else {
                    QString target; CuVariant args;
                    proxy = new CuWsProxyWriter(this, cu_pool, m_ctrl_factory_pool);
                    qobject_cast<CuWsProxyWriter *>(proxy)->parseRawTarget(src, target, args);
                    qobject_cast<CuWsProxyWriter *>(proxy)->setTarget(target);
                    qobject_cast<CuWsProxyWriter *>(proxy)->execute(args);
                    printf("\e[1;31mCuWsProxyServer::processTextMessage targettt %s\e[0m\n", qstoc(target));
                    m_linkpoo.add(target, mode, proxy, so);
                }
                proxy->setObjectName(name);
                connect(proxy, SIGNAL(newData(CuData, const char*)), this, SLOT(onNewData(CuData, const char*)));
            }
            else {
                const CuData& config = proxy->getConfig();
                if(!config.isEmpty()) {
                    CuWsDataToJson d2j;
                    printf("\e[1;32mCuWsProxyServer.processTextMessage: sending cached configuration \e[1;33m%s\e[0m\n", config.toString().c_str());
                    so->sendTextMessage(d2j.toJson(config, mode.toLatin1().data()));
                }
                if(mode == "write") {
                    QString target; CuVariant args;
                    qobject_cast<CuWsProxyWriter *>(proxy)->parseRawTarget(src, target, args);
                    m_linkpoo.add(target, mode, proxy, so);
                }
                else
                    m_linkpoo.add(src, mode, proxy, so);
            }
        }
    }
    else {
        re.setPattern("^(UNSUBSCRIBE|DELCONF|DELWRITE)\\s+(.*)");
        ma = re.match(msg);
        if(ma.hasMatch() && ma.capturedTexts().size() == 3) {
            QString src = ma.capturedTexts()[2];
            QString action = ma.capturedTexts()[1].toLower();
            action.remove("un").remove("del");
            LinkInfo li = m_linkpoo.remove(action + "/" + src, so);
            if(!li.isEmpty())
                li.proxy->deleteLater();
        }
        else
            perr("error: invalid command \"%s\" [CuWsProxyServer.processTextMessage]", qstoc(msg));
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

    foreach(LinkInfo li, m_linkpoo.removeBySocket(so)) {
        // no more sockets associated to proxy, can delete proxy
        li.proxy->deleteLater();
    }
    so->close();
    so->deleteLater();
}

