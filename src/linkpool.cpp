#include "cuwsdatatojson.h"
#include "cuwsproxyconfig.h"
#include "cuwsproxyreader.h"
#include "cuwsproxywriter.h"
#include "cuwssourcevalidator.h"
#include "linkpool.h"
#include <QRegularExpression>
#include <QWebSocket>
#include <QMutableMapIterator>
#include <QtDebug>

LinkInfo::LinkInfo() {
    proxy = nullptr;
}

LinkInfo::LinkInfo(CuWsProxy* _proxy, const QString &_src, const QString &_atype)
{
    src = _src;
    atype = _atype;
    proxy = _proxy;
    name = QString("%1/%2").arg(atype).arg(src);
}

bool LinkInfo::isEmpty() const {
    return name.isEmpty();
}

int LinkInfo::socketCount() const {
    return m_sockets.size();
}

int LinkInfo::addSocket(QWebSocket *so) {
    m_sockets.append(so);
    return m_sockets.size();
}

int LinkInfo::removeSocket(QWebSocket *so) {
    m_sockets.removeAll(so);
    return m_sockets.size();
}

bool LinkInfo::hasSocket(QWebSocket *so) const {
    return m_sockets.contains(so);
}

QList<QWebSocket *> LinkInfo::sockets() const {
    return m_sockets;
}

LinkPool::LinkPool() {

}

LinkInfo &LinkPool::find(const QString &name) {
    if(m_linkmap.contains(name))
        return m_linkmap[name];
    return m_empty_linkinfo;
}

CuWsProxy *LinkPool::findProxy(const QString &name) {
    return find(name).proxy;
}

/*!
 * \brief Remove from the pool all links that after removal of *so* have no more sockets
 * \param so the socket to look for
 * \return A copy of the LinkInfo removed from the pool, that can be used to delete the proxy
 */
QList<LinkInfo> LinkPool::removeBySocket(QWebSocket *so)  {
    QList<LinkInfo> removed;
    QMutableMapIterator <QString, LinkInfo > i(m_linkmap);
    while(i.hasNext()) {
        i.next();
        LinkInfo &li = i.value();
        if(li.removeSocket(so) == 0) {
            removed.append(li);
            i.remove();
        }
    }
    return removed;
}

LinkInfo LinkPool::remove(const QString &name, QWebSocket *so)
{
    LinkInfo ret;
    QMutableMapIterator<QString, LinkInfo> it(m_linkmap);
    while(it.hasNext()) {
        it.next();
        if(it.key() == name && it.value().removeSocket(so) == 0) {
            ret = it.value();
            it.remove();
        }
    }
    return ret;
}

void LinkPool::removeAllSocketless() {
    QMutableMapIterator<QString, LinkInfo> it(m_linkmap);
    while(it.hasNext()) {
        it.next();
        if(it.value().sockets().isEmpty())
            it.remove();
    }
}

void LinkPool::add(const QString &src, const QString& atype, CuWsProxy* proxy, QWebSocket *so) {
    QString name = atype + "/" + src;
    LinkInfo &li = find(name);
    if(!li.isEmpty())
        li.addSocket(so);
    else {
        LinkInfo linfo(proxy, src, atype);
        linfo.addSocket(so);
        m_linkmap.insert(name, linfo);
    }
}

/*!
 * \brief remove the socket from the list associated to src and type
 * \param src the source name
 * \param atype the action type: read, write, conf
 * \param so the socket to remove
 * \return  a reference to the LinkInfo associated to atype and src, through which the caller
 *          can close the referenced socket and delete the proxy if no sockets are in use.
 */
LinkInfo& LinkPool::removeSocket(const QString &src, const QString& atype, QWebSocket *so) {
    int so_cnt = 0;
    QString name = atype + "/" + src;
    LinkInfo &li = find(name);
    if(!li.isEmpty()) {
        so_cnt = li.removeSocket(so);
        return li;
    }
    return m_empty_linkinfo;
}
