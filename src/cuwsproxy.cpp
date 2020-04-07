#include "cuwsproxy.h"
#include <cucontext.h>


CuWsProxy::CuWsProxy(QObject *parent) : QObject(parent) {
    d_p = new CuWsProxyPrivate;
    d_p->auto_configure = true;
}

CuWsProxy::~CuWsProxy() {
    delete d_p;
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
