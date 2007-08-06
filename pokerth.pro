# QMake pro-file for PokerTH

INSTALLS += TARGET 
TARGET.files += bin/* 
TARGET.path = /usr/bin/ 

INCLUDEPATH += . \
		src \
		src/engine \
		src/gui \
		src/net \
		src/engine/local_engine \
		src/engine/network_engine \
		src/config \
		src/core/tinyxml \
		src/gui/qt \
		src/gui/qt/connecttoserverdialog \
		src/core \
		src/gui/qt/sound \
		src/gui/qt/qttools \
		src/gui/qt/qttools/qthelper \
		src/gui/qt/mainwindow \
		src/gui/qt/mainwindow/startsplash \
		src/gui/qt/mainwindow/log \
		src/gui/qt/mainwindow/chat \
		src/gui/qt/aboutpokerth \
		src/gui/qt/createnetworkgamedialog \
		src/gui/qt/joinnetworkgamedialog \
		src/gui/qt/newlocalgamedialog \
		src/gui/qt/settingsdialog \
		src/gui/qt/settingsdialog/selectavatardialog \
		src/gui/qt/startnetworkgamedialog \
		src/gui/qt/changehumanplayernamedialog

DEPENDPATH += . \
              src \
              src/config \
              src/core \
              src/engine \
              src/gui \
              src/net \
              src/core/common \
              src/core/tinyxml \
              src/engine/local_engine \
              src/engine/network_engine \
              src/gui/qt \
              src/net/common \
	      src/gui/qt/sound \
              src/gui/qt/mainwindow \
              src/gui/qt/mainwindow/startsplash \
	      src/gui/qt/mainwindow/log \
	      src/gui/qt/mainwindow/chat \
              src/gui/qt/aboutpokerth \
              src/gui/qt/connecttoserverdialog \
              src/gui/qt/createnetworkgamedialog \
              src/gui/qt/joinnetworkgamedialog \
              src/gui/qt/qttools \
              src/gui/qt/qttools/qthelper \
              src/gui/qt/newlocalgamedialog \
              src/gui/qt/settingsdialog \
	      src/gui/qt/settingsdialog/selectavatardialog \
              src/gui/qt/startnetworkgamedialog \
	      src/gui/qt/changehumanplayernamedialog
# Input
HEADERS += src/game.h \
           src/session.h \
           src/playerdata.h \
           src/gamedata.h \
           src/config/configfile.h \
           src/core/rand.h \
           src/core/thread.h \
           src/engine/boardinterface.h \
           src/engine/enginefactory.h \
           src/engine/flopinterface.h \
           src/engine/handinterface.h \
           src/engine/playerinterface.h \
           src/engine/preflopinterface.h \
           src/engine/riverinterface.h \
           src/engine/turninterface.h \
	   src/engine/berointerface.h \
           src/engine/berofactoryinterface.h \
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
           src/net/serverthread.h \
           src/net/socket_helper.h \
           src/net/socket_msg.h \
           src/net/socket_startup.h \
           src/core/tinyxml/tinystr.h \
           src/core/tinyxml/tinyxml.h \
           src/engine/local_engine/cardsvalue.h \
           src/engine/local_engine/localboard.h \
           src/engine/local_engine/localenginefactory.h \
           src/engine/local_engine/localflop.h \
 	   src/engine/local_engine/localberoflop.h \
           src/engine/local_engine/localhand.h \
           src/engine/local_engine/localplayer.h \
           src/engine/local_engine/localpreflop.h \
	   src/engine/local_engine/localberopreflop.h \
           src/engine/local_engine/localriver.h \
           src/engine/local_engine/localberoriver.h \
           src/engine/local_engine/localberopostriver.h \
           src/engine/local_engine/localturn.h \
	   src/engine/local_engine/localberoturn.h \
           src/engine/local_engine/tools.h \
	   src/engine/local_engine/localbero.h \
           src/engine/local_engine/localberofactory.h \
           src/engine/network_engine/clientboard.h \
           src/engine/network_engine/clientenginefactory.h \
           src/engine/network_engine/clientflop.h \
           src/engine/network_engine/clienthand.h \
           src/engine/network_engine/clientplayer.h \
           src/engine/network_engine/clientpreflop.h \
           src/engine/network_engine/clientriver.h \
           src/engine/network_engine/clientturn.h \
     	   src/engine/network_engine/clientbero.h \
	   src/engine/network_engine/clientberofactory.h \
	   src/gui/qt/sound/sdlplayer.h \
           src/gui/qt/mainwindow/mainwindowimpl.h \
           src/gui/qt/mainwindow/mycardspixmaplabel.h \
	   src/gui/qt/mainwindow/mysetlabel.h \
	   src/gui/qt/mainwindow/myrighttabwidget.h \
	   src/gui/qt/mainwindow/mylefttabwidget.h \
           src/gui/qt/mainwindow/startsplash/startsplash.h \
	   src/gui/qt/mainwindow/log/log.h \
	   src/gui/qt/mainwindow/chat/chat.h \
           src/gui/qt/guiwrapper.h \
           src/gui/qt/aboutpokerth/aboutpokerthimpl.h \
           src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.h \
           src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.h \
           src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.h \
           src/gui/qt/newlocalgamedialog/newgamedialogimpl.h \
           src/gui/qt/settingsdialog/settingsdialogimpl.h \
	   src/gui/qt/settingsdialog/myavatarbutton.h \
	   src/gui/qt/settingsdialog/myhpavatarbutton.h \
	   src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.h \
	   src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.h \
           src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.h \
	   src/gui/qt/changehumanplayernamedialog/changehumanplayernamedialogimpl.h \
           src/gui/qttoolsinterface.h \
           src/gui/qt/qttools/qttoolswrapper.h \
           src/gui/qt/qttools/qthelper/qthelper.h \
           src/gui/generic/serverguiwrapper.h
FORMS += src/gui/qt/mainwindow.ui \
         src/gui/qt/aboutpokerth.ui \
         src/gui/qt/connecttoserverdialog.ui \
         src/gui/qt/createnetworkgamedialog.ui \
         src/gui/qt/joinnetworkgamedialog.ui \
         src/gui/qt/newgamedialog.ui \
         src/gui/qt/settingsdialog.ui \
	 src/gui/qt/selectavatardialog.ui \
         src/gui/qt/startnetworkgamedialog.ui \
	 src/gui/qt/changehumanplayernamedialog.ui
SOURCES += src/game.cpp \
           src/pokerth.cpp \
           src/session.cpp \
           src/playerdata.cpp \
           src/config/configfile.cpp \
           src/engine/boardinterface.cpp \
           src/engine/enginefactory.cpp \
           src/engine/flopinterface.cpp \
           src/engine/handinterface.cpp \
           src/engine/playerinterface.cpp \
           src/engine/preflopinterface.cpp \
           src/engine/riverinterface.cpp \
           src/engine/turninterface.cpp \
	   src/engine/berointerface.cpp \
           src/engine/berofactoryinterface.cpp \
           src/gui/guiinterface.cpp \
           src/core/common/thread.cpp \
           src/core/tinyxml/tinystr.cpp \
           src/core/tinyxml/tinyxml.cpp \
           src/core/tinyxml/tinyxmlerror.cpp \
           src/core/tinyxml/tinyxmlparser.cpp \
           src/engine/local_engine/cardsvalue.cpp \
           src/engine/local_engine/localboard.cpp \
           src/engine/local_engine/localenginefactory.cpp \
           src/engine/local_engine/localflop.cpp \
           src/engine/local_engine/localberoflop.cpp \
	   src/engine/local_engine/localhand.cpp \
           src/engine/local_engine/localplayer.cpp \
           src/engine/local_engine/localpreflop.cpp \
	   src/engine/local_engine/localberopreflop.cpp \
	   src/engine/local_engine/localriver.cpp \
	   src/engine/local_engine/localberoriver.cpp \
	   src/engine/local_engine/localberopostriver.cpp \
           src/engine/local_engine/localturn.cpp \
	   src/engine/local_engine/localberoturn.cpp \
           src/engine/local_engine/tools.cpp \
           src/engine/local_engine/localbero.cpp \
	   src/engine/local_engine/localberofactory.cpp \
           src/engine/network_engine/clientboard.cpp \
           src/engine/network_engine/clientenginefactory.cpp \
           src/engine/network_engine/clientflop.cpp \
           src/engine/network_engine/clienthand.cpp \
           src/engine/network_engine/clientplayer.cpp \
           src/engine/network_engine/clientpreflop.cpp \
           src/engine/network_engine/clientriver.cpp \
           src/engine/network_engine/clientturn.cpp \
	   src/engine/network_engine/clientbero.cpp \
           src/engine/network_engine/clientberofactory.cpp \
           src/net/common/connectdata.cpp \
           src/net/common/clientcallback.cpp \
           src/net/common/clientcontext.cpp \
           src/net/common/clientstate.cpp \
           src/net/common/clientthread.cpp \
           src/net/common/netpacket.cpp \
           src/net/common/resolverthread.cpp \
           src/net/common/senderthread.cpp \
           src/net/common/sendercallback.cpp \
           src/net/common/serverthread.cpp \
           src/net/common/servercontext.cpp \
           src/net/common/serverexception.cpp \
           src/net/common/serverrecvthread.cpp \
           src/net/common/serverrecvstate.cpp \
           src/net/common/servercallback.cpp \
           src/net/common/sessiondata.cpp \
           src/net/common/socket_startup_cmn.cpp \
           src/net/common/socket_helper_cmn.cpp \
           src/net/common/clientexception.cpp \
           src/net/common/netcontext.cpp \
           src/net/common/netexception.cpp \
           src/net/common/receiverhelper.cpp \
	   src/gui/qt/sound/sdlplayer.cpp \
           src/gui/qt/guiwrapper.cpp \
           src/gui/qt/mainwindow/mainwindowimpl.cpp \
           src/gui/qt/mainwindow/mycardspixmaplabel.cpp \
	   src/gui/qt/mainwindow/mysetlabel.cpp \
	   src/gui/qt/mainwindow/myrighttabwidget.cpp \
	   src/gui/qt/mainwindow/mylefttabwidget.cpp \
           src/gui/qt/mainwindow/startsplash/startsplash.cpp \
	   src/gui/qt/mainwindow/log/log.cpp \
	   src/gui/qt/mainwindow/chat/chat.cpp \
           src/gui/qt/aboutpokerth/aboutpokerthimpl.cpp \
           src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.cpp \
           src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.cpp \
           src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.cpp \
           src/gui/qt/newlocalgamedialog/newgamedialogimpl.cpp \
           src/gui/qt/settingsdialog/settingsdialogimpl.cpp \
	   src/gui/qt/settingsdialog/myavatarbutton.cpp \
	   src/gui/qt/settingsdialog/myhpavatarbutton.cpp \
	   src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.cpp \
	   src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.cpp \
           src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.cpp \
	   src/gui/qt/changehumanplayernamedialog/changehumanplayernamedialogimpl.cpp \
           src/gui/qttoolsinterface.cpp \
           src/gui/qt/qttools/qttoolswrapper.cpp \
           src/gui/qt/qttools/qthelper/qthelper.cpp \
           src/gui/generic/serverguiwrapper.cpp 
RESOURCES += src/gui/qt/resources.qrc
TRANSLATIONS = ts/pokerth_de.ts \
               ts/pokerth_es.ts \
               ts/pokerth_fr.ts \
               ts/pokerth_ru.ts \
	       ts/pokerth_no.ts \
               ts/pokerth_sk.ts \
      	       ts/pokerth_eo.ts \
	       ts/pokerth_hu.ts \
	       ts/pokerth_pl.ts \
	       ts/pokerth_ptbr.ts \
   	       ts/pokerth_it.ts \
               ts/pokerth_nl.ts
CODECFORSRC = UTF-8

TEMPLATE = vcapp
TEMPLATE = app

win32{
    DEPENDPATH += src/net/win32/ src/core/win32
    SOURCES += src/core/win32/rand.cpp \
		src/net/win32/socket_helper.cpp \
		src/net/win32/socket_startup.cpp 
    INCLUDEPATH += ../boost/ ../SDL/include ../SDL_mixer
    LIBPATH += ../boost/stage/lib ../SDL/VisualC/SDL/Release ../SDL/VisualC/SDLmain/Release ../SDL_mixer/VisualC/Release
    LIBS += gdi32.lib comdlg32.lib oleaut32.lib imm32.lib winmm.lib winspool.lib ole32.lib uuid.lib user32.lib msimg32.lib shell32.lib kernel32.lib ws2_32.lib advapi32.lib sdl.lib sdlmain.lib sdl_mixer.lib
    RC_FILE = pokerth.rc
}
!win32{
    DEPENDPATH += src/net/linux/ src/core/linux
    SOURCES += src/core/linux/rand.cpp \
		src/net/linux/socket_helper.cpp \
		src/net/linux/socket_startup.cpp 
}

unix: !mac{
        exists( /usr/lib/libboost_thread-mt.so ){
            message("Found libboost_thread-mt")
            LIBS += -lboost_thread-mt
        }
        exists( /usr/lib/libboost_thread.so ){
            message("Found libboost_thread")
            LIBS += -lboost_thread
        }
        LIBS += -lcrypto -lSDL_mixer
        ## My release static libs
        #LIBS += -lcrypto -lSDL_mixer -lSDL -lmikmod
    }

mac{
    # make it universal  
    CONFIG += x86 
    CONFIG += ppc
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3

    # for universal-compilation on PPC-Mac uncomment the following line
    # on Intel-Mac you have to comment this line out or build will fail.
    #	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/

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
    LIBS += /opt/local/lib/libboost_thread.a
    # libraries installed on every mac
    LIBS += -lcrypto -lz -framework Carbon
    # set the application icon
    RC_FILE = pokerth.icns
    LIBPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/lib 
    INCLUDEPATH += /Developer/SDKs/MacOSX10.4u.sdk/usr/include/
    INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
    INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers		
}

#CONFIG += qt release
CONFIG += qt warn_on debug
UI_DIR = uics
TARGET = bin/pokerth
MOC_DIR = mocs
OBJECTS_DIR = obj
QT += 
# QMAKE_CXXFLAGS_DEBUG += -g
