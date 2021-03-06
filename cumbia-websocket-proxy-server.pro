include (/usr/local/cumbia-libs/include/qumbia-epics-controls/qumbia-epics-controls.pri)
include (/usr/local/cumbia-libs/include/qumbia-tango-controls/qumbia-tango-controls.pri)

INSTALL_DIR=$${INSTALL_ROOT}/bin

message("-")
message("cumbia-websocket-proxy-server will be installed under $${INSTALL_DIR}")
message("-")

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
        src/cuwsproxy.cpp \
        src/cuwsproxyconfig.cpp \
        src/cuwsproxyreader.cpp \
        src/cumbia-websocket-proxy-server.cpp \
        src/cuwsproxywriter.cpp \
        src/cuwssourcevalidator.cpp \
        src/linkpool.cpp

HEADERS +=  \
    src/cuwsdatatojson.h \
    src/cuwsproxy.h \
    src/cuwsproxyconfig.h \
        src/cuwsproxyreader.h \
    src/cumbia-websocket-proxy-server.h \
    src/cuwsproxywriter.h \
    src/cuwssourcevalidator.h \
    src/linkpool.h

#
INCLUDEPATH += src

TARGET   = bin/cumbia-websocket-proxy-server

target.path = $${INSTALL_DIR}

INSTALLS += target

# unix:LIBS += -L. -lmylib

# unix:INCLUDEPATH +=  . ../../src

