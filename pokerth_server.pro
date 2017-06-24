# QMake pro-file for the PokerTH dedicated server

isEmpty( PREFIX ){
	PREFIX =/usr
}

TEMPLATE = app
CODECFORSRC = UTF-8

CONFIG += thread console embed_manifest_exe exceptions rtti stl warn_on

UI_DIR = uics
TARGET = bin/pokerth_server
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += POKERTH_DEDICATED_SERVER
DEFINES += ENABLE_IPV6 TIXML_USE_STL BOOST_FILESYSTEM_DEPRECATED
DEFINES += PREFIX=\"$${PREFIX}\"
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
		src/gui/qt/qttools/nonqthelper \
		src/net \
		src/engine/local_engine \
		src/engine/network_engine \
		src/config \
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
		src/engine/local_engine \
		src/engine/network_engine \
		src/net/common \

# Input
HEADERS += \
		src/engine/game.h \
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
		src/net/senderhelper.h \
		src/net/serveraccepthelper.h \
		src/net/serverlobbythread.h \
		src/net/socket_helper.h \
		src/net/socket_msg.h \
		src/net/socket_startup.h \
		src/net/net_helper.h \
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
		src/gui/generic/serverguiwrapper.h \
		src/net/servermanagerirc.h

SOURCES += \
		src/pokerth_server.cpp \
		src/gui/qt/qttools/nonqttoolswrapper.cpp \
		src/gui/qt/qttools/nonqthelper/nonqthelper.cpp \
		src/net/common/net_helper_server.cpp \
		src/core/common/loghelper_server.cpp \
		src/net/common/ircthread.cpp \
		src/net/common/servermanagerirc.cpp \
		src/net/common/servermanagerfactoryserver.cpp

LIBS += -lpokerth_lib \
	-lpokerth_db \
	-lpokerth_protocol \
	-lcurl \
	-lircclient

win32 {
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
	DEFINES += HAVE_OPENSSL
	DEPENDPATH += src/net/win32/ src/core/win32
	INCLUDEPATH += ../sqlite ../boost/ ../openssl/include ../gsasl/include

	SOURCES += src/core/win32/convhelper.cpp

	LIBPATH += ../boost/stage/lib ../openssl/lib ../gsasl/lib ../curl/lib ../mysql/lib ../zlib

	debug:LIBPATH += debug/lib
	release:LIBPATH += release/lib

	LIBS += -lssl -lcrypto -lssh2 -lgnutls -lhogweed -lgmp -lgcrypt -lgpg-error -lgsasl -lnettle -lidn -lintl -lprotobuf -ltinyxml -lsqlite3 -lntlm
	LIBS += -lboost_thread_win32-mt
	LIBS += -lboost_filesystem-mt
	LIBS += -lboost_regex-mt
	LIBS += -lboost_program_options-mt
	LIBS += -lboost_iostreams-mt
	LIBS += -lboost_random-mt
	LIBS += -lboost_chrono-mt
	LIBS += -lboost_system-mt

	LIBS += -liconv \
			-lz \
			-lgdi32 \
			-lcomdlg32 \
			-loleaut32 \
			-limm32 \
			-lwinmm \
			-lwinspool \
			-lole32 \
			-luuid \
			-luser32 \
			-lmsimg32 \
			-lshell32 \
			-lkernel32 \
			-lmswsock \
			-lws2_32 \
			-ladvapi32 \
			-lwldap32 \
			-lcrypt32
}

!win32 {
	DEPENDPATH += src/net/linux/ src/core/linux
	SOURCES +=
	SOURCES += src/core/linux/convhelper.cpp
}

