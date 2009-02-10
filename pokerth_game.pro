# QMake pro-file for PokerTH game

isEmpty( PREFIX ){
    PREFIX =/usr
}

TEMPLATE = app
CODECFORSRC = UTF-8

CONFIG += qt thread embed_manifest_exe exceptions rtti stl warn_on release
#CONFIG += qt thread embed_manifest_exe exceptions rtti stl warn_on debug

#####Uncomment this for RELEASE
#QTPLUGIN += qjpeg qgif

UI_DIR = uics
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += ENABLE_IPV6
DEFINES += PREFIX=\"$${PREFIX}\"
TARGET = pokerth

RESOURCES = src/gui/qt/resources/pokerth.qrc

INCLUDEPATH += . \
		src \
		src/engine \
		src/gui \
		src/net \
		src/engine/local_engine \
		src/engine/network_engine \
		src/config \
		src/third_party/tinyxml \
		src/gui/qt \
		src/gui/qt/connecttoserverdialog \
		src/core \
		src/gui/qt/sound \
		src/gui/qt/qttools \
		src/gui/qt/chattools \
		src/gui/qt/qttools/qthelper \
		src/gui/qt/gametable \
		src/gui/qt/gametable/startsplash \
		src/gui/qt/gametable/log \
		src/gui/qt/gametable/chat \
		src/gui/qt/aboutpokerth \
		src/gui/qt/createnetworkgamedialog \
		src/gui/qt/createinternetgamedialog \
		src/gui/qt/joinnetworkgamedialog \
		src/gui/qt/newlocalgamedialog \
		src/gui/qt/settingsdialog \
		src/gui/qt/settingsdialog/selectavatardialog \
		src/gui/qt/settingsdialog/manualblindsorderdialog \
		src/gui/qt/startnetworkgamedialog \
		src/gui/qt/startwindow \
		src/gui/qt/changehumanplayernamedialog \
		src/gui/qt/changecompleteblindsdialog \
		src/gui/qt/mymessagedialog \
		src/gui/qt/gamelobbydialog \
		src/gui/qt/gamelobbydialog/lobbychat \
		src/gui/qt/timeoutmsgbox

DEPENDPATH += . \
		src \
		src/config \
		src/core \
		src/engine \
		src/gui \
		src/net \
		src/core/common \
		src/third_party/tinyxml \
		src/engine/local_engine \
		src/engine/network_engine \
		src/gui/qt \
		src/net/common \
		src/gui/qt/sound \
		src/gui/qt/chattools \
		src/gui/qt/gametable \
		src/gui/qt/gametable/startsplash \
		src/gui/qt/gametable/log \
		src/gui/qt/gametable/chat \
		src/gui/qt/aboutpokerth \
		src/gui/qt/connecttoserverdialog \
		src/gui/qt/createinternetgamedialog \
		src/gui/qt/createnetworkgamedialog \
		src/gui/qt/joinnetworkgamedialog \
		src/gui/qt/qttools \
		src/gui/qt/qttools/qthelper \
		src/gui/qt/newlocalgamedialog \
		src/gui/qt/settingsdialog \
		src/gui/qt/settingsdialog/selectavatardialog \
		src/gui/qt/settingsdialog/manualblindsorderdialog \
		src/gui/qt/startnetworkgamedialog \
		src/gui/qt/startwindow \
		src/gui/qt/changehumanplayernamedialog \
		src/gui/qt/changecompleteblindsdialog \
		src/gui/qt/mymessagedialog \
		src/gui/qt/gamelobbydialog \
		src/gui/qt/gamelobbydialog/lobbychat \
		src/gui/qt/timeoutmsgbox

