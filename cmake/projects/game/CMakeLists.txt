project(pokerth_game)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
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

set(RESOURCES ${CMAKE_SOURCE_DIR}/src/gui/qt/resources/pokerth.qrc)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/net
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/gui/qt
    ${CMAKE_SOURCE_DIR}/src/gui/qt/connecttoserverdialog
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/gui/qt/sound
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools
    ${CMAKE_SOURCE_DIR}/src/gui/qt/chattools
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/qthelper
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/startsplash
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/log
    ${CMAKE_SOURCE_DIR}/src/gui/qt/aboutpokerth
    ${CMAKE_SOURCE_DIR}/src/gui/qt/createnetworkgamedialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/createinternetgamedialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/joinnetworkgamedialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/newlocalgamedialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/selectavatardialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/manualblindsorderdialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/startnetworkgamedialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/startwindow
    ${CMAKE_SOURCE_DIR}/src/gui/qt/serverlistdialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/styles
    ${CMAKE_SOURCE_DIR}/src/gui/qt/changecontentdialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/changecompleteblindsdialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/internetgamelogindialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/mymessagedialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/timeoutmsgbox
    ${CMAKE_SOURCE_DIR}/src/gui/qt/logfiledialog
    ${CMAKE_SOURCE_DIR}/src/gui/qt/mymessagebox
    ${CMAKE_SOURCE_DIR}/src/third_party/qtlockedfile
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
    ${CMAKE_SOURCE_DIR}/src/config/configfile.h
    ${CMAKE_SOURCE_DIR}/src/core/loghelper.h
    ${CMAKE_SOURCE_DIR}/src/core/thread.h
    ${CMAKE_SOURCE_DIR}/src/engine/berointerface.h
    ${CMAKE_SOURCE_DIR}/src/engine/boardinterface.h
    ${CMAKE_SOURCE_DIR}/src/engine/enginefactory.h
    ${CMAKE_SOURCE_DIR}/src/engine/handinterface.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/cardsvalue.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localbero.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberoflop.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberopostriver.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberopreflop.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberoriver.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberoturn.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localboard.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localenginefactory.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localhand.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localplayer.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/replay.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/tools.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientbero.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientboard.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientenginefactory.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clienthand.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientplayer.h
    ${CMAKE_SOURCE_DIR}/src/engine/playerinterface.h
    ${CMAKE_SOURCE_DIR}/src/gamedata.h
    ${CMAKE_SOURCE_DIR}/src/gui/generic/serverguiwrapper.h
    ${CMAKE_SOURCE_DIR}/src/gui/guiinterface.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/aboutpokerth/aboutpokerthimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/changecontentdialog/changecontentdialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/chattools/chattools.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/createinternetgamedialog/createinternetgamedialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/gamelobbydialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/mygamelisttreewidget.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/gametableimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/log/guilog.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/myactionbutton.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/myavatarlabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mycardspixmaplabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mycashlabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mychancelabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mylefttabwidget.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mynamelabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/myrighttabwidget.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mysetlabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/myslider.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mystatuslabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mytimeoutlabel.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/startsplash/startsplash.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/guiwrapper.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/logfiledialog/logfiledialog.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/mymessagebox/mymessagebox.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/mymessagedialog/mymessagedialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/newlocalgamedialog/newgamedialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/qthelper/qthelper.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/qttoolswrapper.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/serverlistdialog/serverlistdialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/manualblindsorderdialog/manualblindsorderdialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/myavatarbutton.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/myhpavatarbutton.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/mystylelistitem.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/settingsdialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/sound/soundevents.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/startwindow/startwindowimpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/styles/carddeckstylereader.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/styles/gametablestylereader.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.h
    ${CMAKE_SOURCE_DIR}/src/gui/qttoolsinterface.h
    ${CMAKE_SOURCE_DIR}/src/net/clientcallback.h
    ${CMAKE_SOURCE_DIR}/src/net/clientcontext.h
    ${CMAKE_SOURCE_DIR}/src/net/clientexception.h
    ${CMAKE_SOURCE_DIR}/src/net/clientstate.h
    ${CMAKE_SOURCE_DIR}/src/net/clientthread.h
    ${CMAKE_SOURCE_DIR}/src/net/genericsocket.h
    ${CMAKE_SOURCE_DIR}/src/net/net_helper.h
    ${CMAKE_SOURCE_DIR}/src/net/netpacket.h
    ${CMAKE_SOURCE_DIR}/src/net/senderhelper.h
    ${CMAKE_SOURCE_DIR}/src/net/serveraccepthelper.h
    ${CMAKE_SOURCE_DIR}/src/net/serverlobbythread.h
    ${CMAKE_SOURCE_DIR}/src/net/socket_helper.h
    ${CMAKE_SOURCE_DIR}/src/net/socket_msg.h
    ${CMAKE_SOURCE_DIR}/src/net/socket_startup.h
    ${CMAKE_SOURCE_DIR}/src/playerdata.h
    ${CMAKE_SOURCE_DIR}/src/session.h
    ${CMAKE_SOURCE_DIR}/src/engine/game.h

    ${CMAKE_SOURCE_DIR}/src/third_party/qtsingleapplication/qtsingleapplication.h
    ${CMAKE_SOURCE_DIR}/src/third_party/qtsingleapplication/qtlocalpeer.h
    ${CMAKE_SOURCE_DIR}/src/third_party/qtlockedfile/qtlockedfile.h

    ${CMAKE_SOURCE_DIR}/src/gui/qt/sound/sdlplayer.h
    )

