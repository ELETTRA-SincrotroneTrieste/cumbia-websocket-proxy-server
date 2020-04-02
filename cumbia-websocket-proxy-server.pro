include (/usr/local/cumbia-libs/include/qumbia-epics-controls/qumbia-epics-controls.pri)
include (/usr/local/cumbia-libs/include/qumbia-tango-controls/qumbia-tango-controls.pri)

TEMPLATE = app

QT +=  core
QT -= gui


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets x11extras

QT += websockets

CONFIG += debug

DEFINES -= QT_NO_DEBUG_OUTPUT

OBJECTS_DIR = obj

# RESOURCES +=


SOURCES += src/main.cpp \
        src/cuwsdatatojson.cpp \
        src/cuwsproxyreader.cpp \
        src/cumbia-websocket-proxy-server.cpp \
        src/cuwssourcevalidator.cpp

HEADERS +=  \
    src/cuwsdatatojson.h \
        src/cuwsproxyreader.h \
    src/cumbia-websocket-proxy-server.h \
    src/cuwssourcevalidator.h

#
INCLUDEPATH += src

TARGET   = bin/cumbia-websocket-proxy-server

# unix:LIBS += -L. -lmylib

# unix:INCLUDEPATH +=  . ../../src

