# Diese Datei wurde mit dem qmake-Manager von KDevelop erstellt. 
# ------------------------------------------- 
# Unterordner relativ zum Projektordner: .
# Das Target ist eine Anwendung:  bin/pokerth

INSTALLS += TARGET 
TARGET.files += bin/* 
TARGET.path = /usr/bin/ 
FORMS += mainwindow.ui \
         aboutpokerth.ui \
         newgamedialog.ui \
         settingsdialog.ui \
	 joinnetworkgamedialog.ui \
	 connecttoserverdialog.ui \
	 waitforservertostartgamedialog.ui \
	 createnetworkgamedialog.ui \
	 startnetworkgamedialog.ui 	 
HEADERS += tinystr.h \
           tinyxml.h \
           mainwindowimpl.h \
           newgamedialogimpl.h \
           aboutpokerthimpl.h \
           settingsdialogimpl.h \
	   joinnetworkgamedialogimpl.h \
	   connecttoserverdialogimpl.h \
	   waitforservertostartgamedialogimpl.h \
	   createnetworkgamedialogimpl.h \
  	   startnetworkgamedialogimpl.h \
           configfile.h \
           log.h \
           guiinterface.h \
           guiwrapper.h \
           session.h \
           game.h \
           localhand.h \
           localboard.h \
           localplayer.h \
           cardsvalue.h \
           localpreflop.h \
           localflop.h \
           localturn.h \
           localriver.h \
           tools.h \
           handinterface.h \
           playerinterface.h \
           boardinterface.h \
           preflopinterface.h \
           flopinterface.h \
           turninterface.h \
           riverinterface.h \
           enginefactory.h \
           localenginefactory.h \
           startsplash.h \
           mycardspixmaplabel.h \
	   clientdata.h \
	   clientstate.h \
	   clientthread.h \
	   genericsocket.h \
	   socket_helper.h \
	   socket_msg.h \
	   socket_startup.h \
	   thread.h
SOURCES += pokerth.cpp \
           tinystr.cpp \
           tinyxml.cpp \
           tinyxmlerror.cpp \
           tinyxmlparser.cpp \
           mainwindowimpl.cpp \
           newgamedialogimpl.cpp \
           aboutpokerthimpl.cpp \
           settingsdialogimpl.cpp \
 	   joinnetworkgamedialogimpl.cpp \
 	   connecttoserverdialogimpl.cpp \
	   waitforservertostartgamedialogimpl.cpp \
	   createnetworkgamedialogimpl.cpp \
  	   startnetworkgamedialogimpl.cpp \
           configfile.cpp \
           log.cpp \
           guiinterface.cpp \
           guiwrapper.cpp \
           session.cpp \
           game.cpp \
           localhand.cpp \
           localboard.cpp \
           localplayer.cpp \
           cardsvalue.cpp \
           localpreflop.cpp \
           localflop.cpp \
           localturn.cpp \
           localriver.cpp \
           tools.cpp \
           handinterface.cpp \
           playerinterface.cpp \
           boardinterface.cpp \
           preflopinterface.cpp \
           flopinterface.cpp \
           turninterface.cpp \
           riverinterface.cpp \
           enginefactory.cpp \
           localenginefactory.cpp \
           startsplash.cpp \
           mycardspixmaplabel.cpp \
	   socket_helper.cpp \	
	   socket_startup.cpp \
	   clientstate.cpp \
	   clientthread.cpp \
	   resolverthread.cpp \
	   netpacket.cpp \
	   senderthread.cpp \
	   serverthread.cpp \
	   clientdata.cpp \
	   clientcallback.cpp \
	   socket_helper_cmn.cpp \
	   thread.cpp \
	   rand.cpp
TEMPLATE = vcapp
RESOURCES = src/gui/qt/resources.qrc
TEMPLATE = app
DEPENDPATH += . src uics src/config src/core/tinyxml src/gui src/gui/qt src/gui/qt/mainwindow/startsplash src/gui/qt/mainwindow src/gui/qt/aboutpokerth src/gui/qt/joinnetworkgamedialog src/gui/qt/connecttoserverdialog src/gui/qt/newlocalgamedialog src/gui/qt/settingsdialog src/gui/qt/waitforservertostartgamedialog src/gui/qt/startnetworkgamedialog src/gui/qt/createnetworkgamedialog src/gui/qt/log src/engine src/engine/local_engine src/net src/net/common src/core src/core/common
INCLUDEPATH += . src uics src/config src/core/tinyxml src/gui src/gui/qt src/gui/qt/mainwindow/startsplash src/gui/qt/mainwindow src/gui/qt/aboutpokerth src/gui/qt/joinnetworkgamedialog src/gui/qt/connecttoserverdialog src/gui/qt/newlocalgamedialog src/gui/qt/settingsdialog src/gui/qt/waitforservertostartgamedialog src/gui/qt/startnetworkgamedialog src/gui/qt/createnetworkgamedialog src/gui/qt/log src/engine src/engine/local_engine src/net src/net/common src/core src/core/common
win32 {
	DEPENDPATH += src/net/win32/ src/core/win32
	INCLUDEPATH += ../boost/
	LIBPATH += ../boost/stage/lib
	LIBS += ws2_32.lib advapi32.lib
}
!win32 {
	DEPENDPATH += src/net/linux/ src/core/linux
	LIBS += -lboost_thread -l ssl
}

CONFIG += qt release
UI_DIR = uics
TARGET = bin/pokerth
MOC_DIR = mocs
OBJECTS_DIR = obj
QT += 
