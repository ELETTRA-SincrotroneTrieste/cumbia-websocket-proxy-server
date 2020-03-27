#include <QApplication>
#include "cumbia-websocket-proxy-server.h"

#include <cumbiapool.h>
#include <cuthreadfactoryimpl.h>
#include <qthreadseventbridgefactory.h>

#include <QCommandLineOption>
#include <QCommandLineParser>

#define CVSVERSION "$Name:  $"


int main(int argc, char *argv[])
{
    int ret;
    QApplication qu_app( argc, argv );
    qu_app.setOrganizationName("Elettra");
    qu_app.setApplicationName("Cumbia Websocket Proxy Server");
    QString version(CVSVERSION);
    qu_app.setApplicationVersion(version);
    qu_app.setProperty("author", "Giacomo");
    qu_app.setProperty("mail", "giacomo.strangolino@elettra.eu");
    qu_app.setProperty("phone", "0403758073");
    qu_app.setProperty("office", "T2PT025");
    qu_app.setProperty("hwReferent", "$HW_REFERENT$"); /* name of the referent that provides the device server */
    
    QCommandLineParser parser;
    QCommandLineOption port_o(QStringList() << "p" << "port", "Listen port" , "port", "12702");
    QCommandLineOption addr_o(QStringList() << "s" << "--server-address", "Server address", "address", "");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(port_o);
    parser.addOption(addr_o);
    parser.process(qu_app);

    CumbiaPool *cu_p = new CumbiaPool();
    CuWsProxyServer *w = new CuWsProxyServer(cu_p, NULL, parser.value(addr_o), parser.value(port_o).toInt());

    ret = qu_app.exec();
    delete w;
    delete cu_p->get("tango");
    delete cu_p->get("epics");
    return ret;
}
