# QMake pro-file for the PokerTH dedicated server

INSTALLS += TARGET 
TARGET.files += bin/* 
TARGET.path = /usr/bin/ 

INCLUDEPATH += . \
		src \
		src/engine \
		src/gui \
		src/gui/qt \
		src/gui/qt/qttools \
		src/gui/qt/qttools/qthelper \
		src/net \
		src/engine/local_engine \
		src/engine/network_engine \
		src/config \
		src/core/tinyxml \
		src/core \

DEPENDPATH += . \
              src \
              src/config \
              src/core \
              src/engine \
              src/gui \
              src/gui/qt \
              src/gui/generic \
              src/net \
              src/core/common \
              src/core/tinyxml \
              src/engine/local_engine \
              src/engine/network_engine \
              src/net/common \
# Input
HEADERS += src/game.h \
           src/session.h \
           src/playerdata.h \
           src/gamedata.h \
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
           src/engine/network_engine/clientboard.h \
           src/engine/network_engine/clientenginefactory.h \
           src/engine/network_engine/clienthand.h \
           src/engine/network_engine/clientplayer.h \
           src/engine/network_engine/clientbero.h \
           src/gui/qttoolsinterface.h \
           src/gui/qt/qttools/qttoolswrapper.h \
           src/gui/qt/qttools/qthelper/qthelper.h \
           src/gui/generic/serverguiwrapper.h 
SOURCES += src/game.cpp \
           src/pokerth_server.cpp \
           src/session.cpp \
           src/playerdata.cpp \
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
           src/engine/network_engine/clientboard.cpp \
           src/engine/network_engine/clientenginefactory.cpp \
           src/engine/network_engine/clienthand.cpp \
           src/engine/network_engine/clientplayer.cpp \
           src/engine/network_engine/clientbero.cpp \
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
           src/net/common/servercallback.cpp \
           src/net/common/sessiondata.cpp \
           src/net/common/socket_startup_cmn.cpp \
           src/net/common/socket_helper_cmn.cpp \
           src/net/common/clientexception.cpp \
           src/net/common/netcontext.cpp \
           src/net/common/netexception.cpp \
           src/net/common/receiverhelper.cpp \
           src/gui/qttoolsinterface.cpp \
           src/gui/qt/qttools/qttoolswrapper.cpp \
           src/gui/qt/qttools/qthelper/qthelper.cpp \
           src/gui/generic/serverguiwrapper.cpp
CODECFORSRC = UTF-8

TEMPLATE = vcapp
TEMPLATE = app

win32{
	DEPENDPATH += src/net/win32/ src/core/win32
	SOURCES += src/core/win32/rand.cpp \
		src/net/win32/socket_helper.cpp \
		src/net/win32/socket_startup.cpp 
	INCLUDEPATH += ../boost/
	LIBPATH += ../boost/stage/lib
	LIBS += gdi32.lib comdlg32.lib oleaut32.lib imm32.lib winmm.lib winspool.lib ole32.lib uuid.lib user32.lib msimg32.lib shell32.lib kernel32.lib ws2_32.lib advapi32.lib
}
!win32{
	DEPENDPATH += src/net/linux/ src/core/linux
	SOURCES += src/core/linux/rand.cpp \
		src/net/linux/socket_helper.cpp \
		src/net/linux/socket_startup.cpp 
}

unix:!mac{
	exists( /usr/lib/libboost_thread-mt.so ) {
		message("Found libboost_thread-mt")
		LIBS += -lboost_thread-mt
	}
	exists( /usr/lib/libboost_thread.so ) {
		message("Found libboost_thread")
		LIBS += -lboost_thread
	}
	LIBS += -lcrypto
	## My release static libs
	#LIBS += -lcrypto
}

CONFIG += qt console release
#CONFIG += qt console warn_on debug
UI_DIR = uics/server
TARGET = bin/pokerth_server
MOC_DIR = mocs/server
OBJECTS_DIR = obj/server
QT += 
# QMAKE_CXXFLAGS_DEBUG += -g
