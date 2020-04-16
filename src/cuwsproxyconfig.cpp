#include "cuwsproxyconfig.h"
#include <cucontrolsreader_abs.h>
#include <cucontrolswriter_abs.h>
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
    bool reader;
};

/** \brief Constructor with the parent widget, *CumbiaPool*  and *CuControlsFactoryPool*
 *
 *   Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
CuWsProxyConfig::CuWsProxyConfig(QObject *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool, bool reader) :
    CuWsProxy(w), CuDataListener()
{
    m_init();
    d_p->context = new CuContext(cumbia_pool, fpool);
    m->reader = reader;
}

void CuWsProxyConfig::m_init()
{
    m = new CuWsProxyConfigPrivate;
}

CuWsProxyConfig::~CuWsProxyConfig()
{
    pdelete("~CuWsProxyConfig %p %s/%s", this, actionType(), qstoc(src()));
    unsetSource();
    delete m;
}

QString CuWsProxyConfig::source() const
{
    if(CuControlsReaderA* r = d_p->context->getReader())
        return d_p->src_proto_prefix + r->source();
    return "";
}

const char *CuWsProxyConfig::actionType() const {
    if(m->reader) return "rconf";
    return "wconf";
}

void CuWsProxyConfig::setSource(const QString &s)
{
    QRegularExpression re("(ws[s]{0,1}\\://).*");
    QRegularExpressionMatch match = re.match(s);
    if(match.hasMatch() && match.capturedTexts().size() == 2)
        d_p->src_proto_prefix = match.captured(1);
    QString src(s);
    src.remove(d_p->src_proto_prefix);
    printf("CuWsProxyConfig.setSource: src %s\n", qstoc(src));
    d_p->context->setOptions(CuData("properties-only", true));
    if(m->reader) {
        CuControlsReaderA * r = d_p->context->replace_reader(src.toStdString(), this);
        if(r) r->setSource(src);
    }
    else {
        CuControlsWriterA * w = d_p->context->replace_writer(src.toStdString(), this);
        if(w) w->setTarget(src);
    }
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

    if(da.has("type",  "property")) {
        d_p->config = da;
        if(!d_p->src_proto_prefix.isEmpty()) {
            // copy da and modify src to add prefix (ws[s]://)
            CuData dat(da);
            dat["src"] = d_p->src_proto_prefix.toStdString() + da["src"].toString();
            emit newData(dat, actionType());
        }
        else
            emit newData(da, actionType());
    }
}
