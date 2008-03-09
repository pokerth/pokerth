# QMake pro-file for the PokerTH dedicated server

TEMPLATE = app
CODECFORSRC = UTF-8

CONFIG += thread console embed_manifest_exe exceptions rtti stl warn_on release
#CONFIG += thread console embed_manifest_exe exceptions rtti stl warn_on debug

UI_DIR = uics
TARGET = bin/pokerth_server
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += POKERTH_DEDICATED_SERVER
DEFINES += ENABLE_IPV6
QT -= core gui

INSTALLS += TARGET
TARGET.files += bin/*
TARGET.path = /usr/bin/

INCLUDEPATH += . \
		src \
		src/engine \
		src/gui \
		src/gui/qt \
		src/gui/qt/qttools \
		src/gui/qt/qttools/nonqthelper \
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
		src/net/net_helper.h \
		src/core/tinyxml/tinystr.h \
		src/core/tinyxml/tinyxml.h \
		src/core/pokerthexception.h \
		src/core/convhelper.h \
		src/core/loghelper.h \
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
		src/gui/qt/qttools/nonqttoolswrapper.h \
		src/gui/qt/qttools/nonqthelper/nonqthelper.h \
		src/gui/generic/serverguiwrapper.h

SOURCES += \
		src/pokerth_server.cpp \
		src/gui/qt/qttools/nonqttoolswrapper.cpp \
		src/gui/qt/qttools/nonqthelper/nonqthelper.cpp \
		src/net/common/net_helper_server.cpp \
		src/core/common/loghelper_server.cpp

win32 {
    DEPENDPATH += src/net/win32/ src/core/win32
    INCLUDEPATH += ../boost/ ../OpenSSL/include

    SOURCES += src/core/win32/convhelper.cpp

    LIBPATH += ../boost/stage/lib ../OpenSSL/lib

    LIBPATH += Release/lib
    #LIBPATH += Debug/lib

    LIBS += -lpokerth_lib

	win32-msvc2005{
		LIBS += -llibeay32
		LIBS += -lssleay32
	}

	win32-g++{
		LIBS += -lcrypto
		LIBS += -lssl
		LIBS += -lboost_thread-mgw34-mt-1_34_1
		LIBS += -lboost_filesystem-mgw34-mt-1_34_1
		LIBS += -lboost_program_options-mgw34-mt-1_34_1
	}

    LIBS += -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lwinmm -lwinspool -lole32 -luuid -luser32 -lmsimg32 -lshell32 -lkernel32 -lws2_32 -ladvapi32
}
!win32 {
    DEPENDPATH += src/net/linux/ src/core/linux
    SOURCES += src/core/linux/daemon.c
    SOURCES += src/core/linux/convhelper.cpp
}

unix : !mac {

	LIBPATH += lib
	LIBS += -lpokerth_lib

	exists( /usr/lib/libboost_thread-mt.so ){
		message("Found libboost_thread-mt")
		LIBS += -lboost_thread-mt
	}
	exists( /usr/lib64/libboost_thread-mt.so ){
		message("Found libboost_thread-mt")
		LIBS += -lboost_thread-mt
	}
	!exists( /usr/lib/libboost_thread-mt.so ){
		exists( /usr/lib/libboost_thread.so ){
			message("Found libboost_thread")
			LIBS += -lboost_thread
		}
	}
	!exists( /usr/lib64/libboost_thread-mt.so ){
		exists( /usr/lib64/libboost_thread.so ){
			message("Found libboost_thread")
			LIBS += -lboost_thread
		}
	}

	exists( /usr/lib/libboost_filesystem-mt.so ){
		message("Found libboost_filesystem-mt")
		LIBS += -lboost_filesystem-mt
	}
	exists( /usr/lib64/libboost_filesystem-mt.so ){
		message("Found libboost_filesystem-mt")
		LIBS += -lboost_filesystem-mt
	}
	!exists( /usr/lib/libboost_filesystem-mt.so ){
		exists( /usr/lib/libboost_filesystem.so ){
			message("Found libboost_filesystem")
			LIBS += -lboost_filesystem
		}
	}
	!exists( /usr/lib64/libboost_filesystem-mt.so ){
		exists( /usr/lib64/libboost_filesystem.so ){
			message("Found libboost_filesystem")
			LIBS += -lboost_filesystem
		}
	}

	exists( /usr/lib/libboost_program_options-mt.so ){
		message("Found libboost_program_options-mt")
		LIBS += -lboost_program_options-mt
	}
	exists( /usr/lib64/libboost_program_options-mt.so ){
		message("Found libboost_program_options-mt")
		LIBS += -lboost_program_options-mt
	}
	!exists( /usr/lib/libboost_program_options-mt.so ){
		exists( /usr/lib/libboost_program_options.so ){
			message("Found libboost_program_options")
			LIBS += -lboost_program_options
		}
	}
	!exists( /usr/lib64/liblibboost_program_options-mt.so ){
		exists( /usr/lib64/libboost_program_options.so ){
			message("Found libboost_program_options")
			LIBS += -lboost_program_options
		}
	}

	LIBS += -lcrypto
	TARGETDEPS += ./lib/libpokerth_lib.a

	## My release static libs
	#LIBS += -lcrypto
}

mac{
	# make it universal  
	CONFIG += x86 
	CONFIG += ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	#       QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/

	LIBPATH += lib
	LIBS += -lpokerth_lib
	# standard path for darwinports
	# make sure you have a universal version of boost
	LIBS += /usr/local/lib/libboost_thread-mt-1_34_1.a
	LIBS += /usr/local/lib/libboost_filesystem-mt-1_34_1.a
	LIBS += /usr/local/lib/libboost_program_options-mt-1_34_1.a
	# libraries installed on every mac
	LIBS += -lcrypto -liconv
	# set the application icon
	RC_FILE = pokerth.icns
	LIBPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/lib 
	INCLUDEPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/include/
}