# Input
HEADERS += \
		src/game.h \
		src/session.h \
		src/playerdata.h \
		src/gamedata.h \
		src/config/configfile.h \
		src/core/thread.h \
		src/core/loghelper.h \
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
		src/gui/qt/chattools/chattools.h \
		src/gui/qt/sound/sdlplayer.h \
		src/gui/qt/gametable/gametableimpl.h \
		src/gui/qt/gametable/mycardspixmaplabel.h \
		src/gui/qt/gametable/mysetlabel.h \
		src/gui/qt/gametable/mystatuslabel.h \
		src/gui/qt/gametable/myavatarlabel.h \
		src/gui/qt/gametable/myrighttabwidget.h \
		src/gui/qt/gametable/mylefttabwidget.h \
		src/gui/qt/gametable/startsplash/startsplash.h \
		src/gui/qt/gametable/log/log.h \
		src/gui/qt/gametable/chat/chat.h \
		src/gui/qt/guiwrapper.h \
		src/gui/qt/aboutpokerth/aboutpokerthimpl.h \
		src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.h \
		src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.h \
		src/gui/qt/createinternetgamedialog/createinternetgamedialogimpl.h \
		src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.h \
		src/gui/qt/newlocalgamedialog/newgamedialogimpl.h \
		src/gui/qt/settingsdialog/settingsdialogimpl.h \
		src/gui/qt/settingsdialog/myavatarbutton.h \
		src/gui/qt/settingsdialog/myhpavatarbutton.h \
		src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.h \
		src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.h \
		src/gui/qt/settingsdialog/manualblindsorderdialog/manualblindsorderdialogimpl.h \
		src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.h \
		src/gui/qt/startwindow/startwindowimpl.h \
		src/gui/qt/changehumanplayernamedialog/changehumanplayernamedialogimpl.h \
		src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.h \
		src/gui/qt/gamelobbydialog/gamelobbydialogimpl.h \
		src/gui/qt/gamelobbydialog/mygamelisttreewidget.h \
		src/gui/qt/gamelobbydialog/lobbychat/lobbychat.h \
		src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.h \
		src/gui/qt/mymessagedialog/mymessagedialogimpl.h \
		src/gui/qttoolsinterface.h \
		src/gui/qt/qttools/qttoolswrapper.h \
		src/gui/qt/qttools/qthelper/qthelper.h \
		src/gui/generic/serverguiwrapper.h \
 src/gui/qt/gametable/mychancelabel.h

FORMS += \
		src/gui/qt/gametable.ui \
		src/gui/qt/aboutpokerth.ui \
		src/gui/qt/connecttoserverdialog.ui \
		src/gui/qt/createnetworkgamedialog.ui \
		src/gui/qt/createinternetgamedialog.ui \
		src/gui/qt/joinnetworkgamedialog.ui \
		src/gui/qt/newgamedialog.ui \
		src/gui/qt/settingsdialog.ui \
		src/gui/qt/selectavatardialog.ui \
		src/gui/qt/startnetworkgamedialog.ui \
		src/gui/qt/startwindow.ui \
		src/gui/qt/changehumanplayernamedialog.ui \
		src/gui/qt/changecompleteblindsdialog.ui \
		src/gui/qt/gamelobbydialog.ui \
		src/gui/qt/mymessagedialog.ui \
		src/gui/qt/manualblindsorderdialog.ui

