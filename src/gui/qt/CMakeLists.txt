# POKERTH.client ... Qt Widgets Client

set(CMAKE_AUTOUIC_SEARCH_PATHS . gui_800x480)

set(POKERTH_CLIENT_SRC ../../pokerth.cpp
    ../../session.cpp 
    ../../engine/local_engine/replay.cpp
    ../../core/loghelper_client.cpp
    ../../net/net_helper_client.cpp
    chattools/chattools.cpp
    guiwrapper.cpp
    qttools/qttoolswrapper.cpp
    qttools/qthelper/qthelper.cpp
    qttools/darkmodehelper.cpp
    gametable/gametableimpl.cpp
    gametable/mycardspixmaplabel.cpp
    gametable/mysetlabel.cpp
    gametable/myactionbutton.cpp
    gametable/mystatuslabel.cpp
    gametable/myslider.cpp
    gametable/myavatarlabel.cpp
    gametable/myrighttabwidget.cpp
    gametable/mylefttabwidget.cpp
    gametable/startsplash/startsplash.cpp
    gametable/log/guilog.cpp
    aboutpokerth/aboutpokerthimpl.cpp
    connecttoserverdialog/connecttoserverdialogimpl.cpp
    createnetworkgamedialog/createnetworkgamedialogimpl.cpp
    createinternetgamedialog/createinternetgamedialogimpl.cpp
    joinnetworkgamedialog/joinnetworkgamedialogimpl.cpp
    newlocalgamedialog/newgamedialogimpl.cpp
    settingsdialog/settingsdialogimpl.cpp
    settingsdialog/myavatarbutton.cpp
    settingsdialog/myhpavatarbutton.cpp
    settingsdialog/selectavatardialog/selectavatardialogimpl.cpp
    settingsdialog/selectavatardialog/myavatarlistitem.cpp
    settingsdialog/manualblindsorderdialog/manualblindsorderdialogimpl.cpp
    startnetworkgamedialog/startnetworkgamedialogimpl.cpp
    startwindow/startwindowimpl.cpp
    styles/gametablestylereader.cpp
    styles/carddeckstylereader.cpp
    changecontentdialog/changecontentdialogimpl.cpp
    changecompleteblindsdialog/changecompleteblindsdialogimpl.cpp
    mymessagedialog/mymessagedialogimpl.cpp
    gamelobbydialog/gamelobbydialogimpl.cpp
    gamelobbydialog/mygamelisttreewidget.cpp
    timeoutmsgbox/timeoutmsgboximpl.cpp
    gametable/mychancelabel.cpp
    serverlistdialog/serverlistdialogimpl.cpp
    gametable/mytimeoutlabel.cpp
    gametable/mynamelabel.cpp
    settingsdialog/mystylelistitem.cpp
    gamelobbydialog/mygamelistsortfilterproxymodel.cpp
    internetgamelogindialog/internetgamelogindialogimpl.cpp
    gamelobbydialog/mynicklistsortfilterproxymodel.cpp
    gametable/mycashlabel.cpp
    sound/soundevents.cpp
    sound/qtaudioplayer.cpp
    mymessagebox/mymessagebox.cpp
    logfiledialog/logfiledialog.cpp
    mobileinputhelper.cpp
    androidlineedit.cpp)

if(ANDROID)
    list(APPEND POKERTH_CLIENT_SRC "resources/pokerth_android.qrc")
else()
    list(APPEND POKERTH_CLIENT_SRC "resources/pokerth.qrc")
endif()

# Android benötigt qt_add_executable für deployment-settings.json
if(ANDROID)
    # Verwende qt_add_executable mit MANUAL_FINALIZATION
    qt_add_executable(pokerth_client MANUAL_FINALIZATION "${POKERTH_CLIENT_SRC}")
elseif(WIN32)
    # Windows GUI app ohne Konsolenfenster
    add_executable(pokerth_client WIN32 "${POKERTH_CLIENT_SRC}")
else()
    add_executable(pokerth_client "${POKERTH_CLIENT_SRC}")
endif()

target_include_directories(pokerth_client PRIVATE "${Boost_INCLUDE_DIRS}")
target_include_directories(pokerth_client PUBLIC ".uics/")
target_include_directories(pokerth_client PUBLIC
    ../../
    ../../config
    ../../core
    ../../engine
    ../../net
    ../../engine/local_engine
    ../../engine/network_engine
    connecttoserverdialog
    sound
    qttools
    chattools
    qttools/qthelper
    gametable
    gametable/startsplash
    gametable/log
    aboutpokerth
    createnetworkgamedialog
    createinternetgamedialog
    joinnetworkgamedialog
    newlocalgamedialog
    settingsdialog
    settingsdialog/selectavatardialog
    settingsdialog/manualblindsorderdialog
    startnetworkgamedialog
    startwindow
    serverlistdialog
    styles
    changecontentdialog
    changecompleteblindsdialog
    internetgamelogindialog
    mymessagedialog
    gamelobbydialog
    timeoutmsgbox
    logfiledialog
    mymessagebox/)

target_link_libraries(pokerth_client PRIVATE Qt6::Core Qt6::Gui Qt6::Sql Qt6::Xml Qt6::Widgets Qt6::Network Qt6::Multimedia)

# Win32 PlaySound() API for WinMM audio backend
if(WIN32)
    target_link_libraries(pokerth_client PRIVATE winmm)
endif()

target_link_libraries(pokerth_client PUBLIC pokerth_lib)
target_link_libraries(pokerth_client PUBLIC pokerth_db)

target_link_libraries(pokerth_client PUBLIC pokerth_protocol)

# Android: Konfiguriere und finalisiere Target
if(ANDROID)
    # Setze Properties VOR qt_finalize_target
    set_target_properties(pokerth_client PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION "28"
        QT_ANDROID_TARGET_SDK_VERSION "35"
    )
    
    # Finalisiere das Target (generiert deployment-settings.json)
    qt_finalize_target(pokerth_client)
endif()

if(ANDROID)
    add_definitions(-DANDROID -DGUI_800x480)
endif()

install(TARGETS pokerth_client DESTINATION bin OPTIONAL COMPONENT pokerth_client)
install(DIRECTORY ../../../data DESTINATION share/pokerth COMPONENT pokerth_client)
install(FILES ../../../pokerth.desktop DESTINATION share/applications COMPONENT pokerth_client)
install(FILES ../../../pokerth.png DESTINATION share/pixmaps COMPONENT pokerth_client)

