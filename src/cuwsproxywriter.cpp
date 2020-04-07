#include "cuwsproxywriter.h"
#include "cucontrolswriter_abs.h"
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

class CuWsProxyWriterPrivate {
public:
};

/** \brief Constructor with the parent widget, *CumbiaPool*  and *CuControlsFactoryPool*
 *
 *   Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
CuWsProxyWriter::CuWsProxyWriter(QObject *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool) :
    CuWsProxy(w), CuDataListener() {
    m_init();
    d_p->context = new CuContext(cumbia_pool, fpool);
}

void CuWsProxyWriter::m_init() {
    m = new CuWsProxyWriterPrivate;
}

CuWsProxyWriter::~CuWsProxyWriter() {
    pdelete("~CuWsProxyWriter %p", this);
    delete m;
}

QString CuWsProxyWriter::target() const {
    if(CuControlsWriterA* r = d_p->context->getWriter())
        return d_p->src_proto_prefix + r->target();
    return "";
}

const char *CuWsProxyWriter::actionType() const {
    return "write";
}

/** \brief Connect the Writer to the specified source.
 *
 * If a Writer with a different source is configured, it is deleted.
 * If options have been set with QuContext::setOptions, they are used to set up the Writer as desired.
 *
 * @see QuContext::setOptions
 * @see source
 */
void CuWsProxyWriter::setTarget(const QString &s)
{
    QRegularExpression re("(ws[s]{0,1}\\://).*");
    QRegularExpressionMatch match = re.match(s);
    if(match.hasMatch() && match.capturedTexts().size() == 2)
        d_p->src_proto_prefix = match.captured(1);
    QString src(s);
    src.remove(d_p->src_proto_prefix);
    printf("CuWsProxyWriter.setTarget: target %s\n", qstoc(src));
    CuControlsWriterA * w = d_p->context->replace_writer(src.toStdString(), this);
    if(w)
        w->setTarget(src);
}

void CuWsProxyWriter::clearTarget()
{
    d_p->context->disposeWriter();
}

void CuWsProxyWriter::onUpdate(const CuData &da) {
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
}
