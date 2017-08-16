# QMake pro-file for PokerTH common library
TEMPLATE = lib
CODECFORSRC = UTF-8
CONFIG += staticlib \
	thread \
	exceptions \
	rtti \
	stl \
	warn_on
UI_DIR = uics
TARGET = lib/pokerth_protocol
QMAKE_CLEAN += ./lib/libpokerth_protocol.a
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += ENABLE_IPV6
QT -= core \
	gui
# PRECOMPILED_HEADER = src/pch_lib.h

# Check for c++11
include(pokerth_common.pro)

INCLUDEPATH += . \
	src
DEPENDPATH += . \
	src

# Input

HEADERS += src/third_party/protobuf/pokerth.pb.h \
	src/third_party/protobuf/chatcleaner.pb.h
SOURCES += src/third_party/protobuf/pokerth.pb.cc \
	src/third_party/protobuf/chatcleaner.pb.cc
win32 {
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
	
	system(protoc pokerth.proto --cpp_out=src/third_party/protobuf)
	system(protoc chatcleaner.proto --cpp_out=src/third_party/protobuf)
	system(protoc pokerth.proto --java_out=tests/src)
}
unix : !mac {
        INCLUDEPATH += $${PREFIX}/include
	system(protoc pokerth.proto --cpp_out=src/third_party/protobuf)
	system(protoc chatcleaner.proto --cpp_out=src/third_party/protobuf)
	system(protoc pokerth.proto --java_out=tests/src)
	
	android {
		system(wine protoc.exe pokerth.proto --cpp_out=src/third_party/protobuf)
		system(wine protoc.exe chatcleaner.proto --cpp_out=src/third_party/protobuf)
		system(wine protoc.exe pokerth.proto --java_out=tests/src)
	}
}
mac { 
        # make it x86_64 only
        CONFIG += x86_64
        CONFIG -= x86
        CONFIG -= ppc
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	# QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
	INCLUDEPATH += /Developer/SDKs/MacOSX10.6.sdk/usr/include/
	INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
	INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
	INCLUDEPATH += /usr/local/include
}
