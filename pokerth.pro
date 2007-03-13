# QMake pro-file for PokerTH

INSTALLS += TARGET 
TARGET.files += bin/* 
TARGET.path = /usr/bin/ 
DEPENDPATH += . \
              src \
              src/config \
              src/core \
              src/engine \
              src/gui \
              src/net \
              src/core/common \
              src/core/tinyxml \
              src/engine/local_engine \
              src/gui/qt \
              src/net/common \
              src/gui/qt/aboutpokerth \
              src/gui/qt/connecttoserverdialog \
              src/gui/qt/createnetworkgamedialog \
              src/gui/qt/joinnetworkgamedialog \
              src/gui/qt/log \
              src/gui/qt/mainwindow \
              src/gui/qt/newlocalgamedialog \
              src/gui/qt/settingsdialog \
              src/gui/qt/startnetworkgamedialog \
              src/gui/qt/waitforservertostartgamedialog \
              src/gui/qt/mainwindow/startsplash
INCLUDEPATH += . \
               src \
               src/engine \
               src/gui \
               src/net \
               src/engine/local_engine \
               src/config \
               src/core/tinyxml \
               src/gui/qt \
               src/gui/qt/log \
               src/gui/qt/mainwindow \
               src/gui/qt/connecttoserverdialog \
               src/core \
               src/gui/qt/aboutpokerth \
               src/gui/qt/createnetworkgamedialog \
               src/gui/qt/joinnetworkgamedialog \
               src/gui/qt/newlocalgamedialog \
               src/gui/qt/settingsdialog \
               src/gui/qt/startnetworkgamedialog \
               src/gui/qt/mainwindow/startsplash \
               src/gui/qt/waitforservertostartgamedialog

# Input
HEADERS += src/game.h \
           src/session.h \
           src/config/configfile.h \
           src/core/rand.h \
           src/core/thread.h \
           src/engine/boardinterface.h \
           src/engine/enginefactory.h \
           src/engine/flopinterface.h \
           src/engine/handinterface.h \
           src/engine/playerinterface.h \
           src/engine/preflopinterface.h \
           src/engine/riverinterface.h \
           src/engine/turninterface.h \
           src/gui/guiinterface.h \
           src/net/clientcallback.h \
           src/net/clientcontext.h \
           src/net/clientexception.h \
           src/net/clientstate.h \
           src/net/clientthread.h \
           src/net/genericsocket.h \
           src/net/netpacket.h \
           src/net/resolverthread.h \
           src/net/senderthread.h \
           src/net/serverthread.h \
           src/net/socket_helper.h \
           src/net/socket_msg.h \
           src/net/socket_startup.h \
           src/core/tinyxml/tinystr.h \
           src/core/tinyxml/tinyxml.h \
           src/engine/local_engine/cardsvalue.h \
           src/engine/local_engine/localboard.h \
           src/engine/local_engine/localenginefactory.h \
           src/engine/local_engine/localflop.h \
           src/engine/local_engine/localhand.h \
           src/engine/local_engine/localplayer.h \
           src/engine/local_engine/localpreflop.h \
           src/engine/local_engine/localriver.h \
           src/engine/local_engine/localturn.h \
           src/engine/local_engine/tools.h \
           src/gui/qt/guiwrapper.h \
           src/gui/qt/aboutpokerth/aboutpokerthimpl.h \
           src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.h \
           src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.h \
           src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.h \
           src/gui/qt/log/log.h \
           src/gui/qt/mainwindow/mainwindowimpl.h \
           src/gui/qt/mainwindow/mycardspixmaplabel.h \
           src/gui/qt/newlocalgamedialog/newgamedialogimpl.h \
           src/gui/qt/settingsdialog/settingsdialogimpl.h \
           src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.h \
           src/gui/qt/waitforservertostartgamedialog/waitforservertostartgamedialogimpl.h \
           src/gui/qt/mainwindow/startsplash/startsplash.h
FORMS += src/gui/qt/aboutpokerth.ui \
         src/gui/qt/connecttoserverdialog.ui \
         src/gui/qt/createnetworkgamedialog.ui \
         src/gui/qt/joinnetworkgamedialog.ui \
         src/gui/qt/mainwindow.ui \
         src/gui/qt/newgamedialog.ui \
         src/gui/qt/settingsdialog.ui \
         src/gui/qt/startnetworkgamedialog.ui \
         src/gui/qt/waitforservertostartgamedialog.ui
SOURCES += src/game.cpp \
           src/pokerth.cpp \
           src/session.cpp \
           src/config/configfile.cpp \
           src/engine/boardinterface.cpp \
           src/engine/enginefactory.cpp \
           src/engine/flopinterface.cpp \
           src/engine/handinterface.cpp \
           src/engine/playerinterface.cpp \
           src/engine/preflopinterface.cpp \
           src/engine/riverinterface.cpp \
           src/engine/turninterface.cpp \
           src/gui/guiinterface.cpp \
           src/core/common/thread.cpp \
           src/core/tinyxml/tinystr.cpp \
           src/core/tinyxml/tinyxml.cpp \
           src/core/tinyxml/tinyxmlerror.cpp \
           src/core/tinyxml/tinyxmlparser.cpp \
           src/engine/local_engine/cardsvalue.cpp \
           src/engine/local_engine/localboard.cpp \
           src/engine/local_engine/localenginefactory.cpp \
           src/engine/local_engine/localflop.cpp \
           src/engine/local_engine/localhand.cpp \
           src/engine/local_engine/localplayer.cpp \
           src/engine/local_engine/localpreflop.cpp \
           src/engine/local_engine/localriver.cpp \
           src/engine/local_engine/localturn.cpp \
           src/engine/local_engine/tools.cpp \
           src/gui/qt/guiwrapper.cpp \
           src/net/common/connectdata.cpp \
           src/net/common/clientcallback.cpp \
           src/net/common/clientcontext.cpp \
           src/net/common/clientstate.cpp \
           src/net/common/clientthread.cpp \
           src/net/common/netpacket.cpp \
           src/net/common/resolverthread.cpp \
           src/net/common/senderthread.cpp \
           src/net/common/sendercallback.cpp \
           src/net/common/serverthread.cpp \
           src/net/common/servercontext.cpp \
           src/net/common/serverexception.cpp \
           src/net/common/serverrecvthread.cpp \
           src/net/common/serverrecvstate.cpp \
           src/net/common/socket_helper_cmn.cpp \
           src/net/common/clientexception.cpp \
           src/net/common/netcallback.cpp \
           src/net/common/netcontext.cpp \
           src/net/common/netexception.cpp \
           src/net/common/receiverhelper.cpp \
           src/gui/qt/aboutpokerth/aboutpokerthimpl.cpp \
           src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.cpp \
           src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.cpp \
           src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.cpp \
           src/gui/qt/log/log.cpp \
           src/gui/qt/mainwindow/mainwindowimpl.cpp \
           src/gui/qt/mainwindow/mycardspixmaplabel.cpp \
           src/gui/qt/newlocalgamedialog/newgamedialogimpl.cpp \
           src/gui/qt/settingsdialog/settingsdialogimpl.cpp \
           src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.cpp \
           src/gui/qt/waitforservertostartgamedialog/waitforservertostartgamedialogimpl.cpp \
           src/gui/qt/mainwindow/startsplash/startsplash.cpp
RESOURCES += src/gui/qt/resources.qrc

TEMPLATE = vcapp
TEMPLATE = app
win32 {
	DEPENDPATH += src/net/win32/ src/core/win32
	SOURCES += src/core/win32/rand.cpp \ 
		src/net/win32/socket_helper.cpp \
           	src/net/win32/socket_startup.cpp 
	INCLUDEPATH += ../boost/
	LIBPATH += ../boost/stage/lib
	LIBS += ws2_32.lib advapi32.lib
}
!win32 {
	DEPENDPATH += src/net/linux/ src/core/linux
	SOURCES += src/core/linux/rand.cpp \
		src/net/linux/socket_helper.cpp \
           	src/net/linux/socket_startup.cpp 
	LIBS += -lboost_thread -l ssl
}

CONFIG += qt release
UI_DIR = uics
TARGET = bin/pokerth
MOC_DIR = mocs
OBJECTS_DIR = obj
QT += 
