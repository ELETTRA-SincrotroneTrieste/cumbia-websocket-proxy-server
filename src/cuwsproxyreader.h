#ifndef CuWsProxyReader_H
#define CuWsProxyReader_H

#include <QObject>
#include <cudatalistener.h>
#include <cucontexti.h>
#include <cudata.h>
#include <cuwsproxy.h>

#include <QString>

class CuWsProxyReaderPrivate;
class CumbiaPool;
class CuControlsFactoryPool;

/*! \brief CuWsProxyReader is a reader
 *
 * Connection is initiated with setSource. When new data arrives, it is displayed and the newData convenience
 * signal is emitted.
 * 
 * getContext returns a pointer to the CuContext used as a delegate for the connection.
 *
*/
class CuWsProxyReader : public CuWsProxy, public CuDataListener
{
    Q_OBJECT
public:
    CuWsProxyReader(QObject *o, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool);
    virtual ~CuWsProxyReader();
    QString source() const;
    const char *actionType() const;

public slots:
    void setSource(const QString& s);
    void unsetSource();

signals:
    void newData(const CuData&da, const char* atype);

private:
    CuWsProxyReaderPrivate *m;
    // CuDataListener interface
public:
    void onUpdate(const CuData &d);
};

#endif // QUTLABEL_H
