# QMake pro-file for PokerTH official server DB backend
TEMPLATE = lib
CODECFORSRC = UTF-8
CONFIG += staticlib \
    thread \
    exceptions \
    rtti \
    stl \
    warn_on
UI_DIR = uics
TARGET = lib/pokerth_dbofficial
MOC_DIR = mocs
OBJECTS_DIR = obj
QT -= core \
    gui

# Check for c++11
include(pokerth_common.pro)

# PRECOMPILED_HEADER = dbofficial/pch_lib.h
INCLUDEPATH += . \
    ./src
DEPENDPATH += . \
    ./src

# Input
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
win32 { 
    DEFINES += _WIN32_WINNT=0x0501
	INCLUDEPATH += ../../../boost/ \
		../../../GnuTLS/include \
		../../../curl/include \
		../../../mysql/include \
		../../../mysql++/lib \
		../../../zlib
}
!win32 { 
    # #### My release static build options
    # QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
    INCLUDEPATH += /usr/include \
        /usr/include/mysql \
        /usr/include/mysql++
    INCLUDEPATH += /opt/boost/include
}
mac { 
    # make it universal
    CONFIG += x86
    CONFIG -= ppc
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
    
    # for universal-compilation on PPC-Mac uncomment the following line
    # on Intel-Mac you have to comment this line out or build will fail.
    # QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
    INCLUDEPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/include/
    INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
    INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
    INCLUDEPATH += /opt/local/include/mysql++ \
        /opt/local/include/mysql5/mysql
}
