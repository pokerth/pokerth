# -------------------------------------------------
# Project created by QtCreator 2009-07-17T21:51:00
# -------------------------------------------------
CODECFORSRC = UTF-8
QT += network
QT -= gui
TARGET = chatcleaner
CONFIG += console
CONFIG -= app_bundle
MOC_DIR = mocs
OBJECTS_DIR = obj
TEMPLATE = app
INCLUDEPATH += src/ src/chatcleaner/ 
DEPENDPATH += src/ src/chatcleaner/
SOURCES += chatcleaner.cpp \
    cleanerserver.cpp \
    messagefilter.cpp \
    badwordcheck.cpp \
    textfloodcheck.cpp
HEADERS += cleanerserver.h \
    messagefilter.h \
    badwordcheck.h \
    textfloodcheck.h
