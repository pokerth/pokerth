project(pokerth_game)

find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sql REQUIRED)
find_package(Qt5LinguistTools)

find_package(GSasl REQUIRED)
find_package(SDL REQUIRED)
find_package(SDLMixer REQUIRED)
find_package(Sqlite3 REQUIRED)
find_package(TinyXML REQUIRED)
find_package(Protobuf REQUIRED)
find_package(GCrypt REQUIRED)

find_package(Boost REQUIRED COMPONENTS filesystem thread iostreams regex system random chrono)

add_definitions(-DENABLE_IPV6 -DTIXML_USE_STL -DBOOST_FILESYSTEM_DEPRECATED)

set(RESOURCES src/gui/qt/resources/pokerth.qrc)

include_directories(
        src
        src/engine
        src/gui
        src/net
        src/engine/local_engine
        src/engine/network_engine
        src/config
        src/gui/qt
        src/gui/qt/connecttoserverdialog
        src/core
        src/gui/qt/sound
        src/gui/qt/qttools
        src/gui/qt/chattools
        src/gui/qt/qttools/qthelper
        src/gui/qt/gametable
        src/gui/qt/gametable/startsplash
        src/gui/qt/gametable/log
        src/gui/qt/aboutpokerth
        src/gui/qt/createnetworkgamedialog
        src/gui/qt/createinternetgamedialog
        src/gui/qt/joinnetworkgamedialog
        src/gui/qt/newlocalgamedialog
        src/gui/qt/settingsdialog
        src/gui/qt/settingsdialog/selectavatardialog
        src/gui/qt/settingsdialog/manualblindsorderdialog
        src/gui/qt/startnetworkgamedialog
        src/gui/qt/startwindow
        src/gui/qt/serverlistdialog
        src/gui/qt/styles
        src/gui/qt/changecontentdialog
        src/gui/qt/changecompleteblindsdialog
        src/gui/qt/internetgamelogindialog
        src/gui/qt/mymessagedialog
        src/gui/qt/gamelobbydialog
        src/gui/qt/timeoutmsgbox
        src/gui/qt/logfiledialog
        src/gui/qt/mymessagebox
        src/third_party/qtlockedfile
)