unix : !mac {

	##### My release static build options
	#QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
	#QMAKE_LFLAGS += -Wl,--gc-sections

	LIBPATH += lib $${PREFIX}/lib /opt/gsasl/lib
	INCLUDEPATH += $${PREFIX}/include
	# see issue https://github.com/pokerth/pokerth/issues/282
	INCLUDEPATH += $${PREFIX}/include/libircclient
	LIB_DIRS = $${PREFIX}/lib $${PREFIX}/lib64 $$system(qmake -query QT_INSTALL_LIBS)
	BOOST_FS = boost_filesystem boost_filesystem-mt
	BOOST_THREAD = boost_thread boost_thread-mt
	BOOST_PROGRAM_OPTIONS = boost_program_options boost_program_options-mt
	BOOST_IOSTREAMS = boost_iostreams boost_iostreams-mt
	BOOST_SYS = boost_system boost_system-mt
	BOOST_REGEX = boost_regex boost_regex-mt
	BOOST_RANDOM = boost_random boost_random-mt
	BOOST_CHRONO = boost_chrono boost_chrono-mt

	#
	# searching in $PREFIX/lib, $PREFIX/lib64 and $$system(qmake -query QT_INSTALL_LIBS)
	# to override the default '/usr' pass PREFIX
	# variable to qmake.
	#
	for(dir, LIB_DIRS){
		exists($$dir){
			for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_THREAD = -l$$lib
			}
			for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_THREAD = -l$$lib
			}
			for(lib, BOOST_FS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_FS = -l$$lib
			}
			for(lib, BOOST_FS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_FS = -l$$lib
			}
			for(lib, BOOST_IOSTREAMS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_IOSTREAMS = -l$$lib
			}
			for(lib, BOOST_IOSTREAMS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_IOSTREAMS = -l$$lib
			}
			for(lib, BOOST_PROGRAM_OPTIONS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_PROGRAM_OPTIONS = -l$$lib
			}
			for(lib, BOOST_PROGRAM_OPTIONS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_PROGRAM_OPTIONS = -l$$lib
			}
			for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_REGEX = -l$$lib
			}
			for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_REGEX = -l$$lib
			}
			for(lib, BOOST_RANDOM):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_RANDOM = -l$$lib
			}
			for(lib, BOOST_RANDOM):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_RANDOM = -l$$lib
			}
			for(lib, BOOST_SYS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_SYS = -l$$lib
			}
			for(lib, BOOST_SYS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_SYS = -l$$lib
			}
			!c++11 { 
				for(lib, BOOST_CHRONO):exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_CHRONO = -l$$lib
				}
				for(lib, BOOST_CHRONO):exists($${dir}/lib$${lib}.a) {
					message("Found $$lib")
					BOOST_CHRONO = -l$$lib
				}
			}
		}
	}
	c++11 { 
		BOOST_LIBS = $$BOOST_THREAD $$BOOST_FS $$BOOST_PROGRAM_OPTIONS $$BOOST_IOSTREAMS $$BOOST_REGEX $$BOOST_RANDOM $$BOOST_SYS
		!count(BOOST_LIBS, 7){
			error("Unable to find boost libraries in PREFIX=$${PREFIX}")
		}
	}
	!c++11 { 
		BOOST_LIBS = $$BOOST_THREAD $$BOOST_FS $$BOOST_PROGRAM_OPTIONS $$BOOST_IOSTREAMS $$BOOST_REGEX $$BOOST_RANDOM $$BOOST_SYS $$BOOST_CHRONO
		!count(BOOST_LIBS, 8){
			error("Unable to find boost libraries in PREFIX=$${PREFIX}")
		}
	}

	UNAME = $$system(uname -s)
	BSD = $$find(UNAME, "BSD")
	kFreeBSD = $$find(UNAME, "kFreeBSD")

	LIBS += $$BOOST_LIBS
	LIBS += -lsqlite3 \
			-ltinyxml \
			-lprotobuf
	LIBS += -lgsasl
	!isEmpty( BSD ): isEmpty( kFreeBSD ){
		LIBS += -lcrypto -liconv
	} else {
		LIBS += -lgcrypt
	}

	TARGETDEPS += ./lib/libpokerth_lib.a \
				  ./lib/libpokerth_db.a \
				  ./lib/libpokerth_protocol.a

	#### INSTALL ####

	binary.path += $${PREFIX}/bin/
	binary.files += pokerth_server

	INSTALLS += binary
}

mac {
	# make it x86_64 only
	CONFIG += x86_64
	CONFIG -= x86
	CONFIG -= ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
	QMAKE_CXXFLAGS -= -std=gnu++0x

	# workaround for problems with boost_filesystem exceptions
	QMAKE_LFLAGS += -no_dead_strip_inits_and_terms

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	#       QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/

	LIBPATH += lib
	# make sure you have an x86_64 version of boost
	LIBS += /usr/local/lib/libboost_thread-mt.a
	LIBS += /usr/local/lib/libboost_filesystem.a
	LIBS += /usr/local/lib/libboost_regex.a
	LIBS += /usr/local/lib/libboost_random.a
	LIBS += /usr/local/lib/libboost_system.a
	LIBS += /usr/local/lib/libboost_iostreams.a
	LIBS += /usr/local/lib/libboost_program_options.a
	LIBS += /usr/local/lib/libgsasl.a

	# libraries installed on every mac
	LIBS += -lsqlite3
	LIBS += -ltinyxml
	LIBS += -lcrypto -lssl -lz -liconv
	# set the application icon
	RC_FILE = pokerth.icns
	LIBPATH += /Developer/SDKs/MacOSX10.6.sdk/usr/lib
	INCLUDEPATH += /Developer/SDKs/MacOSX10.6.sdk/usr/include/
	INCLUDEPATH += /usr/local/include
}

official_server {
	LIBPATH += pkth_stat/daemon_lib/lib
	LIBS += -lpokerth_dbofficial -lmysqlpp
	DEFINES += POKERTH_OFFICIAL_SERVER
}

android_test{
	DEFINES += ANDROID
}
