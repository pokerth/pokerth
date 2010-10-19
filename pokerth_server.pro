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
DEFINES += ENABLE_IPV6
DEFINES += PREFIX=\"$${PREFIX}\"
QT -= core gui
#PRECOMPILED_HEADER = src/pch_lib.h

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
		src/third_party/asn1 \
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
		src/net/senderhelper.h \
		src/net/serveraccepthelper.h \
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
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
	DEPENDPATH += src/net/win32/ src/core/win32
	INCLUDEPATH += ../boost/ ../GnuTLS/include ../gsasl/include

	SOURCES += src/core/win32/convhelper.cpp

	LIBPATH += ../boost/stage/lib ../GnuTLS/lib ../openssl/lib ../gsasl/lib ../curl/lib ../mysql/lib ../zlib

	LIBS += -lpokerth_lib \
		-lpokerth_db \
		-lpokerth_protocol \
		-lcurl \
		-lz

	debug:LIBPATH += debug/lib
	release:LIBPATH += release/lib

	pkth_win64 {
		LIBS += -lcrypto -lssl -llibeay32 -lssleay32 -lgsasl
	}
	!pkth_win64 {
		LIBS += -lgnutls-openssl -lgnutls -lgcrypt -lgpg-error -lgsasl -lidn
	}
	win32-g++-cross {
		LIBS += -lntlm
		LIBS += -lboost_thread_win32-mt
		LIBS += -lboost_filesystem-mt
		LIBS += -lboost_regex-mt
		LIBS += -lboost_program_options-mt
		LIBS += -lboost_iostreams-mt
		LIBS += -lboost_system-mt
	}
	win32-g++ {
		!pkth_win64:LIBS += -ltasn1
		LIBS += -lboost_thread-mgw45-mt-1_44.dll
		LIBS += -lboost_filesystem-mgw45-mt-1_44.dll
		LIBS += -lboost_regex-mgw45-mt-1_44
		LIBS += -lboost_program_options-mgw45-mt-1_44.dll
		LIBS += -lboost_iostreams-mgw45-mt-1_44.dll
		LIBS += -lboost_system-mgw45-mt-1_44.dll
	}

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
			-lwldap32
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

	##### My release static build options
	#QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
	#QMAKE_LFLAGS += -Wl,--gc-sections

	LIBPATH += lib

	LIB_DIRS = $${PREFIX}/lib $${PREFIX}/lib64
	BOOST_FS = boost_filesystem boost_filesystem-mt
	BOOST_THREAD = boost_thread boost_thread-mt
	BOOST_PROGRAM_OPTIONS = boost_program_options boost_program_options-mt
	BOOST_IOSTREAMS = boost_iostreams boost_iostreams-mt
	BOOST_SYS = boost_system boost_system-mt
	BOOST_REGEX = boost_regex boost_regex-mt


	#
	# searching in $PREFIX/lib and $PREFIX/lib64
	# to override the default '/usr' pass PREFIX
	# variable to qmake.
	#
	for(dir, LIB_DIRS){
		exists($$dir){
			for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_THREAD = -l$$lib
			}
			for(lib, BOOST_THREAD):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_THREAD = -l$$lib
			}
			for(lib, BOOST_FS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_FS = -l$$lib
			}
			for(lib, BOOST_FS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_FS = -l$$lib
			}
			for(lib, BOOST_IOSTREAMS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_IOSTREAMS = -l$$lib
			}
			for(lib, BOOST_IOSTREAMS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_IOSTREAMS = -l$$lib
			}
			for(lib, BOOST_PROGRAM_OPTIONS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_PROGRAM_OPTIONS = -l$$lib
			}
			for(lib, BOOST_PROGRAM_OPTIONS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_PROGRAM_OPTIONS = -l$$lib
			}
			for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_REGEX = -l$$lib
			}
			for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_REGEX = -l$$lib
			}
			for(lib, BOOST_SYS):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_SYS = -l$$lib
			}
			for(lib, BOOST_SYS):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_SYS = -l$$lib
			}
		}
	}
	BOOST_LIBS = $$BOOST_THREAD $$BOOST_FS $$BOOST_PROGRAM_OPTIONS $$BOOST_IOSTREAMS $$BOOST_REGEX $$BOOST_SYS
	!count(BOOST_LIBS, 6){
		error("Unable to find boost libraries in PREFIX=$${PREFIX}")
	}

	UNAME = $$system(uname -s)
	BSD = $$find(UNAME, "BSD")
	kFreeBSD = $$find(UNAME, "kFreeBSD")

	LIBS += -lpokerth_lib \
			-lpokerth_db \
			-lpokerth_protocol
	LIBS += $$BOOST_LIBS
	LIBS += -lcurl -lgsasl
	!isEmpty( BSD ): isEmpty( kFreeBSD ){
		LIBS += -lcrypto -liconv
	} else {
		LIBS += -lgnutls-openssl -lgcrypt
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
	# make it universal
	CONFIG += x86
	CONFIG -= ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	#       QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/

	LIBPATH += lib
	LIBS += -lpokerth_lib -lpokerth_db -lpokerth_protocol
	# standard path for darwinports
	# make sure you have a universal version of boost
	LIBS += /usr/local/lib/libboost_thread.a
	LIBS += /usr/local/lib/libboost_filesystem.a
	LIBS += /usr/local/lib/libboost_regex.a
	LIBS += /usr/local/lib/libboost_system.a
	LIBS += /usr/local/lib/libboost_iostreams.a
	LIBS += /usr/local/lib/libboost_program_options.a
	LIBS += /usr/local/lib/libgsasl.a

	# libraries installed on every mac
	LIBS += -lcrypto -lssl -lz -lcurl -liconv
	# set the application icon
	RC_FILE = pokerth.icns
	LIBPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/lib
	INCLUDEPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/include/
}

official_server {
	LIBPATH += pkth_stat/daemon_lib/lib
	LIBS += -lpokerth_closed -lmysqlpp
	DEFINES += POKERTH_OFFICIAL_SERVER
}
