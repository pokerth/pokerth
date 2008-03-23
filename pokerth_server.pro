# QMake pro-file for the PokerTH dedicated server

isEmpty( PREFIX ) {
    PREFIX=/usr
}

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
DEFINES += PREFIX=\"$${PREFIX}\"
QT -= core gui

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
		src/third_party/tinyxml \
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
		src/third_party/tinyxml \
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
		src/third_party/tinyxml/tinystr.h \
		src/third_party/tinyxml/tinyxml.h \
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

unix {
	# workaround for problems with boost_filesystem exceptions
	QMAKE_LFLAGS += -no_dead_strip_inits_and_terms
}

unix : !mac {

	LIBPATH += lib

	LIB_DIRS = $${PREFIX}/lib $${PREFIX}/lib64
	BOOST_FS = boost_filesystem boost_filesystem-mt
	BOOST_THREAD = boost_thread boost_thread-mt
	BOOST_PROGRAM_OPTIONS = boost_program_options boost_program_options-mt

	for(dir, LIB_DIRS) {
		exists($$dir) {
			for(lib, BOOST_THREAD) {
				exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_THREAD = -l$$lib
				}
			}
			for(lib, BOOST_FS) {
				exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_FS = -l$$lib
				}
			}
			for(lib, BOOST_PROGRAM_OPTIONS) {
				exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_PROGRAM_OPTIONS = -l$$lib
				}
			}
 		}
 	}
	BOOST_LIBS = $$BOOST_THREAD $$BOOST_FS $$BOOST_PROGRAM_OPTIONS
	!count(BOOST_LIBS, 3) {
		error("could not locate required library: \
		    libboost (version >= 1.34.1)  --> http://www.boost.org/")
	}

	LIBS += -lpokerth_lib
	LIBS += $$BOOST_LIBS
	LIBS += -lcrypto

	TARGETDEPS += ./lib/libpokerth_lib.a

	#### INSTALL ####

	binary.path += $${PREFIX}/bin/
	binary.files += pokerth_server

	INSTALLS += binary
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
