# QMake pro-file for PokerTH common library

isEmpty( PREFIX ){
	PREFIX =/usr
}

TEMPLATE = lib
CODECFORSRC = UTF-8

CONFIG += staticlib thread exceptions rtti stl warn_on
UI_DIR = uics
TARGET = lib/pokerth_lib
QMAKE_CLEAN += ./lib/libpokerth_lib.a
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += ENABLE_IPV6 TIXML_USE_STL BOOST_FILESYSTEM_DEPRECATED
QT -= core gui
#PRECOMPILED_HEADER = src/pch_lib.h

# Check for c++11
include(pokerth_common.pro)

INCLUDEPATH += . \
		src \
		src/engine \
		src/gui \
		src/gui/qt \
		src/gui/qt/qttools \
		src/net \
		src/engine/local_engine \
		src/engine/network_engine \
		src/config \
		src/core \
		src/third_party/websocketpp

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
		src/engine/local_engine \
		src/engine/network_engine \
		src/net/common

# Input
HEADERS += \
		src/engine/game.h \
		src/session.h \
		src/playerdata.h \
		src/gamedata.h \
		src/config/configfile.h \
		src/core/thread.h \
		src/core/crypthelper.h \
		src/core/avatarmanager.h \
		src/core/pokerthexception.h \
		src/engine/boardinterface.h \
		src/engine/enginefactory.h \
		src/engine/handinterface.h \
		src/engine/playerinterface.h \
		src/engine/berointerface.h \
		src/gui/guiinterface.h \
		src/net/chatcleanermanager.h \
		src/net/chatcleanercallback.h \
		src/net/clientcallback.h \
		src/net/clientcontext.h \
		src/net/clientexception.h \
		src/net/netcontext.h \
		src/net/clientstate.h \
		src/net/clientthread.h \
		src/net/genericsocket.h \
		src/net/netpacket.h \
		src/net/netpacketvalidator.h \
		src/net/senderhelper.h \
		src/net/sendercallback.h \
		src/net/serverexception.h \
		src/net/serveracceptinterface.h \
		src/net/serveraccepthelper.h \
		src/net/serveracceptwebhelper.h \
		src/net/servergame.h \
		src/net/servergamestate.h \
		src/net/serverlobbythread.h \
		src/net/serverbanmanager.h \
		src/net/servercallback.h \
		src/net/serveradminbot.h \
		src/net/serverlobbybot.h \
		src/net/serverircbotcallback.h \
		src/net/sessiondata.h \
		src/net/sessiondatacallback.h \
		src/net/sessionmanager.h \
		src/net/socket_helper.h \
		src/net/socket_msg.h \
		src/net/socket_startup.h \
		src/net/irccallback.h \
		src/net/ircthread.h \
		src/net/netexception.h \
		src/net/servermanager.h \
		src/net/transferdata.h \
		src/net/transferhelper.h \
		src/net/uploaderthread.h \
		src/net/uploadhelper.h \
		src/net/downloaderthread.h \
		src/net/downloadhelper.h \
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
		src/engine/local_engine/localexception.h \
		src/engine/local_engine/arraydata.h \
		src/engine/log.h \
		src/engine/network_engine/clientboard.h \
		src/engine/network_engine/clientenginefactory.h \
		src/engine/network_engine/clienthand.h \
		src/engine/network_engine/clientplayer.h \
		src/engine/network_engine/clientbero.h \
		src/db/serverdbcallback.h \
		src/db/serverdbfactory.h \
		src/db/serverdbinterface.h \
		src/db/serverdbgeneric.h \
		src/db/serverdbfactorygeneric.h \
		src/gui/qttoolsinterface.h \
		src/gui/generic/serverguiwrapper.h \
		src/net/receivebuffer.h \
		src/net/asioreceivebuffer.h \
		src/net/webreceivebuffer.h \
		src/net/sendbuffer.h \
		src/net/asiosendbuffer.h \
		src/net/websendbuffer.h \
		src/net/servermanagerfactory.h \
		src/net/uploadcallback.h \
		src/net/websocket_defs.h \
		src/net/websocketdata.h

