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

INCLUDEPATH += . \
	src \
	src/third_party/asn1
DEPENDPATH += . \
	src \
	src/third_party/asn1

# Input
HEADERS +=	src/third_party/asn1/asn_application.h \
	src/third_party/asn1/asn_codecs.h \
	src/third_party/asn1/asn_codecs_prim.h \
	src/third_party/asn1/asn_internal.h \
	src/third_party/asn1/asn_SEQUENCE_OF.h \
	src/third_party/asn1/asn_SET_OF.h \
	src/third_party/asn1/asn_system.h \
	src/third_party/asn1/ber_decoder.h \
	src/third_party/asn1/ber_tlv_length.h \
	src/third_party/asn1/ber_tlv_tag.h \
	src/third_party/asn1/BIT_STRING.h \
	src/third_party/asn1/BOOLEAN.h \
	src/third_party/asn1/constraints.h \
	src/third_party/asn1/constr_CHOICE.h \
	src/third_party/asn1/constr_SEQUENCE.h \
	src/third_party/asn1/constr_SEQUENCE_OF.h \
	src/third_party/asn1/constr_SET_OF.h \
	src/third_party/asn1/constr_TYPE.h \
	src/third_party/asn1/der_encoder.h \
	src/third_party/asn1/INTEGER.h \
	src/third_party/asn1/NativeEnumerated.h \
	src/third_party/asn1/NativeInteger.h \
	src/third_party/asn1/OCTET_STRING.h \
	src/third_party/asn1/per_decoder.h \
	src/third_party/asn1/per_encoder.h \
	src/third_party/asn1/per_opentype.h \
	src/third_party/asn1/per_support.h \
	src/third_party/asn1/UTF8String.h \
	src/third_party/asn1/xer_decoder.h \
	src/third_party/asn1/xer_encoder.h \
	src/third_party/asn1/xer_support.h \
	src/third_party/asn1/ChatCleanerMessage.h \
	src/third_party/asn1/CleanerChatTypeLobby.h \
	src/third_party/asn1/CleanerChatTypeGame.h \
	src/third_party/asn1/CleanerChatType.h \
	src/third_party/asn1/CleanerInitMessage.h \
	src/third_party/asn1/CleanerInitAckMessage.h \
	src/third_party/asn1/CleanerChatRequestMessage.h \
	src/third_party/asn1/CleanerChatReplyMessage.h
HEADERS += src/third_party/protobuf/pokerth.pb.h
SOURCES += src/third_party/asn1/ChatCleanerMessage.c \
    src/third_party/asn1/CleanerChatTypeLobby.c \
    src/third_party/asn1/CleanerChatTypeGame.c \
    src/third_party/asn1/CleanerChatType.c \
	src/third_party/asn1/CleanerInitMessage.c \
	src/third_party/asn1/CleanerInitAckMessage.c \
	src/third_party/asn1/CleanerChatRequestMessage.c \
	src/third_party/asn1/CleanerChatReplyMessage.c \
	src/third_party/asn1/xer_support.c \
	src/third_party/asn1/xer_encoder.c \
	src/third_party/asn1/xer_decoder.c \
	src/third_party/asn1/UTF8String.c \
	src/third_party/asn1/per_support.c \
	src/third_party/asn1/per_opentype.c \
	src/third_party/asn1/per_encoder.c \
	src/third_party/asn1/per_decoder.c \
	src/third_party/asn1/OCTET_STRING.c \
	src/third_party/asn1/NativeInteger.c \
	src/third_party/asn1/NativeEnumerated.c \
	src/third_party/asn1/INTEGER.c \
	src/third_party/asn1/der_encoder.c \
	src/third_party/asn1/constraints.c \
	src/third_party/asn1/constr_TYPE.c \
	src/third_party/asn1/constr_SET_OF.c \
	src/third_party/asn1/constr_SEQUENCE_OF.c \
	src/third_party/asn1/constr_SEQUENCE.c \
	src/third_party/asn1/constr_CHOICE.c \
	src/third_party/asn1/BOOLEAN.c \
	src/third_party/asn1/BIT_STRING.c \
	src/third_party/asn1/ber_tlv_tag.c \
	src/third_party/asn1/ber_tlv_length.c \
	src/third_party/asn1/ber_decoder.c \
	src/third_party/asn1/asn_SET_OF.c \
	src/third_party/asn1/asn_SEQUENCE_OF.c \
	src/third_party/asn1/asn_codecs_prim.c
SOURCES += src/third_party/protobuf/pokerth.pb.cc
win32 { 
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
}
unix : !mac {
        INCLUDEPATH += $${PREFIX}/include
	system(protoc pokerth.proto --cpp_out=src/third_party/protobuf)
	system(protoc pokerth.proto --java_out=tests/src)
}
mac { 
        # make it x86_64 only
        CONFIG += x86_64
        CONFIG -= x86
        CONFIG -= ppc
        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6

	# for universal-compilation on PPC-Mac uncomment the following line
	# on Intel-Mac you have to comment this line out or build will fail.
	# QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
        INCLUDEPATH += /Developer/SDKs/MacOSX10.6.sdk/usr/include/
	INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
	INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
}
