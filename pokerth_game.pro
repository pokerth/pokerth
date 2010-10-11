# QMake pro-file for PokerTH game
isEmpty( PREFIX ):PREFIX = /usr
TEMPLATE = app
CODECFORSRC = UTF-8
CONFIG += qt \
    thread \
    embed_manifest_exe \
    exceptions \
    rtti \
    stl \
    warn_on
include(src/third_party/qtsingleapplication/qtsingleapplication.pri)
QT += sql
UI_DIR = uics
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += ENABLE_IPV6
DEFINES += PREFIX=\"$${PREFIX}\"
TARGET = pokerth

# PRECOMPILED_HEADER = src/pch_game.h
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
    src/third_party/asn1 \
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
    src/gui/qt/serverlistdialog \
    src/gui/qt/styles \
    src/gui/qt/changecontentdialog \
    src/gui/qt/changecompleteblindsdialog \
    src/gui/qt/internetgamelogindialog \
    src/gui/qt/mymessagedialog \
    src/gui/qt/gamelobbydialog \
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
    src/gui/qt/serverlistdialog \
    src/gui/qt/styles \
    src/gui/qt/changecontentdialog \
    src/gui/qt/internetgamelogindialog \
    src/gui/qt/changecompleteblindsdialog \
    src/gui/qt/mymessagedialog \
    src/gui/qt/gamelobbydialog \
    src/gui/qt/timeoutmsgbox

# Input
HEADERS += src/game.h \
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
    src/net/senderhelper.h \
    src/net/serveraccepthelper.h \
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
    src/gui/qt/gametable/myactionbutton.h \
    src/gui/qt/gametable/mystatuslabel.h \
    src/gui/qt/gametable/myavatarlabel.h \
    src/gui/qt/gametable/myrighttabwidget.h \
    src/gui/qt/gametable/mylefttabwidget.h \
    src/gui/qt/gametable/startsplash/startsplash.h \
    src/gui/qt/gametable/log/log.h \
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
    src/gui/qt/styles/gametablestylereader.h \
    src/gui/qt/styles/carddeckstylereader.h \
    src/gui/qt/changecontentdialog/changecontentdialogimpl.h \
    src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.h \
    src/gui/qt/gamelobbydialog/gamelobbydialogimpl.h \
    src/gui/qt/gamelobbydialog/mygamelisttreewidget.h \
    src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.h \
    src/gui/qt/mymessagedialog/mymessagedialogimpl.h \
    src/gui/qttoolsinterface.h \
    src/gui/qt/qttools/qttoolswrapper.h \
    src/gui/qt/qttools/qthelper/qthelper.h \
    src/gui/generic/serverguiwrapper.h \
    src/gui/qt/gametable/mychancelabel.h \
    src/gui/qt/serverlistdialog/serverlistdialogimpl.h \
    src/gui/qt/gametable/mymenubar.h \
    src/gui/qt/gametable/mytimeoutlabel.h \
    src/gui/qt/gametable/mynamelabel.h \
    src/gui/qt/settingsdialog/mystylelistitem.h \
    src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.h \
    src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.h \
    src/engine/local_engine/replay.h \
    src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.h \
    src/gui/qt/gametable/myslider.h
FORMS += src/gui/qt/gametable.ui \
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
    src/gui/qt/changecompleteblindsdialog.ui \
    src/gui/qt/gamelobbydialog.ui \
    src/gui/qt/mymessagedialog.ui \
    src/gui/qt/manualblindsorderdialog.ui \
    src/gui/qt/serverlistdialog.ui \
    src/gui/qt/internetgamelogindialog.ui \
    src/gui/qt/changecontentdialog.ui
