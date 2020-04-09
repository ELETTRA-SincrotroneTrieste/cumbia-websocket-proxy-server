#ifndef CUWSPROXY_H
#define CUWSPROXY_H

#include <QObject>
#include <QString>
#include <cucontexti.h>
#include <cudata.h>

class CuContext;

class CuWsProxyPrivate {
public:
    CuData config;
    bool auto_configure;
    bool read_ok;
    CuContext *context;
    QString src_proto_prefix;
};

class CuWsProxy : public QObject, public CuContextI
{
    Q_OBJECT
public:
    explicit CuWsProxy(QObject *parent = nullptr);

    virtual ~CuWsProxy();

    QString src() const;

    virtual const CuData& getConfig() const;

    CuContext *getContext() const;

    virtual const char* actionType() const = 0;

    virtual void dispose();

    virtual bool isRunning() const;

signals:

    protected:
    CuWsProxyPrivate *d_p;

};

#endif // CUWSPROXY_H
