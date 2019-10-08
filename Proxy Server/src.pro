QT += core
QT += network
QT -= gui

TARGET = myproxy
CONFIG += console c++11
CONFIG -= app_bundle

OBJECTS_DIR         = ../build
MOC_DIR             = ../build
DESTDIR             = ../bin

QMAKE_CXXFLAGS += "-std=c++11"

TEMPLATE = app

SOURCES += main.cpp \
    client.cpp \
    controlserver.cpp \
    global.cpp \
    http.cpp \
    myfunctions.cpp \
    server.cpp

# Check if the git version file exists
! include(./gitversion.pri) {
        error("Couldn't find the gitversion.pri file!")
}
! include(./myLibs.pri) {
        error("Couldn't find the gitversion.pri file!")
}

HEADERS += \
    client.h \
    controlserver.h \
    global.h \
    http.h \
    myfunctions.h \
    server.h

RESOURCES += \
    resources.qrc