SOURCES += src/pokerth.cpp \
    src/gui/qt/chattools/chattools.cpp \
    src/gui/qt/sound/sdlplayer.cpp \
    src/gui/qt/guiwrapper.cpp \
    src/gui/qt/qttools/qttoolswrapper.cpp \
    src/gui/qt/qttools/qthelper/qthelper.cpp \
    src/gui/qt/gametable/gametableimpl.cpp \
    src/gui/qt/gametable/mycardspixmaplabel.cpp \
    src/gui/qt/gametable/mysetlabel.cpp \
    src/gui/qt/gametable/myactionbutton.cpp \
    src/gui/qt/gametable/mystatuslabel.cpp \
    src/gui/qt/gametable/myavatarlabel.cpp \
    src/gui/qt/gametable/myrighttabwidget.cpp \
    src/gui/qt/gametable/mylefttabwidget.cpp \
    src/gui/qt/gametable/startsplash/startsplash.cpp \
    src/gui/qt/gametable/log/log.cpp \
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
    src/gui/qt/styles/gametablestylereader.cpp \
    src/gui/qt/styles/carddeckstylereader.cpp \
    src/gui/qt/changecontentdialog/changecontentdialogimpl.cpp \
    src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.cpp \
    src/gui/qt/mymessagedialog/mymessagedialogimpl.cpp \
    src/gui/qt/gamelobbydialog/gamelobbydialogimpl.cpp \
    src/gui/qt/gamelobbydialog/mygamelisttreewidget.cpp \
    src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.cpp \
    src/net/common/net_helper_client.cpp \
    src/core/common/loghelper_client.cpp \
    src/gui/qt/gametable/mychancelabel.cpp \
    src/gui/qt/serverlistdialog/serverlistdialogimpl.cpp \
    src/gui/qt/gametable/mymenubar.cpp \
    src/gui/qt/gametable/mytimeoutlabel.cpp \
    src/gui/qt/gametable/mynamelabel.cpp \
    src/gui/qt/settingsdialog/mystylelistitem.cpp \
    src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.cpp \
    src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.cpp \
    src/engine/local_engine/replay.cpp \
    src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.cpp
TRANSLATIONS = ts/pokerth_af.ts \
    ts/pokerth_bg.ts \
    ts/pokerth_zhcn.ts \
    ts/pokerth_ca.ts \
    ts/pokerth_cz.ts \
    ts/pokerth_dk.ts \
    ts/pokerth_nl.ts \
    ts/pokerth_fr.ts \
    ts/pokerth_fi.ts \
    ts/pokerth_de.ts \
    ts/pokerth_gr.ts \
    ts/pokerth_hu.ts \
    ts/pokerth_it.ts \
    ts/pokerth_lt.ts \
    ts/pokerth_no.ts \
    ts/pokerth_pl.ts \
    ts/pokerth_ptbr.ts \
    ts/pokerth_ptpt.ts \
    ts/pokerth_ru.ts \
    ts/pokerth_sk.ts \
    ts/pokerth_es.ts \
    ts/pokerth_sv.ts \
    ts/pokerth_ta.ts \
    ts/pokerth_tr.ts \
    ts/pokerth_START_HERE.ts
