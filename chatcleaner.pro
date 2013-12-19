# -------------------------------------------------
# Project created by QtCreator 2009-07-17T21:51:00
# -------------------------------------------------
CODECFORSRC = UTF-8
QT += network
QT -= gui
TARGET = chatcleaner
CONFIG += console
CONFIG -= app_bundle
DEFINES += TIXML_USE_STL
MOC_DIR = mocs
OBJECTS_DIR = obj
TEMPLATE = app
INCLUDEPATH += src/ \
	src/chatcleaner/ \
	src/net/
DEPENDPATH += src/ \
	src/chatcleaner/ \
	src/net/
SOURCES += src/chatcleaner/chatcleaner.cpp \
	src/chatcleaner/cleanerserver.cpp \
	src/chatcleaner/messagefilter.cpp \
	src/chatcleaner/badwordcheck.cpp \
	src/chatcleaner/textfloodcheck.cpp \
	src/chatcleaner/cleanerconfig.cpp \
	src/chatcleaner/capsfloodcheck.cpp \
	src/chatcleaner/letterrepeatingcheck.cpp \
	src/chatcleaner/urlcheck.cpp
HEADERS += src/chatcleaner/cleanerserver.h \
	src/chatcleaner/messagefilter.h \
	src/chatcleaner/badwordcheck.h \
	src/chatcleaner/textfloodcheck.h \
	src/chatcleaner/cleanerconfig.h \
	src/chatcleaner/capsfloodcheck.h \
	src/chatcleaner/letterrepeatingcheck.h \
	src/chatcleaner/urlcheck.h
LIBPATH += lib
LIBS += -lpokerth_lib \
	-lpokerth_protocol \
	-lprotobuf \
	-ltinyxml

win32 {
	LIBPATH += ../boost/stage/lib
	INCLUDEPATH += ../boost/
	debug:LIBPATH += debug/lib
	release:LIBPATH += release/lib
}
!win32{
	##### My release static build options
	#QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
	INCLUDEPATH += $${PREFIX}/include
}
mac {
	# make it x86_64 only
	CONFIG += x86_64
	CONFIG -= x86
	CONFIG -= ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
	QMAKE_CXXFLAGS -= -std=gnu++0x
	LIBPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/lib
	INCLUDEPATH += /Developer/SDKs/MacOSX10.6.sdk/usr/include/
	INCLUDEPATH += /usr/local/include
}
