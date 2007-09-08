# QMake pro-file for PokerTH common library

TEMPLATE = lib
CODECFORSRC = UTF-8

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
		src/core/libircclient/include \
		src/core

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
		src/core/libircclient \
		src/core/libircclient/src \
		src/core/libircclient/include \
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
		src/core/thread.h \
		src/core/crypthelper.h \
		src/core/avatarmanager.h \
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
		src/net/servergamethread.h \
		src/net/servergamestate.h \
		src/net/serverlobbythread.h \
		src/net/socket_helper.h \
		src/net/socket_msg.h \
		src/net/socket_startup.h \
		src/core/tinyxml/tinystr.h \
		src/core/tinyxml/tinyxml.h \
		src/core/libircclient/include/libircclient.h \
		src/core/libircclient/include/config.h \
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
		src/game.cpp \
		src/session.cpp \
		src/playerdata.cpp \
		src/config/configfile.cpp \
		src/engine/boardinterface.cpp \
		src/engine/enginefactory.cpp \
		src/engine/handinterface.cpp \
		src/engine/playerinterface.cpp \
		src/engine/berointerface.cpp \
		src/gui/guiinterface.cpp \
		src/core/common/thread.cpp \
		src/core/common/crypthelper.cpp \
		src/core/common/avatarmanager.cpp \
		src/core/tinyxml/tinystr.cpp \
		src/core/tinyxml/tinyxml.cpp \
		src/core/tinyxml/tinyxmlerror.cpp \
		src/core/tinyxml/tinyxmlparser.cpp \
		src/core/libircclient/src/libircclient.c \
		src/engine/local_engine/cardsvalue.cpp \
		src/engine/local_engine/localboard.cpp \
		src/engine/local_engine/localenginefactory.cpp \
		src/engine/local_engine/localhand.cpp \
		src/engine/local_engine/localplayer.cpp \
		src/engine/local_engine/localberopreflop.cpp \
		src/engine/local_engine/localberoflop.cpp \
		src/engine/local_engine/localberoturn.cpp \
		src/engine/local_engine/localberoriver.cpp \
		src/engine/local_engine/localberopostriver.cpp \
		src/engine/local_engine/tools.cpp \
		src/engine/local_engine/localbero.cpp \
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
		src/net/common/servercontext.cpp \
		src/net/common/serverexception.cpp \
		src/net/common/serveracceptthread.cpp \
		src/net/common/servergamethread.cpp \
		src/net/common/servergamestate.cpp \
		src/net/common/serverlobbythread.cpp \
		src/net/common/servercallback.cpp \
		src/net/common/sessiondata.cpp \
		src/net/common/sessionmanager.cpp \
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

win32{
	DEPENDPATH += src/net/win32/ src/core/win32
	SOURCES += src/net/win32/socket_helper.cpp \
		src/net/win32/socket_startup.cpp
	INCLUDEPATH += ../boost/ ../OpenSSL/include
}
!win32{
	DEPENDPATH += src/net/linux/ src/core/linux
	SOURCES += src/net/linux/socket_helper.cpp \
		src/net/linux/socket_startup.cpp 
}

mac{
	# make it universal  
	CONFIG += x86 
	CONFIG += ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	#	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/

	INCLUDEPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/include/
	INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
	INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers		
}

CONFIG += staticlib qt thread warn_on release
#CONFIG += staticlib qt thread warn_on debug
UI_DIR = uics
TARGET = lib/pokerth_lib
MOC_DIR = mocs
OBJECTS_DIR = obj
QT += 
# QMAKE_CXXFLAGS_DEBUG += -g