win32 { 
    DEFINES += CURL_STATICLIB
    DEFINES += _WIN32_WINNT=0x0501
    DEPENDPATH += src/net/win32/ \
        src/core/win32
    INCLUDEPATH += ../boost/ \
        ../SDL/include \
		../SDL/include/SDL \
		../SDL_mixer \
		../SDL_mixer/include/SDL \
        ../GnuTLS/include \
        ../gsasl/include \
        ../curl/include \
        ../zlib
    LIBPATH += ../boost/stage/lib \
        ../GnuTLS/lib \
		../openssl/lib \
        ../gsasl/lib \
        ../curl/lib \
        ../SDL/lib \
        ../SDL_mixer/lib \
        ../mysql/lib \
        ../zlib
    LIBS += -lpokerth_lib \
        -lpokerth_db \
        -lpokerth_protocol
    debug:LIBPATH += Debug/lib
    release:LIBPATH += Release/lib
    win32-msvc2008:LIBS += -llibgnutls-openssl-26 \
        -llibgcrypt-11 \
        -llibgsasl-7 \
        -llibcurl
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
        -lsdl \
        -lsdlmain \
        -lsdl_mixer \
        -lwldap32
    RC_FILE = pokerth.rc
}
unix:# workaround for problems with boost_filesystem exceptions
QMAKE_LFLAGS += -no_dead_strip_inits_and_terms
unix:!mac { 
    # #### My release static build options
    # QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
    # QMAKE_LFLAGS += -Wl,--gc-sections
    LIBPATH += lib
    LIB_DIRS = $${PREFIX}/lib \
        $${PREFIX}/lib64
    BOOST_FS = boost_filesystem \
        boost_filesystem-mt
    BOOST_THREAD = boost_thread \
        boost_thread-mt
    BOOST_IOSTREAMS = boost_iostreams \
        boost_iostreams-mt
    BOOST_REGEX = boost_regex \
        boost_regex-mt
    BOOST_SYS = boost_system \
        boost_system-mt
    
    # searching in $PREFIX/lib and $PREFIX/lib64
    # to override the default '/usr' pass PREFIX
    # variable to qmake.
    for(dir, LIB_DIRS):exists($$dir) { 
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
    BOOST_LIBS = $$BOOST_THREAD \
        $$BOOST_FS \
        $$BOOST_IOSTREAMS \
        $$BOOST_REGEX \
        $$BOOST_SYS
    !count(BOOST_LIBS, 5):error("Unable to find boost libraries in PREFIX=$${PREFIX}")
    if($$system(sdl-config --version)):error("sdl-config not found in PATH - libSDL_mixer, libSDL are required!")
    UNAME = $$system(uname -s)
    BSD = $$find(UNAME, "BSD")
    kFreeBSD = $$find(UNAME, "kFreeBSD")
    LIBS += -lpokerth_lib \
        -lpokerth_db \
        -lpokerth_protocol
    LIBS += $$BOOST_LIBS
    LIBS += -lSDL_mixer \
        -lcurl \
        -lgsasl
    !isEmpty( BSD ):isEmpty( kFreeBSD ):LIBS += -lcrypto
    else:LIBS += -lgnutls-openssl \
        -lgcrypt
    TARGETDEPS += ./lib/libpokerth_lib.a \
        ./lib/libpokerth_db.a \
        ./lib/libpokerth_protocol.a
    
    # #### My release static libs
    # LIBS += -lgcrypt_static -lgpg-error_static -lgnutls-openssl_static -lgnutls_static -lSDL_mixer_static -lSDL -lmikmod -lcurl
    # ### INSTALL ####
    binary.path += $${PREFIX}/bin/
    binary.files += pokerth
    data.path += $${PREFIX}/share/pokerth/data/
    data.files += data/*
    pixmap.path += $${PREFIX}/share/pixmaps/
    pixmap.files += pokerth.png
    desktop.path += $${PREFIX}/share/applications/
    desktop.files += pokerth.desktop
    INSTALLS += binary \
        data \
        pixmap \
        desktop
}
mac { 
    # make it universal
    CONFIG += x86
	CONFIG -= ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
    
    # for universal-compilation on PPC-Mac uncomment the following line
    # on Intel-Mac you have to comment this line out or build will fail.
    # QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
    LIBPATH += lib
    LIBS += -lpokerth_lib \
        -lpokerth_db \
        -lpokerth_protocol
    
    # QT dynamic linked framework (see also mac_post_make.sh)
    LIBS += -framework \
        QtCore
    LIBS += -framework \
        QtGui
    
    # SDL and SDL_mixer come as frameworks
    LIBS += -framework \
        SDL
    LIBS += -framework \
        SDL_mixer
    
    # standard path for darwinports
    # make sure you have a universal version of boost
    LIBS += /usr/local/lib/libboost_thread.a
    LIBS += /usr/local/lib/libboost_filesystem.a
    LIBS += /usr/local/lib/libboost_regex.a
    LIBS += /usr/local/lib/libboost_system.a
    LIBS += /usr/local/lib/libboost_iostreams.a
    LIBS += /usr/local/lib/libgsasl.a
    
    # libraries installed on every mac
    LIBS += -lcrypto \
        -lssl \
        -lz \
        -lcurl \
        -framework \
        Carbon
    
    # set the application icon
    RC_FILE = pokerth.icns
    LIBPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/lib
    INCLUDEPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/include/
    INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
    INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
}
OTHER_FILES += docs/infomessage-id-desc.txt
official_server { 
    LIBPATH += pkth_stat/daemon_lib/lib
    LIBS += -lpokerth_closed \
        -lmysqlpp
    DEFINES += POKERTH_OFFICIAL_SERVER
}
