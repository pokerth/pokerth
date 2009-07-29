# -------------------------------------------------
# Project created by QtCreator 2009-07-17T21:51:00
# -------------------------------------------------
CODECFORSRC = UTF-8
QT += network
QT -= gui
TARGET = chatcleaner
CONFIG += console \
    debug
CONFIG -= app_bundle
MOC_DIR = mocs
OBJECTS_DIR = obj
TEMPLATE = app
INCLUDEPATH += src/ \
    src/third_party/tinyxml \
    src/chatcleaner/
DEPENDPATH += src/ \
    src/third_party/tinyxml \
    src/chatcleaner/
SOURCES += chatcleaner.cpp \
    cleanerserver.cpp \
    messagefilter.cpp \
    badwordcheck.cpp \
    textfloodcheck.cpp \
    cleanerconfig.cpp \
    tinystr.cpp \
    tinyxml.cpp \
    tinyxmlerror.cpp \
    tinyxmlparser.cpp \
	capsfloodcheck.cpp
HEADERS += cleanerserver.h \
    messagefilter.h \
    badwordcheck.h \
    textfloodcheck.h \
    cleanerconfig.h \
    tinyxml.h \
    tinystr.h \
	capsfloodcheck.h
