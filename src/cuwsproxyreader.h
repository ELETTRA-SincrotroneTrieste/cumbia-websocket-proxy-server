#ifndef CuWsProxyReader_H
#define CuWsProxyReader_H

#include <QObject>
#include <cudatalistener.h>
#include <cucontexti.h>
#include <cudata.h>

#include <QString>

class CuWsProxyReaderPrivate;
class Cumbia;
class CumbiaPool;
class CuControlsReaderFactoryI;
class CuControlsFactoryPool;
class CuContext;
class CuLinkStats;

/*! \brief CuWsProxyReader is a reader
 *
 * Connection is initiated with setSource. When new data arrives, it is displayed and the newData convenience
 * signal is emitted.
 * 
 * getContext returns a pointer to the CuContext used as a delegate for the connection.
 *
*/
class CuWsProxyReader : public QObject, public CuDataListener, public CuContextI
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource DESIGNABLE true)

public:
    CuWsProxyReader(QObject *o, Cumbia *cumbia, const CuControlsReaderFactoryI &r_fac);

    CuWsProxyReader(QObject *o, CumbiaPool *cumbia_pool, const CuControlsFactoryPool &fpool);

    virtual ~CuWsProxyReader();

    /** \brief returns the source of the reader
     *
     * @return a QString with the name of the source
     */
    QString source() const;

    /** \brief returns a pointer to the CuContext used as a delegate for the connection.
     *
     * @return CuContext
     */
    CuContext *getContext() const;

public slots:

    /** \brief set the source and start reading
     *
     * @param the name of the source
     */
    void setSource(const QString& s);

    /** \brief disconnect the source
     *
     * remove the source and stop reading
     */
    void unsetSource();

signals:
    void newData(const CuData&);

    void linkStatsRequest(QWidget *myself, CuContextI *myself_as_cwi);

protected:

private:
    CuWsProxyReaderPrivate *d;

    void m_init();


    // CuDataListener interface
public:
    void onUpdate(const CuData &d);
};

#endif // QUTLABEL_H
