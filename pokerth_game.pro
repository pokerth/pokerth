# QMake pro-file for PokerTH game

# Hack around https://bugreports.qt-project.org/browse/QTBUG-22829
QMAKE_MOC = $$QMAKE_MOC -DBOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

isEmpty( PREFIX ){
	PREFIX =/usr
}

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
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
UI_DIR = uics
MOC_DIR = mocs
OBJECTS_DIR = obj
DEFINES += ENABLE_IPV6 TIXML_USE_STL BOOST_FILESYSTEM_DEPRECATED
DEFINES += PREFIX=\"$${PREFIX}\"
TARGET = pokerth

# Check for c++11
include(pokerth_common.pro)

# PRECOMPILED_HEADER = src/pch_game.h

android{
    RESOURCES = src/gui/qt/resources/pokerth_android.qrc
}
!android{
    RESOURCES = src/gui/qt/resources/pokerth.qrc
}

INCLUDEPATH += . \
	src \
	src/engine \
	src/gui \
	src/net \
	src/engine/local_engine \
	src/engine/network_engine \
	src/config \
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
	src/gui/qt/timeoutmsgbox \
	src/gui/qt/logfiledialog \
	src/gui/qt/mymessagebox

DEPENDPATH += . \
	src \
	src/config \
	src/core \
	src/engine \
	src/gui \
	src/net \
	src/core/common \
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
	src/gui/qt/timeoutmsgbox \
	src/gui/qt/logfiledialog \
	src/gui/qt/mymessagebox

# Input
HEADERS += src/engine/game.h \
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
	src/gui/qt/gametable/gametableimpl.h \
	src/gui/qt/gametable/mycardspixmaplabel.h \
	src/gui/qt/gametable/mysetlabel.h \
	src/gui/qt/gametable/myactionbutton.h \
	src/gui/qt/gametable/mystatuslabel.h \
	src/gui/qt/gametable/myavatarlabel.h \
	src/gui/qt/gametable/myrighttabwidget.h \
	src/gui/qt/gametable/mylefttabwidget.h \
	src/gui/qt/gametable/startsplash/startsplash.h \
	src/gui/qt/gametable/log/guilog.h \
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
	src/gui/qt/gametable/mytimeoutlabel.h \
	src/gui/qt/gametable/mynamelabel.h \
	src/gui/qt/settingsdialog/mystylelistitem.h \
	src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.h \
	src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.h \
	src/engine/local_engine/replay.h \
	src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.h \
	src/gui/qt/gametable/myslider.h \
	src/gui/qt/gametable/mycashlabel.h \
	src/gui/qt/sound/soundevents.h \
	src/gui/qt/mymessagebox/mymessagebox.h \
	src/gui/qt/logfiledialog/logfiledialog.h

!gui_800x480 {
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
	src/gui/qt/logfiledialog.ui \
	src/gui/qt/changecontentdialog.ui
}

SOURCES += src/pokerth.cpp \
	src/gui/qt/chattools/chattools.cpp \
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
	src/gui/qt/gametable/log/guilog.cpp \
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
	src/gui/qt/gametable/mytimeoutlabel.cpp \
	src/gui/qt/gametable/mynamelabel.cpp \
	src/gui/qt/settingsdialog/mystylelistitem.cpp \
	src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.cpp \
	src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.cpp \
	src/engine/local_engine/replay.cpp \
	src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.cpp \
	src/net/common/servermanagerfactoryclient.cpp \
	src/gui/qt/gametable/mycashlabel.cpp \
	src/gui/qt/sound/soundevents.cpp \
	src/gui/qt/mymessagebox/mymessagebox.cpp \
	src/gui/qt/logfiledialog/logfiledialog.cpp

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
	ts/pokerth_gd.ts \
	ts/pokerth_gl.ts \
	ts/pokerth_gr.ts \
	ts/pokerth_hu.ts \
	ts/pokerth_it.ts \
	ts/pokerth_jp.ts \
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
	ts/pokerth_vi.ts \
	ts/pokerth_START_HERE.ts

LIBS += -lpokerth_lib \
	-lpokerth_db \
	-lpokerth_protocol \
	-lcurl

