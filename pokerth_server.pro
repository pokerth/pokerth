# QMake pro-file for the PokerTH dedicated server

isEmpty( PREFIX ){
    PREFIX =/usr
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
    DEFINES += CURL_STATICLIB
    DEPENDPATH += src/net/win32/ src/core/win32
    INCLUDEPATH += ../boost/ ../GnuTLS/include

    SOURCES += src/core/win32/convhelper.cpp

    LIBPATH += ../boost/stage/lib ../GnuTLS/lib ../curl/lib ../zlib

    LIBS += -lpokerth_lib

    win32-msvc2005 {
        LIBPATH += Release/lib
        #LIBPATH += Debug/lib
        LIBS += -llibgnutls-openssl -llibgcrypt
        LIBS += -llibcurl
    }

    win32-g++ {
        debug {
            LIBPATH += Debug/lib
        }
        release {
            LIBPATH += Release/lib
        }
        LIBS += -lgnutls-openssl -lgnutls -lgcrypt -ltasn1 -lgpg-error
        LIBS += -lcurl
        LIBS += -lz
        LIBS += -llibboost_thread-mgw43-mt-1_38
        LIBS += -llibboost_filesystem-mgw43-mt-1_38
        LIBS += -llibboost_system-mgw43-mt-1_38
        LIBS += -llibboost_iostreams-mgw43-mt-1_38
        LIBS += -llibboost_zlib-mgw43-mt-1_38
        LIBS += -llibboost_program_options-mgw43-mt-1_38
    }

    LIBS += -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lwinmm -lwinspool -lole32 -luuid -luser32 -lmsimg32 -lshell32 -lkernel32 -lws2_32 -ladvapi32 -lwldap32
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

        #
        # searching in $PREFIX/lib and $PREFIX/lib64
        # to override the default '/usr' pass PREFIX
        # variable to qmake.
        #
        for(dir, LIB_DIRS){
            exists($$dir){
                for(lib, BOOST_THREAD){
                    exists($${dir}/lib$${lib}.so*){
                        message("Found $$lib")
                        BOOST_THREAD = -l$$lib
                    }
                }
                for(lib, BOOST_FS){
                    exists($${dir}/lib$${lib}.so*){
                        message("Found $$lib")
                        BOOST_FS = -l$$lib
                    }
                }
                for(lib, BOOST_PROGRAM_OPTIONS){
                    exists($${dir}/lib$${lib}.so*){
                        message("Found $$lib")
                        BOOST_PROGRAM_OPTIONS = -l$$lib
                    }
                }
                for(lib, BOOST_IOSTREAMS){
                    exists($${dir}/lib$${lib}.so*){
                        message("Found $$lib")
                        BOOST_IOSTREAMS = -l$$lib
                    }
                }
            }
        }
        BOOST_LIBS = $$BOOST_THREAD $$BOOST_FS $$BOOST_PROGRAM_OPTIONS $$BOOST_IOSTREAMS
        !count(BOOST_LIBS, 4){
            error("Unable to find boost libraries in PREFIX=$${PREFIX}")
        }

        UNAME = $$system(uname -s)
        BSD = $$find(UNAME, "BSD")
        kFreeBSD = $$find(UNAME, "kFreeBSD")

        LIBS += -lpokerth_lib
        LIBS += $$BOOST_LIBS
        LIBS += -lcurl
        !isEmpty( BSD ) && isEmpty( kFreeBSD ){
            LIBS += -lcrypto -liconv
        }        else {
            LIBS += -lgnutls-openssl -lgcrypt
        }

        TARGETDEPS += ./lib/libpokerth_lib.a

        #### INSTALL ####

        binary.path += $${PREFIX}/bin/
        binary.files += pokerth_server

        INSTALLS += binary
    }

mac {
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
    LIBS += /usr/local/lib/libboost_thread-xgcc40-mt.a
    LIBS += /usr/local/lib/libboost_filesystem-xgcc40-mt.a
    LIBS += /usr/local/lib/libboost_system-xgcc40-mt.a
    LIBS += /usr/local/lib/libboost_iostreams-xgcc40-mt.a
    LIBS += /usr/local/lib/libboost_program_options-xgcc40-mt.a
    # libraries installed on every mac
    LIBS += -lcrypto -lssl -lz -lcurl -liconv
    # set the application icon
    RC_FILE = pokerth.icns
    LIBPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/lib
    INCLUDEPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/include/
}
