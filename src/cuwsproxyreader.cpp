#include "cuwsproxyreader.h"
#include "cucontrolsreader_abs.h"
#include <cumacros.h>
#include <cumbiapool.h>
#include <cudata.h>
#include <QContextMenuEvent>
#include <QMetaProperty>
#include <QRegularExpression>
#include <QtDebug>

#include "cucontrolsfactories_i.h"
#include "cucontrolsfactorypool.h"
#include "culinkstats.h"
#include "cucontextmenu.h"
#include "cucontext.h"

/** @private */
class CuWsProxyReaderPrivate
{
public:
};

/** \brief Constructor with the parent widget, *CumbiaPool*  and *CuControlsFactoryPool*
 *
 *   Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
CuWsProxyReader::CuWsProxyReader(QObject *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool) :
    CuWsProxy(w), CuDataListener() {
    m = new CuWsProxyReaderPrivate;
    d_p->context = new CuContext(cumbia_pool, fpool);
}

CuWsProxyReader::~CuWsProxyReader() {
    pdelete("~CuWsProxyReader %p %s/%s", this, actionType(), qstoc(src()));
    unsetSource();
    delete m;
}

QString CuWsProxyReader::source() const {
    if(CuControlsReaderA* r = d_p->context->getReader())
        return d_p->src_proto_prefix + r->source();
    return "";
}

/** \brief Connect the reader to the specified source.
 *
 * If a reader with a different source is configured, it is deleted.
 * If options have been set with QuContext::setOptions, they are used to set up the reader as desired.
 *
 * @see QuContext::setOptions
 * @see source
 */
void CuWsProxyReader::setSource(const QString &s) {
    QRegularExpression re("(ws[s]{0,1}\\://).*");
    QRegularExpressionMatch match = re.match(s);
    if(match.hasMatch() && match.capturedTexts().size() == 2)
        d_p->src_proto_prefix = match.captured(1);
    QString src(s);
    src.remove(d_p->src_proto_prefix);
    d_p->context->setOptions(CuData("no-properties", true));
    CuControlsReaderA * r = d_p->context->replace_reader(src.toStdString(), this);
    if(r)
        r->setSource(src);
}

void CuWsProxyReader::unsetSource() {
    d_p->context->disposeReader();
}

void CuWsProxyReader::onUpdate(const CuData &da) {
    std::string message = da["msg"].toString();
    d_p->read_ok = !da["err"].toBool();

    // update link statistics
    d_p->context->getLinkStats()->addOperation();
    if(!d_p->read_ok)
        d_p->context->getLinkStats()->addError(message);

    if(da.has("type",  "property"))
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

const char *CuWsProxyReader::actionType() const {
    return "subscribe";
}