if(GUI_800x480)
    include_directories(${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480)
    set(FORMS
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/startwindow_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/gametable_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/aboutpokerth_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/connecttoserverdialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/createnetworkgamedialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/createinternetgamedialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/joinnetworkgamedialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/newgamedialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/settingsdialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/selectavatardialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/startnetworkgamedialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/changecompleteblindsdialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/gamelobbydialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/mymessagedialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/manualblindsorderdialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/serverlistdialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/internetgamelogindialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/changecontentdialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/logfiledialog_800x480.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gui_800x480/tabs_800x480.ui
        )

else(GUI_800x480)
    set(FORMS
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/aboutpokerth.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/connecttoserverdialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/createnetworkgamedialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/createinternetgamedialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/joinnetworkgamedialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/newgamedialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/selectavatardialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/startnetworkgamedialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/startwindow.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/changecompleteblindsdialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/mymessagedialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/manualblindsorderdialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/serverlistdialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/internetgamelogindialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/logfiledialog.ui
        ${CMAKE_SOURCE_DIR}/src/gui/qt/changecontentdialog.ui
        )
endif(GUI_800x480)

set(pokerth_game_SRCS
    ${CMAKE_SOURCE_DIR}/src/core/common/loghelper_client.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/replay.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/aboutpokerth/aboutpokerthimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/changecompleteblindsdialog/changecompleteblindsdialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/changecontentdialog/changecontentdialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/chattools/chattools.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/connecttoserverdialog/connecttoserverdialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/createinternetgamedialog/createinternetgamedialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/createnetworkgamedialog/createnetworkgamedialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/gamelobbydialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/mygamelistsortfilterproxymodel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/mygamelisttreewidget.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gamelobbydialog/mynicklistsortfilterproxymodel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/gametableimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/log/guilog.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/myactionbutton.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/myavatarlabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mycardspixmaplabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mycashlabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mychancelabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mylefttabwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mynamelabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/myrighttabwidget.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mysetlabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mystatuslabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/mytimeoutlabel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/gametable/startsplash/startsplash.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/guiwrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/internetgamelogindialog/internetgamelogindialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/joinnetworkgamedialog/joinnetworkgamedialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/logfiledialog/logfiledialog.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/mymessagebox/mymessagebox.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/mymessagedialog/mymessagedialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/newlocalgamedialog/newgamedialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/qthelper/qthelper.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/qttoolswrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/serverlistdialog/serverlistdialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/manualblindsorderdialog/manualblindsorderdialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/myavatarbutton.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/myhpavatarbutton.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/mystylelistitem.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/selectavatardialog/myavatarlistitem.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/selectavatardialog/selectavatardialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/settingsdialog/settingsdialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/sound/soundevents.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/startnetworkgamedialog/startnetworkgamedialogimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/startwindow/startwindowimpl.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/styles/carddeckstylereader.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/styles/gametablestylereader.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/timeoutmsgbox/timeoutmsgboximpl.cpp
    ${CMAKE_SOURCE_DIR}/src/net/common/net_helper_client.cpp
    ${CMAKE_SOURCE_DIR}/src/net/common/servermanagerfactoryclient.cpp
    ${CMAKE_SOURCE_DIR}/src/pokerth.cpp

    ${CMAKE_SOURCE_DIR}/src/third_party/qtsingleapplication/qtsingleapplication.cpp
    ${CMAKE_SOURCE_DIR}/src/third_party/qtsingleapplication/qtlocalpeer.cpp
    ${CMAKE_SOURCE_DIR}/src/third_party/qtlockedfile/qtlockedfile.cpp

    ${CMAKE_SOURCE_DIR}/src/gui/qt/sound/sdlplayer.cpp
    )

if(UNIX)
    set(pokerth_game_SRCS ${pokerth_game_SRCS}
        ${CMAKE_SOURCE_DIR}/src/third_party/qtlockedfile/qtlockedfile_unix.cpp
        )
elseif(WIN32)
    set(pokerth_game_SRCS ${pokerth_game_SRCS}
        ${CMAKE_SOURCE_DIR}/src/third_party/qtlockedfile/qtlockedfile_win.cpp
        )
endif(UNIX)

set(TRANSLATIONS
    ${CMAKE_SOURCE_DIR}/ts/pokerth_af.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_bg.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_zhcn.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_ca.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_cz.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_dk.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_nl.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_fr.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_fi.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_de.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_gd.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_gl.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_gr.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_hu.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_it.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_jp.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_lt.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_no.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_pl.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_ptbr.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_ptpt.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_ru.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_sk.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_es.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_sv.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_ta.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_tr.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_vi.ts
    ${CMAKE_SOURCE_DIR}/ts/pokerth_START_HERE.ts
    )

set(OTHER_FILES
    #docs/infomessage-id-desc.txt
    ${CMAKE_SOURCE_DIR}/pokerth.desktop
    ${CMAKE_SOURCE_DIR}/pokerth.ico
    ${CMAKE_SOURCE_DIR}/pokerth.png
    ${CMAKE_SOURCE_DIR}/pokerth.svg
    )

qtcreator_add_project_resources(${RESOURCES} ${OTHER_FILES})

qt5_add_resources(RESOURCES_ADDED ${RESOURCES})
qt5_wrap_ui(FORMS_ADDED ${FORMS})
qt5_add_translation(QM_FILES ${TRANSLATIONS})

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_SOURCE_DIR}/pokerth.icns
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
    ${CMAKE_SOURCE_DIR}/pokerth.icns
    )

qt5_use_modules(pokerth Core Widgets Sql Network)

target_link_libraries(pokerth
    ${LIBS})

install(TARGETS pokerth
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/)
install(FILES pokerth.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps/)
install(FILES pokerth.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
