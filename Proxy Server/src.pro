QT += core
QT += network sql
QT -= gui

TARGET = webProxy
CONFIG += console c++11
CONFIG -= app_bundle

OBJECTS_DIR = _build
DESTDIR  = ../bin

QMAKE_CXXFLAGS += "-std=c++11"

TEMPLATE = app

SOURCES += main.cpp \
    client.cpp \
    clientsmanager.cpp \
    global.cpp \
    http.cpp \
    myfunctions.cpp \
    server.cpp \
    sql.cpp \
    threadmanager.cpp

HEADERS += \
    client.h \
    clientsmanager.h \
    global.h \
    http.h \
    myfunctions.h \
    server.h \
    sql.h \
    threadmanager.h

RESOURCES += \
    resources.qrc
