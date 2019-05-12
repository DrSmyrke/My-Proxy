TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

OBJECTS_DIR = _build
DESTDIR = ../bin

LIBS += -liw

SOURCES += main.cpp \
    device.cpp

HEADERS += \
    device.h
