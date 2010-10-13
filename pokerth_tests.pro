# QMake pro-file for PokerTH tests

isEmpty( PREFIX ){
	PREFIX =/usr
}

TEMPLATE = app
CODECFORSRC = UTF-8

CONFIG += thread console embed_manifest_exe exceptions rtti stl warn_on

UI_DIR = uics
TARGET = bin/pokerth_tests
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += PREFIX=\"$${PREFIX}\"
QT -= core gui
#PRECOMPILED_HEADER = src/pch_lib.h

INCLUDEPATH += . \
		src \
		src/tests \
		src/third_party/asn1

DEPENDPATH += . \
		src \
		src/tests \
		src/third_party/asn1

# Input
HEADERS += \
		src/net/net_packet.h
SOURCES += \
		src/tests/pokerth_tests.cpp

LIBS += -lpokerth_lib -lpokerth_protocol

win32 {
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
	DEPENDPATH += src/net/win32/ src/core/win32
	INCLUDEPATH += ../boost/ ../GnuTLS/include ../gsasl/include

	LIBPATH += ../boost/stage/lib ../GnuTLS/lib ../openssl/lib ../gsasl/lib ../curl/lib ../mysql/lib ../zlib


	debug:LIBPATH += Debug/lib
	release:LIBPATH += Release/lib

	win32-msvc2008 {
		LIBS += -llibgnutls-openssl-26 \
			-llibgcrypt-11 \
			-llibgsasl-7 \
			-llibcurl
	}

	win32-g++ {
		pkth_win64 {
			LIBS += -lcrypto -lssl -llibeay32 -lssleay32 -lgsasl
		}
		!pkth_win64 {
			LIBS += -lgnutls-openssl -lgnutls -lgcrypt -ltasn1 -lgpg-error -lgsasl -lidn
		}
		LIBS += -lcurl
		LIBS += -lz
		LIBS += -lboost_thread-mgw45-mt-1_44.dll
		LIBS += -lboost_filesystem-mgw45-mt-1_44.dll
		LIBS += -lboost_regex-mgw45-mt-1_44
		LIBS += -lboost_system-mgw45-mt-1_44.dll
		LIBS += -lboost_iostreams-mgw45-mt-1_44.dll
		LIBS += -lboost_zlib-mgw45-mt-1_44.dll
		LIBS += -lboost_program_options-mgw45-mt-1_44.dll
	}

	LIBS += -lgdi32 \
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

	LIBS += $$BOOST_LIBS
	LIBS += -lcurl -lgsasl
	!isEmpty( BSD ): isEmpty( kFreeBSD ){
		LIBS += -lcrypto -liconv
	} else {
		LIBS += -lgnutls-openssl -lgcrypt
	}

	TARGETDEPS += ./lib/libpokerth_lib.a ./lib/libpokerth_protocol.a

	#### INSTALL ####

	binary.path += $${PREFIX}/bin/
	binary.files += pokerth_tests

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

