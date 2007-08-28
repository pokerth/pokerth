# QMake pro-file for the PokerTH dedicated server

TEMPLATE = app
CODECFORSRC = UTF-8

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
HEADERS += \
		src/game.h \
		src/session.h \
		src/playerdata.h \
		src/gamedata.h \
		src/config/configfile.h \
		src/core/rand.h \
		src/core/thread.h \
		src/engine/boardinterface.h \
		src/engine/enginefactory.h \
		src/engine/handinterface.h \
		src/engine/playerinterface.h \
		src/engine/berointerface.h \
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
		src/net/serveracceptthread.h \
		src/net/serverlobbythread.h \
		src/net/socket_helper.h \
		src/net/socket_msg.h \
		src/net/socket_startup.h \
		src/core/tinyxml/tinystr.h \
		src/core/tinyxml/tinyxml.h \
		src/engine/local_engine/cardsvalue.h \
		src/engine/local_engine/localboard.h \
		src/engine/local_engine/localenginefactory.h \
		src/engine/local_engine/localhand.h \
		src/engine/local_engine/localplayer.h \
		src/engine/local_engine/localberopreflop.h \
		src/engine/local_engine/localberoflop.h \
		src/engine/local_engine/localberoturn.h \
		src/engine/local_engine/localberoriver.h \
		src/engine/local_engine/localberopostriver.h \
		src/engine/local_engine/tools.h \
		src/engine/local_engine/localbero.h \
		src/engine/network_engine/clientboard.h \
		src/engine/network_engine/clientenginefactory.h \
		src/engine/network_engine/clienthand.h \
		src/engine/network_engine/clientplayer.h \
		src/engine/network_engine/clientbero.h \
		src/gui/qttoolsinterface.h \
		src/gui/qt/qttools/qttoolswrapper.h \
		src/gui/qt/qttools/qthelper/qthelper.h \
		src/gui/generic/serverguiwrapper.h

SOURCES += \
		src/pokerth_server.cpp

win32{
	DEPENDPATH += src/net/win32/ src/core/win32
	INCLUDEPATH += ../boost/
	LIBPATH += ../boost/stage/lib
	debug {
		LIBPATH += Debug/lib
	}
	!debug {
		LIBPATH += Release/lib
	}
	LIBS += pokerth_lib.lib
	LIBS += gdi32.lib comdlg32.lib oleaut32.lib imm32.lib winmm.lib winspool.lib ole32.lib uuid.lib user32.lib msimg32.lib shell32.lib kernel32.lib ws2_32.lib advapi32.lib
}
!win32{
	DEPENDPATH += src/net/linux/ src/core/linux
	SOURCES += src/core/linux/daemon.c
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
	LIBPATH += lib
	LIBS += -lpokerth_lib
	LIBS += -lcrypto
	TARGETDEPS += ./lib/libpokerth_lib.a
	
	## My release static libs
	#LIBS += -lcrypto
}

CONFIG += qt thread console embed_manifest_exe release
#CONFIG += qt thread console embed_manifest_exe warn_on debug
UI_DIR = uics
TARGET = bin/pokerth_server
MOC_DIR = mocs
OBJECTS_DIR = obj
QT -= gui
# QMAKE_CXXFLAGS_DEBUG += -g
