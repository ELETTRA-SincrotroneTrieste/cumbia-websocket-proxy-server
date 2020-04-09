#include "cuwsproxy.h"
#include <cucontext.h>
#include <cucontrolsreader_abs.h>
#include <cucontrolswriter_abs.h>

CuWsProxy::CuWsProxy(QObject *parent) : QObject(parent) {
    d_p = new CuWsProxyPrivate;
    d_p->auto_configure = true;
}

CuWsProxy::~CuWsProxy() {
    delete d_p;
}

QString CuWsProxy::src() const {
    if(d_p->context->getReader())
        return d_p->context->getReader()->source();
    if(d_p->context->getWriter())
        return d_p->context->getWriter()->target();
    return "";
}

/*!
 * \brief Returns the CuData holding the "property" type
 * \return the configuration obtained from the "property" type data.
 */
const CuData &CuWsProxy::getConfig() const {
    return d_p->config;
}

CuContext *CuWsProxy::getContext() const {
    return d_p->context;
}

void CuWsProxy::dispose() {
    if(d_p->context->getReader())
        d_p->context->disposeReader();
    if(d_p->context->getWriter())
        d_p->context->disposeWriter();
    // readers and writers in context will now have size 0
    deleteLater();
}

bool CuWsProxy::isRunning() const
{
    return d_p->context && ( d_p->context->readers().size() > 0  || d_p->context->writers().size() > 0);
}
