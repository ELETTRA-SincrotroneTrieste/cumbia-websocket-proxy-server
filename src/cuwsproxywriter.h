#ifndef CUWSPROXYWRITER_H
#define CUWSPROXYWRITER_H

#include <QObject>
#include <cudatalistener.h>
#include <cudata.h>
#include <cuwsproxy.h>

#include <QString>

class CuWsProxyWriterPrivate;
class CumbiaPool;
class CuControlsWriterFactoryI;
class CuControlsFactoryPool;

/*! \brief CuWsProxyWriter is a Writer
 *
 * Connection is initiated with setSource. When new data arrives, it is displayed and the newData convenience
 * signal is emitted.
 * 
 * getContext returns a pointer to the CuContext used as a delegate for the connection.
 *
*/
class CuWsProxyWriter : public CuWsProxy, public CuDataListener
{
    Q_OBJECT
public:
    CuWsProxyWriter(QObject *o, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool);
    virtual ~CuWsProxyWriter();
    QString target() const;
    const char *actionType() const;

public slots:
    void setTarget(const QString& s);
    void clearTarget();

signals:
    void newData(const CuData&, const char* atype);

private:
    CuWsProxyWriterPrivate *m;
    void m_init();
public:
    void onUpdate(const CuData &d);
};

#endif // QUTLABEL_H