include_directories(
        ${GSASL_INCLUDE_DIR}
        ${SDL_INCLUDE_DIR}
        ${SDL_MIXER_INCLUDE_DIR}
        ${SQLITE3_INCLUDE_DIR}
        ${TINYXML_INCLUDE_DIR}
        ${GCRYPT_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
)

if(WIN32)
   add_definitions(-DCURL_STATICLIB -D_WIN32_WINNT=0x0501 -DHAVE_OPENSSL)
   include_directories(../boost/ ../GnuTLS/include ../gsasl/include
       ../curl/include ../zlib ../sqlite ../openssl/include)
endif(WIN32)

set(pokerth_game_HDRS
	src/config/configfile.h
	src/core/loghelper.h
	src/core/thread.h
	src/engine/berointerface.h
	src/engine/boardinterface.h
	src/engine/enginefactory.h
	src/engine/handinterface.h
	src/engine/local_engine/cardsvalue.h
	src/engine/local_engine/localbero.h
	src/engine/local_engine/localberoflop.h
	src/engine/local_engine/localberopostriver.h
	src/engine/local_engine/localberopreflop.h
	src/engine/local_engine/localberoriver.h
	src/engine/local_engine/localberoturn.h
	src/engine/local_engine/localboard.h
	src/engine/local_engine/localenginefactory.h
	src/engine/local_engine/localhand.h
	src/engine/local_engine/localplayer.h
	src/engine/local_engine/replay.h
	src/engine/local_engine/tools.h
	src/engine/network_engine/clientbero.h
	src/engine/network_engine/clientboard.h
	src/engine/network_engine/clientenginefactory.h
	src/engine/network_engine/clienthand.h
	src/engine/network_engine/clientplayer.h
	src/engine/playerinterface.h
	src/gamedata.h
	src/gui/generic/serverguiwrapper.h
	src/gui/guiinterface.h
	src/gui/qt/aboutpokerth/aboutpokerthimpl.h
	src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.h
	src/gui/qt/changecontentdialog/changecontentdialogimpl.h
	src/gui/qt/chattools/chattools.h
	src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.h
	src/gui/qt/createinternetgamedialog/createinternetgamedialogimpl.h
	src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.h
	src/gui/qt/gamelobbydialog/gamelobbydialogimpl.h
	src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.h
	src/gui/qt/gamelobbydialog/mygamelisttreewidget.h
	src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.h
	src/gui/qt/gametable/gametableimpl.h
	src/gui/qt/gametable/log/guilog.h
	src/gui/qt/gametable/myactionbutton.h
	src/gui/qt/gametable/myavatarlabel.h
	src/gui/qt/gametable/mycardspixmaplabel.h
	src/gui/qt/gametable/mycashlabel.h
	src/gui/qt/gametable/mychancelabel.h
	src/gui/qt/gametable/mylefttabwidget.h
	src/gui/qt/gametable/mynamelabel.h
	src/gui/qt/gametable/myrighttabwidget.h
	src/gui/qt/gametable/mysetlabel.h
    src/gui/qt/gametable/myslider.h
	src/gui/qt/gametable/mystatuslabel.h
	src/gui/qt/gametable/mytimeoutlabel.h
	src/gui/qt/gametable/startsplash/startsplash.h
	src/gui/qt/guiwrapper.h
	src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.h
	src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.h
	src/gui/qt/logfiledialog/logfiledialog.h
	src/gui/qt/mymessagebox/mymessagebox.h
	src/gui/qt/mymessagedialog/mymessagedialogimpl.h
	src/gui/qt/newlocalgamedialog/newgamedialogimpl.h
	src/gui/qt/qttools/qthelper/qthelper.h
	src/gui/qt/qttools/qttoolswrapper.h
	src/gui/qt/serverlistdialog/serverlistdialogimpl.h
	src/gui/qt/settingsdialog/manualblindsorderdialog/manualblindsorderdialogimpl.h
	src/gui/qt/settingsdialog/myavatarbutton.h
	src/gui/qt/settingsdialog/myhpavatarbutton.h
	src/gui/qt/settingsdialog/mystylelistitem.h
	src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.h
	src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.h
	src/gui/qt/settingsdialog/settingsdialogimpl.h
	src/gui/qt/sound/soundevents.h
	src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.h
	src/gui/qt/startwindow/startwindowimpl.h
	src/gui/qt/styles/carddeckstylereader.h
	src/gui/qt/styles/gametablestylereader.h
	src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.h
	src/gui/qttoolsinterface.h
	src/net/clientcallback.h
	src/net/clientcontext.h
	src/net/clientexception.h
	src/net/clientstate.h
	src/net/clientthread.h
	src/net/genericsocket.h
	src/net/net_helper.h
	src/net/netpacket.h
	src/net/senderhelper.h
	src/net/serveraccepthelper.h
	src/net/serverlobbythread.h
	src/net/socket_helper.h
	src/net/socket_msg.h
	src/net/socket_startup.h
	src/playerdata.h
	src/session.h
    src/engine/game.h

    src/third_party/qtsingleapplication/qtsingleapplication.h
    src/third_party/qtsingleapplication/qtlocalpeer.h
    src/third_party/qtlockedfile/qtlockedfile.h

    src/gui/qt/sound/sdlplayer.h
)
if(GUI_800x480)
    include_directories(src/gui/qt/gui_800x480)
    set(FORMS
        src/gui/qt/gui_800x480/startwindow_800x480.ui
        src/gui/qt/gui_800x480/gametable_800x480.ui
        src/gui/qt/gui_800x480/aboutpokerth_800x480.ui
        src/gui/qt/gui_800x480/connecttoserverdialog_800x480.ui
        src/gui/qt/gui_800x480/createnetworkgamedialog_800x480.ui
        src/gui/qt/gui_800x480/createinternetgamedialog_800x480.ui
        src/gui/qt/gui_800x480/joinnetworkgamedialog_800x480.ui
        src/gui/qt/gui_800x480/newgamedialog_800x480.ui
        src/gui/qt/gui_800x480/settingsdialog_800x480.ui
        src/gui/qt/gui_800x480/selectavatardialog_800x480.ui
        src/gui/qt/gui_800x480/startnetworkgamedialog_800x480.ui
        src/gui/qt/gui_800x480/changecompleteblindsdialog_800x480.ui
        src/gui/qt/gui_800x480/gamelobbydialog_800x480.ui
        src/gui/qt/gui_800x480/mymessagedialog_800x480.ui
        src/gui/qt/gui_800x480/manualblindsorderdialog_800x480.ui
        src/gui/qt/gui_800x480/serverlistdialog_800x480.ui
        src/gui/qt/gui_800x480/internetgamelogindialog_800x480.ui
        src/gui/qt/gui_800x480/changecontentdialog_800x480.ui
        src/gui/qt/gui_800x480/logfiledialog_800x480.ui
        src/gui/qt/gui_800x480/tabs_800x480.ui
    )
else(GUI_800x480)
    set(FORMS src/gui/qt/gametable.ui
        src/gui/qt/aboutpokerth.ui
        src/gui/qt/connecttoserverdialog.ui
        src/gui/qt/createnetworkgamedialog.ui
        src/gui/qt/createinternetgamedialog.ui
        src/gui/qt/joinnetworkgamedialog.ui
        src/gui/qt/newgamedialog.ui
        src/gui/qt/settingsdialog.ui
        src/gui/qt/selectavatardialog.ui
        src/gui/qt/startnetworkgamedialog.ui
        src/gui/qt/startwindow.ui
        src/gui/qt/changecompleteblindsdialog.ui
        src/gui/qt/gamelobbydialog.ui
        src/gui/qt/mymessagedialog.ui
        src/gui/qt/manualblindsorderdialog.ui
        src/gui/qt/serverlistdialog.ui
        src/gui/qt/internetgamelogindialog.ui
        src/gui/qt/logfiledialog.ui
        src/gui/qt/changecontentdialog.ui
    )
endif(GUI_800x480)

set(pokerth_game_SRCS
	src/core/common/loghelper_client.cpp
	src/engine/local_engine/replay.cpp
	src/gui/qt/aboutpokerth/aboutpokerthimpl.cpp
	src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.cpp
	src/gui/qt/changecontentdialog/changecontentdialogimpl.cpp
	src/gui/qt/chattools/chattools.cpp
	src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.cpp
	src/gui/qt/createinternetgamedialog/createinternetgamedialogimpl.cpp
	src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.cpp
	src/gui/qt/gamelobbydialog/gamelobbydialogimpl.cpp
	src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.cpp
	src/gui/qt/gamelobbydialog/mygamelisttreewidget.cpp
	src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.cpp
	src/gui/qt/gametable/gametableimpl.cpp
	src/gui/qt/gametable/log/guilog.cpp
	src/gui/qt/gametable/myactionbutton.cpp
	src/gui/qt/gametable/myavatarlabel.cpp
	src/gui/qt/gametable/mycardspixmaplabel.cpp
	src/gui/qt/gametable/mycashlabel.cpp
	src/gui/qt/gametable/mychancelabel.cpp
	src/gui/qt/gametable/mylefttabwidget.cpp
	src/gui/qt/gametable/mynamelabel.cpp
	src/gui/qt/gametable/myrighttabwidget.cpp
	src/gui/qt/gametable/mysetlabel.cpp
	src/gui/qt/gametable/mystatuslabel.cpp
	src/gui/qt/gametable/mytimeoutlabel.cpp
	src/gui/qt/gametable/startsplash/startsplash.cpp
	src/gui/qt/guiwrapper.cpp
	src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.cpp
	src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.cpp
	src/gui/qt/logfiledialog/logfiledialog.cpp
	src/gui/qt/mymessagebox/mymessagebox.cpp
	src/gui/qt/mymessagedialog/mymessagedialogimpl.cpp
	src/gui/qt/newlocalgamedialog/newgamedialogimpl.cpp
	src/gui/qt/qttools/qthelper/qthelper.cpp
	src/gui/qt/qttools/qttoolswrapper.cpp
	src/gui/qt/serverlistdialog/serverlistdialogimpl.cpp
	src/gui/qt/settingsdialog/manualblindsorderdialog/manualblindsorderdialogimpl.cpp
	src/gui/qt/settingsdialog/myavatarbutton.cpp
	src/gui/qt/settingsdialog/myhpavatarbutton.cpp
	src/gui/qt/settingsdialog/mystylelistitem.cpp
	src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.cpp
	src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.cpp
	src/gui/qt/settingsdialog/settingsdialogimpl.cpp
	src/gui/qt/sound/soundevents.cpp
	src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.cpp
	src/gui/qt/startwindow/startwindowimpl.cpp
	src/gui/qt/styles/carddeckstylereader.cpp
	src/gui/qt/styles/gametablestylereader.cpp
	src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.cpp
	src/net/common/net_helper_client.cpp
	src/net/common/servermanagerfactoryclient.cpp
    src/pokerth.cpp

    src/third_party/qtsingleapplication/qtsingleapplication.cpp
    src/third_party/qtsingleapplication/qtlocalpeer.cpp
    src/third_party/qtlockedfile/qtlockedfile.cpp

    src/gui/qt/sound/sdlplayer.cpp
)

if(UNIX)
    set(pokerth_game_SRCS ${pokerth_game_SRCS}
        src/third_party/qtlockedfile/qtlockedfile_unix.cpp
    )
elseif(WIN32)
    set(pokerth_game_SRCS ${pokerth_game_SRCS}
        src/third_party/qtlockedfile/qtlockedfile_win.cpp
    )
endif(UNIX)

set(TRANSLATIONS
    ts/pokerth_af.ts
	ts/pokerth_bg.ts
	ts/pokerth_zhcn.ts
	ts/pokerth_ca.ts
	ts/pokerth_cz.ts
	ts/pokerth_dk.ts
	ts/pokerth_nl.ts
	ts/pokerth_fr.ts
	ts/pokerth_fi.ts
	ts/pokerth_de.ts
	ts/pokerth_gd.ts
	ts/pokerth_gl.ts
	ts/pokerth_gr.ts
	ts/pokerth_hu.ts
	ts/pokerth_it.ts
	ts/pokerth_jp.ts
	ts/pokerth_lt.ts
	ts/pokerth_no.ts
	ts/pokerth_pl.ts
	ts/pokerth_ptbr.ts
	ts/pokerth_ptpt.ts
	ts/pokerth_ru.ts
	ts/pokerth_sk.ts
	ts/pokerth_es.ts
	ts/pokerth_sv.ts
	ts/pokerth_ta.ts
	ts/pokerth_tr.ts
	ts/pokerth_vi.ts
	ts/pokerth_START_HERE.ts
)

set(OTHER_FILES
    #docs/infomessage-id-desc.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/pokerth.desktop
    ${CMAKE_CURRENT_SOURCE_DIR}/pokerth.ico
    ${CMAKE_CURRENT_SOURCE_DIR}/pokerth.png
    ${CMAKE_CURRENT_SOURCE_DIR}/pokerth.svg
)

qtcreator_add_project_resources(${RESOURCES} ${OTHER_FILES})

qt5_add_resources(RESOURCES_ADDED ${RESOURCES})
qt5_wrap_ui(FORMS_ADDED ${FORMS})
qt5_add_translation(QM_FILES ${TRANSLATIONS})

SET_SOURCE_FILES_PROPERTIES(
    pokerth.icns
    PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
)

set(LIBS_TO_COPY ${GSASL_LIBRARIES} ${SDL_LIBRARY} ${SDL_MIXER_LIBRARY}
    ${SQLITE3_LIBRARIES} ${TINYXML_LIBRARIES} ${PROTOBUF_LIBRARIES}
    ${GCRYPT_LIBRARIES} ${Boost_LIBRARIES})

set(LIBS pokerth_lib pokerth_db pokerth_protocol curl ${LIBS_TO_COPY})

add_executable(pokerth
    ${pokerth_game_MOC_SRCS}
    ${pokerth_game_SRCS}
    ${pokerth_game_HDRS}
    ${RESOURCES_ADDED}
    ${FORMS_ADDED}
    pokerth.icns
)

qt5_use_modules(pokerth Gui Quick Qml Core Widgets Sql)

target_link_libraries(pokerth
    ${LIBS})

install(TARGETS pokerth
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/)
install(FILES pokerth.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps/)
install(FILES pokerth.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
