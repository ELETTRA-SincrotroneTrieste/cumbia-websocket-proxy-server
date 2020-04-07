#include "cuwsproxyconfig.h"
#include <cucontrolsreader_abs.h>
#include <cumbiapool.h>
#include <QContextMenuEvent>
#include <QMetaProperty>
#include <QRegularExpression>
#include <QtDebug>

#include "cucontrolsfactories_i.h"
#include "cucontrolsfactorypool.h"
#include "culinkstats.h"
#include "cucontext.h"

/** @private */
class CuWsProxyConfigPrivate
{
public:
};

/** \brief Constructor with the parent widget, *CumbiaPool*  and *CuControlsFactoryPool*
 *
 *   Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
CuWsProxyConfig::CuWsProxyConfig(QObject *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool) :
    CuWsProxy(w), CuDataListener()
{
    m_init();
    d_p->context = new CuContext(cumbia_pool, fpool);
}

void CuWsProxyConfig::m_init()
{
    m = new CuWsProxyConfigPrivate;
}

CuWsProxyConfig::~CuWsProxyConfig()
{
    pdelete("~CuWsProxyConfig %p", this);
    delete m;
}

QString CuWsProxyConfig::source() const
{
    if(CuControlsReaderA* r = d_p->context->getReader())
        return d_p->src_proto_prefix + r->source();
    return "";
}

const char *CuWsProxyConfig::actionType() const {
    return "conf";
}

void CuWsProxyConfig::setSource(const QString &s)
{
    QRegularExpression re("(ws[s]{0,1}\\://).*");
    QRegularExpressionMatch match = re.match(s);
    if(match.hasMatch() && match.capturedTexts().size() == 2)
        d_p->src_proto_prefix = match.captured(1);
    QString src(s);
    src.remove(d_p->src_proto_prefix);
    printf("CuWsProxyServer.setSource: src %s\n", qstoc(src));
    d_p->context->setOptions(CuData("properties-only", true));
    CuControlsReaderA * r = d_p->context->replace_reader(src.toStdString(), this);
    if(r)
        r->setSource(src);
}

void CuWsProxyConfig::unsetSource()
{
    d_p->context->disposeReader();
}

void CuWsProxyConfig::onUpdate(const CuData &da) {
    std::string message = da["msg"].toString();
    d_p->read_ok = !da["err"].toBool();

    // update link statistics
    d_p->context->getLinkStats()->addOperation();
    if(!d_p->read_ok)
        d_p->context->getLinkStats()->addError(message);

    if(da.has("type",  "property"))
        d_p->config = da;
    qDebug() << __PRETTY_FUNCTION__ << da.toString().c_str();
    if(!d_p->src_proto_prefix.isEmpty()) {
        // copy da and modify src to add prefix (ws[s]://)
        CuData dat(da);
        dat["src"] = d_p->src_proto_prefix.toStdString() + da["src"].toString();
        emit newData(dat, actionType());
    }
    else
        emit newData(da, actionType());

    deleteLater();
}
