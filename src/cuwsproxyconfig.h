#ifndef CuWsProxyConfig_H
#define CuWsProxyConfig_H

#include <QObject>
#include <cudatalistener.h>
#include <cucontexti.h>
#include <cudata.h>
#include <cuwsproxy.h>

#include <QString>

class CuWsProxyConfigPrivate;
class CumbiaPool;
class CuControlsFactoryPool;

/*! \brief CuWsProxyConfig is a Config
 *
 * Connection is initiated with setSource. When new data arrives, it is displayed and the newData convenience
 * signal is emitted.
 * 
 * getContext returns a pointer to the CuContext used as a delegate for the connection.
 *
*/
class CuWsProxyConfig : public CuWsProxy, public CuDataListener
{
    Q_OBJECT
public:
    CuWsProxyConfig(QObject *o, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool, bool reader);
    virtual ~CuWsProxyConfig();
    QString source() const;
    const char* actionType() const;
public slots:
    void setSource(const QString& s);
    void unsetSource();
signals:
    void newData(const CuData&da, const char* atype);
private:
    CuWsProxyConfigPrivate *m;
    void m_init();
    // CuDataListener interface
public:
    void onUpdate(const CuData &d);
};

#endif // QUTLABEL_H
