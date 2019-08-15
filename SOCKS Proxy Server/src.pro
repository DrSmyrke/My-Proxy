QT += core
QT += network
QT -= gui

TARGET = socksproxy
CONFIG += console c++11
CONFIG -= app_bundle

OBJECTS_DIR = _build
DESTDIR  = ../bin

QMAKE_CXXFLAGS += "-std=c++11"

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp \
    client.cpp \
    global.cpp

HEADERS += \
    server.h \
    client.h \
    global.h