SOURCES += \
		src/pokerth.cpp \
		src/gui/qt/chattools/chattools.cpp \
		src/gui/qt/sound/sdlplayer.cpp \
		src/gui/qt/guiwrapper.cpp \
		src/gui/qt/qttools/qttoolswrapper.cpp \
		src/gui/qt/qttools/qthelper/qthelper.cpp \
		src/gui/qt/gametable/gametableimpl.cpp \
		src/gui/qt/gametable/mycardspixmaplabel.cpp \
		src/gui/qt/gametable/mysetlabel.cpp \
		src/gui/qt/gametable/mystatuslabel.cpp \
		src/gui/qt/gametable/myavatarlabel.cpp \
		src/gui/qt/gametable/myrighttabwidget.cpp \
		src/gui/qt/gametable/mylefttabwidget.cpp \
		src/gui/qt/gametable/startsplash/startsplash.cpp \
		src/gui/qt/gametable/log/log.cpp \
		src/gui/qt/gametable/chat/chat.cpp \
		src/gui/qt/aboutpokerth/aboutpokerthimpl.cpp \
		src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.cpp \
		src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.cpp \
		src/gui/qt/createinternetgamedialog/createinternetgamedialogimpl.cpp \
		src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.cpp \
		src/gui/qt/newlocalgamedialog/newgamedialogimpl.cpp \
		src/gui/qt/settingsdialog/settingsdialogimpl.cpp \
		src/gui/qt/settingsdialog/myavatarbutton.cpp \
		src/gui/qt/settingsdialog/myhpavatarbutton.cpp \
		src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.cpp \
		src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.cpp \
		src/gui/qt/settingsdialog/manualblindsorderdialog/manualblindsorderdialogimpl.cpp \
		src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.cpp \
		src/gui/qt/startwindow/startwindowimpl.cpp \
		src/gui/qt/changehumanplayernamedialog/changehumanplayernamedialogimpl.cpp \
		src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.cpp \
		src/gui/qt/mymessagedialog/mymessagedialogimpl.cpp \
		src/gui/qt/gamelobbydialog/gamelobbydialogimpl.cpp \
		src/gui/qt/gamelobbydialog/mygamelisttreewidget.cpp \
		src/gui/qt/gamelobbydialog/lobbychat/lobbychat.cpp \
		src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.cpp \
		src/net/common/net_helper_client.cpp \
		src/core/common/loghelper_client.cpp \
 src/gui/qt/gametable/mychancelabel.cpp

TRANSLATIONS = \
		ts/pokerth_bg.ts \
		ts/pokerth_de.ts \
		ts/pokerth_dk.ts \
		ts/pokerth_es.ts \
		ts/pokerth_fr.ts \
		ts/pokerth_hu.ts \
		ts/pokerth_it.ts \
		ts/pokerth_nl.ts \
		ts/pokerth_no.ts \
		ts/pokerth_pl.ts \
		ts/pokerth_ru.ts \
		ts/pokerth_sk.ts \
		ts/pokerth_sv.ts \
		ts/pokerth_tr.ts \
		ts/pokerth_gr.ts \
		ts/pokerth_ptbr.ts \
		ts/pokerth_zhcn.ts \
		ts/pokerth_fi.ts \
		ts/pokerth_START_HERE.ts