win32 { 
	DEFINES += CURL_STATICLIB
	DEFINES += _WIN32_WINNT=0x0501
	DEFINES += HAVE_OPENSSL
	DEPENDPATH += src/net/win32/ \
		src/core/win32
	INCLUDEPATH += ../boost/ \
		../SDL/include \
		../SDL/include/SDL \
		../SDL_mixer/include \
		../openssl/include \
		../gsasl/include \
		../curl/include \
		../sqlite \
		../zlib
	LIBPATH += ../boost/stage/lib \
		../openssl/lib \
		../gsasl/lib \
		../curl/lib \
		../SDL/lib \
		../SDL_mixer/lib \
		../mysql/lib \
		../zlib
	LIBS += -lSDL_mixer \
			-lsmpeg \
			-lSDL \
			-lSDLmain \
			-ltinyxml \
			-lz \
			-lssl \
			-lcrypto \
			-lssh2 \
			-lgnutls \
			-lhogweed \
			-lgmp \
			-lgcrypt \
			-lgpg-error \
			-lgsasl \
			-lnettle \
			-lidn \
			-lintl \
			-lprotobuf
	debug:LIBPATH += debug/lib
	release:LIBPATH += release/lib
			LIBS += -lsqlite3
			LIBS += -lntlm -lmodplug -lddraw -ldxguid -lvorbisfile -lvorbis -logg
			LIBS += -lboost_thread_win32-mt
			LIBS += -lboost_filesystem-mt
			LIBS += -lboost_regex-mt
			LIBS += -lboost_iostreams-mt
			LIBS += -lboost_random-mt
			LIBS += -lboost_chrono-mt
			LIBS += -lboost_system-mt

	LIBS += \
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
	RC_FILE = pokerth.rc
}
unix:!mac { 
	# #### My release static build options
	# QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
	# QMAKE_LFLAGS += -Wl,--gc-sections
	INCLUDEPATH += $${PREFIX}/include
	QMAKE_LIBDIR += lib
	!android{
		LIBPATH += $${PREFIX}/lib /opt/gsasl/lib
		LIB_DIRS = $${PREFIX}/lib \
			$${PREFIX}/lib64 \
                        $${PREFIX}/lib/x86_64-linux-gnu \
			$$system(qmake -query QT_INSTALL_LIBS)
	}
	android{
		LIBPATH += $${PREFIX}/lib/armv7
		LIB_DIRS = $${PREFIX}/lib/armv7
	}
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
	BOOST_RANDOM = boost_random \
		boost_random-mt
	BOOST_CHRONO = boost_chrono \
		boost_chrono-mt

	# searching in $PREFIX/lib, $PREFIX/lib64 and $$system(qmake -query QT_INSTALL_LIBS)
	# to override the default '/usr' pass PREFIX
	# variable to qmake.
	for(dir, LIB_DIRS):exists($$dir) {
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
			for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.so*) {
				message("Found $$lib")
				BOOST_REGEX = -l$$lib
			}
			for(lib, BOOST_REGEX):exists($${dir}/lib$${lib}.a) {
				message("Found $$lib")
				BOOST_REGEX = -l$$lib
			}
			!android{
				for(lib, BOOST_RANDOM):exists($${dir}/lib$${lib}.so*) {
					message("Found $$lib")
					BOOST_RANDOM = -l$$lib
				}
				for(lib, BOOST_RANDOM):exists($${dir}/lib$${lib}.a) {
					message("Found $$lib")
					BOOST_RANDOM = -l$$lib
				}
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
	!android{
		c++11 {
			BOOST_LIBS = $$BOOST_THREAD \
			$$BOOST_FS \
			$$BOOST_IOSTREAMS \
			$$BOOST_REGEX \
			$$BOOST_RANDOM \
			$$BOOST_SYS
		!count(BOOST_LIBS, 6):error("Unable to find boost libraries in PREFIX=$${PREFIX}")
		}
		!c++11 {
			BOOST_LIBS = $$BOOST_THREAD \
			$$BOOST_FS \
			$$BOOST_IOSTREAMS \
			$$BOOST_REGEX \
			$$BOOST_RANDOM \
			$$$$BOOST_SYS \
			$$BOOST_CHRONO
			!count(BOOST_LIBS, 7):error("Unable to find boost libraries in PREFIX=$${PREFIX}")
		}
		if($$system(sdl-config --version)):error("sdl-config not found in PATH - libSDL_mixer, libSDL are required!")
		UNAME = $$system(uname -s)
		BSD = $$find(UNAME, "BSD")
		kFreeBSD = $$find(UNAME, "kFreeBSD")
		LIBS += -lsqlite3 \
				-ltinyxml \
				-lprotobuf
		LIBS += $$BOOST_LIBS
		LIBS += -lSDL \
				-lSDL_mixer \
			-lgsasl
		!isEmpty( BSD ):isEmpty( kFreeBSD ):LIBS += -lcrypto
		else:LIBS += -lgcrypt
	}
	android{
		BOOST_LIBS = $$BOOST_THREAD \
			$$BOOST_FS \
			$$BOOST_IOSTREAMS \
			$$BOOST_REGEX \
			$$BOOST_SYS
		!count(BOOST_LIBS, 5):error("Unable to find boost libraries in PREFIX=$${PREFIX}/armv5")
		LIBS += -ltinyxml
		LIBS += $$BOOST_LIBS
		LIBS += -lgsasl -lidn
		LIBS += -lssl -lcrypto -lgcrypt -lgpg-error -lprotobuf-lite

		!android_api8 {
			#android sound lib for api9 and higher
			LIBS += -lOpenSLES
		}
	}
	TARGETDEPS += ./lib/libpokerth_lib.a \
		./lib/libpokerth_db.a \
		./lib/libpokerth_protocol.a

	!android{
		# #### My release static libs
		# LIBS += -lgcrypt_static -lgpg-error_static -lgnutls_static -lSDL_mixer_static -lSDL -lmikmod -lcurl
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
	# QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk/
	LIBPATH += lib

	# QT dynamic linked framework (see also mac_post_make.sh)
    LIBS += -F /Library/Frameworks
	LIBS += -framework \
		QtCore
	LIBS += -framework \
		QtGui
    LIBS += -framework \
        SDL
    LIBS += -framework \
        SDL_mixer

	# make sure you have an x86_64 version of boost
	LIBS += /usr/local/lib/libboost_thread-mt.a
	LIBS += /usr/local/lib/libboost_filesystem.a
	LIBS += /usr/local/lib/libboost_regex.a
	LIBS += /usr/local/lib/libboost_random.a
	LIBS += /usr/local/lib/libboost_system.a
	LIBS += /usr/local/lib/libboost_iostreams.a
	LIBS += /usr/local/lib/libgsasl.a

	# libraries installed on every mac
	LIBS += -lcrypto \
		-lssl \
		-lsqlite3 \
		-ltinyxml \
		-lprotobuf \
		-lz \
		-framework \
            Cocoa

	# set the application icon
	RC_FILE = pokerth.icns

    LIBPATH += /usr/local/lib
    LIBPATH += /usr/local/opt/openssl/lib
    LIBPATH += /usr/local/opt/tinyxml/lib
    LIBPATH += /usr/local/opt/protobuf/lib
	INCLUDEPATH += /usr/local/include
    INCLUDEPATH += /usr/local/opt/openssl/include
    INCLUDEPATH += /usr/local/opt/tinyxml/include
    INCLUDEPATH += /usr/local/opt/protobuf/include
    INCLUDEPATH += /Library/Frameworks/SDL.framework/Headers
    INCLUDEPATH += /Library/Frameworks/SDL_mixer.framework/Headers
}
OTHER_FILES += docs/infomessage-id-desc.txt
official_server { 
	LIBPATH += pkth_stat/daemon_lib/lib
	LIBS += -lpokerth_dbofficial \
		-lmysqlpp
	DEFINES += POKERTH_OFFICIAL_SERVER
}

gui_800x480 {

	DEFINES += GUI_800x480
	INCLUDEPATH +=  src/gui/qt/gui_800x480/
	DEPENDPATH += src/gui/qt/gui_800x480/
	FORMS +=  src/gui/qt/gui_800x480/startwindow_800x480.ui \
		src/gui/qt/gui_800x480/gametable_800x480.ui \
		src/gui/qt/gui_800x480/aboutpokerth_800x480.ui \
		src/gui/qt/gui_800x480/connecttoserverdialog_800x480.ui \
		src/gui/qt/gui_800x480/createnetworkgamedialog_800x480.ui \
		src/gui/qt/gui_800x480/createinternetgamedialog_800x480.ui \
		src/gui/qt/gui_800x480/joinnetworkgamedialog_800x480.ui \
		src/gui/qt/gui_800x480/newgamedialog_800x480.ui \
		src/gui/qt/gui_800x480/settingsdialog_800x480.ui \
		src/gui/qt/gui_800x480/selectavatardialog_800x480.ui \
		src/gui/qt/gui_800x480/startnetworkgamedialog_800x480.ui \
		src/gui/qt/gui_800x480/changecompleteblindsdialog_800x480.ui \
		src/gui/qt/gui_800x480/gamelobbydialog_800x480.ui \
		src/gui/qt/gui_800x480/mymessagedialog_800x480.ui \
		src/gui/qt/gui_800x480/manualblindsorderdialog_800x480.ui \
		src/gui/qt/gui_800x480/serverlistdialog_800x480.ui \
		src/gui/qt/gui_800x480/internetgamelogindialog_800x480.ui \
		src/gui/qt/gui_800x480/changecontentdialog_800x480.ui \
		src/gui/qt/gui_800x480/logfiledialog_800x480.ui \
		src/gui/qt/gui_800x480/tabs_800x480.ui
}

android{
	# Use old boost::filesystem, because the new version requires std::wstring.
	DEFINES += BOOST_FILESYSTEM_VERSION=3
	DEFINES += TIXML_USE_STL
	# sqlite3 is included directly.
	INCLUDEPATH += src/third_party/sqlite3

	# sound system switches
	!android_api8 {
		HEADERS += src/gui/qt/sound/androidaudio.h \
		src/gui/qt/sound/androidsoundeffect.h

		SOURCES += src/gui/qt/sound/androidaudio.cpp \
		src/gui/qt/sound/androidsoundeffect.cpp
	}
	android_api8 {
		DEFINES += ANDROID_API8
		HEADERS += src/gui/qt/sound/androidapi8dummy.h
		SOURCES += src/gui/qt/sound/androidapi8dummy.cpp
	}
}

!android{
	HEADERS += src/gui/qt/sound/sdlplayer.h
	SOURCES += src/gui/qt/sound/sdlplayer.cpp
}

maemo{
	DEFINES += MAEMO
}

android_test{
	DEFINES += ANDROID
	DEFINES += ANDROID_TEST
	DEFINES += ANDROID_API8
	HEADERS += src/gui/qt/sound/androidapi8dummy.h
	SOURCES += src/gui/qt/sound/androidapi8dummy.cpp
}