SOURCES += \
		src/engine/game.cpp \
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
		src/core/common/pokerthexception.cpp \
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
		src/engine/local_engine/localbero.cpp \
		src/engine/local_engine/localexception.cpp \
		src/engine/local_engine/arraydata.cpp \
		src/engine/log.cpp \
		src/engine/network_engine/clientboard.cpp \
		src/engine/network_engine/clientenginefactory.cpp \
		src/engine/network_engine/clienthand.cpp \
		src/engine/network_engine/clientplayer.cpp \
		src/engine/network_engine/clientbero.cpp \
		src/net/common/chatcleanermanager.cpp \
		src/net/common/chatcleanercallback.cpp \
		src/net/common/clientcallback.cpp \
		src/net/common/clientcontext.cpp \
		src/net/common/clientstate.cpp \
		src/net/common/clientthread.cpp \
		src/net/common/downloadhelper.cpp \
		src/net/common/downloaderthread.cpp \
		src/net/common/netpacket.cpp \
		src/net/common/netpacketvalidator.cpp \
		src/net/common/senderhelper.cpp \
		src/net/common/sendercallback.cpp \
		src/net/common/serverexception.cpp \
		src/net/common/serveracceptinterface.cpp \
		src/net/common/serveracceptwebhelper.cpp \
		src/net/common/servergame.cpp \
		src/net/common/servergamestate.cpp \
		src/net/common/serverlobbythread.cpp \
		src/net/common/serverbanmanager.cpp \
		src/net/common/servercallback.cpp \
		src/net/common/serveradminbot.cpp \
		src/net/common/serverlobbybot.cpp \
		src/net/common/serverircbotcallback.cpp \
		src/net/common/sessiondata.cpp \
		src/net/common/sessiondatacallback.cpp \
		src/net/common/sessionmanager.cpp \
		src/net/common/socket_startup.cpp \
		src/net/common/clientexception.cpp \
		src/net/common/netcontext.cpp \
		src/net/common/netexception.cpp \
		src/net/common/irccallback.cpp \
		src/net/common/servermanager.cpp \
		src/net/common/transferhelper.cpp \
		src/net/common/uploaderthread.cpp \
		src/net/common/uploadhelper.cpp \
		src/gui/generic/serverguiwrapper.cpp \
		src/gui/qttoolsinterface.cpp \
		src/net/common/sendbuffer.cpp \
		src/net/common/asiosendbuffer.cpp \
		src/net/common/websendbuffer.cpp \
		src/net/common/receivebuffer.cpp \
		src/net/common/asioreceivebuffer.cpp \
		src/net/common/webreceivebuffer.cpp \
		src/net/common/uploadcallback.cpp

!android:!android_test{
	SOURCES += src/engine/local_engine/tools.cpp
}
android|android_test{
	SOURCES += src/engine/local_engine/tools_android.cpp
}

official_server:!win32:!gui_800x480:!client:{
	# INCLUDEPATH += pkth_stat/daemon_lib/src # note: relict from former official/closed db build
	DEFINES += POKERTH_OFFICIAL_SERVER
	HEADERS += src/dbofficial/asyncdbauth.h \
		src/dbofficial/asyncdbcreategame.h \
		src/dbofficial/asyncdbgameplace.h \
		src/dbofficial/asyncdbupdatescore.h \
		src/dbofficial/asyncdbquery.h \
		src/dbofficial/serverdbthread.h \
		src/dbofficial/serverdbfactoryinternal.h \
		src/dbofficial/compositeasyncdbquery.h \
		src/dbofficial/singleasyncdbquery.h \
		src/dbofficial/querycontext.h \
		src/dbofficial/asyncdbendgame.h \
		src/dbofficial/db_table_defs.h \
		src/dbofficial/asyncdblogin.h \
		src/dbofficial/asyncdbreportavatar.h \
		src/dbofficial/asyncdbreportgame.h \
		src/dbofficial/asyncdbavatarblacklist.h \
		src/dbofficial/asyncdbadminplayers.h \
		src/dbofficial/asyncdbblockplayer.h \
		src/dbofficial/dbidmanager.h
	SOURCES += src/dbofficial/asyncdbauth.cpp \
		src/dbofficial/asyncdbcreategame.cpp \
		src/dbofficial/asyncdbgameplace.cpp \
		src/dbofficial/asyncdbupdatescore.cpp \
		src/dbofficial/asyncdbquery.cpp \
		src/dbofficial/serverdbthread.cpp \
		src/dbofficial/serverdbfactoryinternal.cpp \
		src/dbofficial/singleasyncdbquery.cpp \
		src/dbofficial/compositeasyncdbquery.cpp \
		src/dbofficial/querycontext.cpp \
		src/dbofficial/asyncdbendgame.cpp \
		src/dbofficial/asyncdblogin.cpp \
		src/dbofficial/asyncdbreportavatar.cpp \
		src/dbofficial/asyncdbreportgame.cpp \
		src/dbofficial/asyncdbavatarblacklist.cpp \
		src/dbofficial/asyncdbadminplayers.cpp \
		src/dbofficial/asyncdbblockplayer.cpp \
		src/dbofficial/dbidmanager.cpp
    INCLUDEPATH += /usr/include \
        /usr/include/mysql \
        /usr/include/mysql++
}

win32{
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
	DEFINES += HAVE_OPENSSL
	DEPENDPATH += src/net/win32/ src/core/win32
	win32-g++ {
		INCLUDEPATH += ../boost/ ../GnuTLS/include ../gsasl/include ../curl/include ../zlib ../sqlite ../openssl/include
	}
}
!win32{
	##### My release static build options
	#QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
	INCLUDEPATH += $${PREFIX}/include /opt/gsasl/include
}

mac{
	# make it x86_64 only
	CONFIG += x86_64
	CONFIG -= x86
	CONFIG -= ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
	QMAKE_CXXFLAGS -= -std=gnu++0x

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	#	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
    LIBPATH += /usr/local/opt/openssl/lib
    INCLUDEPATH += /usr/local/opt/openssl/include
	INCLUDEPATH += /usr/local/include
    INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
    INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
}

android{
	# Use old boost::filesystem, because the new version requires std::wstring.
	DEFINES += BOOST_FILESYSTEM_VERSION=3
	# sqlite3 is included directly.
	INCLUDEPATH += src/third_party/sqlite3
	SOURCES += src/third_party/sqlite3/sqlite3.c
}

android_test{
	DEFINES += ANDROID
}