win32 {
    DEFINES += CURL_STATICLIB
    DEPENDPATH += src/net/win32/ src/core/win32
    INCLUDEPATH += ../boost/ ../SDL/include ../SDL_mixer
    INCLUDEPATH += ../SDL/include/SDL ../SDL_mixer/include ../GnuTLS/include  ../curl/include ../zlib
    LIBPATH += ../boost/stage/lib ../GnuTLS/lib ../curl/lib ../zlib

    LIBS += -lpokerth_lib

    win32-msvc2005 {
        LIBPATH += Release/lib ../SDL/VisualC/SDL/Release ../SDL/VisualC/SDLmain/Release ../SDL_mixer/VisualC/Release
        #LIBPATH += Debug/lib ../SDL/VisualC/SDL/Debug ../SDL/VisualC/SDLmain/Debug ../SDL_mixer/VisualC/Debug

        LIBS += -llibgnutls-openssl -llibgcrypt
        LIBS += -llibcurl
    }

    win32-g++ {
        LIBPATH += Release/lib
        #LIBPATH += Debug/lib
        LIBPATH += ../SDL/lib ../SDL_mixer/lib
        LIBS += -lgnutls-openssl -lgnutls -lgcrypt -ltasn1 -lgpg-error
        LIBS += -lcurl
        LIBS += -lz
        LIBS += -llibboost_thread-mgw34-mt-1_37
        LIBS += -llibboost_filesystem-mgw34-mt-1_37
        LIBS += -llibboost_system-mgw34-mt-1_37
        LIBS += -llibboost_iostreams-mgw34-mt-1_37
        LIBS += -llibboost_zlib-mgw34-mt-1_37
    }

    LIBS += -lgdi32 -lcomdlg32 -loleaut32 -limm32 -lwinmm -lwinspool -lole32 -luuid -luser32 -lmsimg32 -lshell32 -lkernel32 -lws2_32 -ladvapi32 -lsdl -lsdlmain -lsdl_mixer -lwldap32
    RC_FILE = pokerth.rc
}
!win32 {
    DEPENDPATH += src/net/linux/ src/core/linux
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
                for(lib, BOOST_IOSTREAMS){
                    exists($${dir}/lib$${lib}.so*){
                        message("Found $$lib")
                        BOOST_IOSTREAMS = -l$$lib
                    }
                }
            }
        }
        BOOST_LIBS = $$BOOST_THREAD $$BOOST_FS $$BOOST_IOSTREAMS
        !count(BOOST_LIBS, 3){
            error("Unable to find boost libraries in PREFIX=$${PREFIX}")
        }

        if($$system(sdl-config --version)){
            error("sdl-config not found in PATH - libSDL_mixer, libSDL are required!")
        }

        UNAME = $$system(uname -s)
        BSD = $$find(UNAME, "BSD")

        LIBS += -lpokerth_lib
        LIBS += $$BOOST_LIBS
        LIBS += -lSDL_mixer -lcurl
        !isEmpty( BSD ){
            LIBS += -lcrypto
        }        else {
            LIBS += -lgnutls-openssl -lgcrypt
        }
        TARGETDEPS += ./lib/libpokerth_lib.a

        ##### My release static libs
        #LIBS += -lgcrypt_static -lgpg-error_static -lgnutls-openssl_static -lgnutls_static -lSDL_mixer_static -lSDL -lmikmod -lcurl

        #### INSTALL ####

        binary.path += $${PREFIX}/bin/
        binary.files += pokerth

        data.path += $${PREFIX}/share/pokerth/data/
        data.files += data/* 

        pixmap.path += $${PREFIX}/share/pixmaps/
        pixmap.files +=	pokerth.png

        desktop.path += $${PREFIX}/share/applications/
        desktop.files += pokerth.desktop

        INSTALLS += binary data pixmap desktop
    }

mac {
    # make it universal  
    CONFIG += x86 
    CONFIG += ppc
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3

    # for universal-compilation on PPC-Mac uncomment the following line
    # on Intel-Mac you have to comment this line out or build will fail.
    #	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/

    LIBPATH += lib
    LIBS += -lpokerth_lib
    # Qt static (path is standard for self-compiling qt)
    #LIBS += /usr/local/Trolltech/Qt-4.2.3/lib/libQtCore.a
    #LIBS += /usr/local/Trolltech/Qt-4.2.3/lib/libQtGui.a
    # QT dynamic linked framework (see also mac_post_make.sh)
    LIBS += -framework QtCore
    LIBS += -framework QtGui
    # SDL and SDL_mixer come as frameworks
    LIBS += -framework SDL
    LIBS += -framework SDL_mixer
    # standard path for darwinports
    # make sure you have a universal version of boost
    LIBS += /usr/local/lib/libboost_thread-mt-1_35.a
    LIBS += /usr/local/lib/libboost_filesystem-mt-1_35.a
    LIBS += /usr/local/lib/libboost_system-mt-1_35.a
    LIBS += /usr/local/lib/libboost_iostreams-mt-1_35.a
    # libraries installed on every mac
    LIBS += -lcrypto -lssl -lz -lcurl -framework Carbon
    # set the application icon
    RC_FILE = pokerth.icns
    LIBPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/lib 
    INCLUDEPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/include/
    INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
    INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
}
