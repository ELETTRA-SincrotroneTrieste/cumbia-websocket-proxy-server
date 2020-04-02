#include "cuwsproxyreader.h"
#include "cucontrolsreader_abs.h"
#include <cumacros.h>
#include <cumbiapool.h>
#include <cudata.h>
#include <QContextMenuEvent>
#include <QMetaProperty>

#include "cucontrolsfactories_i.h"
#include "cucontrolsfactorypool.h"
#include "culinkstats.h"
#include "cucontextmenu.h"
#include "cucontext.h"

/** @private */
class CuWsProxyReaderPrivate
{
public:
    bool auto_configure;
    bool read_ok;
    CuContext *context;
};

/** \brief Constructor with the parent widget, an *engine specific* Cumbia implementation and a CuControlsReaderFactoryI interface.
 *
 *  Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
CuWsProxyReader::CuWsProxyReader(QObject *w, Cumbia *cumbia, const CuControlsReaderFactoryI &r_factory) :
    QObject(w), CuDataListener()
{
    m_init();
    d->context = new CuContext(cumbia, r_factory);
}

/** \brief Constructor with the parent widget, *CumbiaPool*  and *CuControlsFactoryPool*
 *
 *   Please refer to \ref md_src_cumbia_qtcontrols_widget_constructors documentation.
 */
CuWsProxyReader::CuWsProxyReader(QObject *w, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool) :
    QObject(w), CuDataListener()
{
    m_init();
    d->context = new CuContext(cumbia_pool, fpool);
}

void CuWsProxyReader::m_init()
{
    d = new CuWsProxyReaderPrivate;
    d->context = nullptr;
    d->auto_configure = true;
    d->read_ok = false;
}

CuWsProxyReader::~CuWsProxyReader()
{
    pdelete("~CuWsProxyReader %p", this);
    delete d->context;
    delete d;
}

QString CuWsProxyReader::source() const
{
    if(CuControlsReaderA* r = d->context->getReader())
        return r->source();
    return "";
}

/** \brief returns the pointer to the CuContext
 *
 * CuContext sets up the connection and is used as a mediator to send and get data
 * to and from the reader.
 *
 * @see CuContext
 */
CuContext *CuWsProxyReader::getContext() const
{
    return d->context;
}

/** \brief Connect the reader to the specified source.
 *
 * If a reader with a different source is configured, it is deleted.
 * If options have been set with QuContext::setOptions, they are used to set up the reader as desired.
 *
 * @see QuContext::setOptions
 * @see source
 */
void CuWsProxyReader::setSource(const QString &s)
{
    CuControlsReaderA * r = d->context->replace_reader(s.toStdString(), this);
    if(r)
        r->setSource(s);
}

void CuWsProxyReader::unsetSource()
{
    d->context->disposeReader();
}

void CuWsProxyReader::onUpdate(const CuData &da) {
    std::string message = da["msg"].toString();
    d->read_ok = !da["err"].toBool();

    // update link statistics
    d->context->getLinkStats()->addOperation();
    if(!d->read_ok)
        d->context->getLinkStats()->addError(message);

    emit newData(da);
}
