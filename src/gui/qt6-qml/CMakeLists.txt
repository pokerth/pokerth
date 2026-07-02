# POKERTH.qml-client ... unfinisihed

set(POKERTH_QML_CLIENT_SRC ../../pokerth.cpp
    ../../session.cpp 
    ../../core/loghelper_client.cpp
    ../../net/net_helper_client.cpp
    # Server files needed for embedded local network game server
    ../../net/serverexception.cpp
    ../../net/serveracceptinterface.cpp
    ../../net/serveracceptwebhelper.cpp
    ../../net/servergame.cpp
    ../../net/servergamestate.cpp
    ../../net/serverlobbythread.cpp
    ../../net/serverbanmanager.cpp
    ../../net/servermanager.cpp
    ../../net/servermanagerfactoryclient.cpp
    ../../core/qttools/qthelper/qthelper.cpp
    ../../core/qttools/qttoolswrapper.cpp
    ../../core/qttoolsinterface.cpp
    cpp/retranslate.cpp
    cpp/settingsxmlhandler.cpp
    cpp/settingsmanager.cpp
    cpp/styleprovider.cpp
    cpp/serverconnectionhandler.cpp
    cpp/lobbyhandler.cpp
    cpp/gamehandler.cpp
    cpp/loghandler.cpp
    cpp/networkgamehandler.cpp
    cpp/qmlguiinterface.cpp
    cpp/screenhelper.cpp
    cpp/webnetworkfactory.cpp
    ../qt/sound/soundevents.cpp
    ../qt/sound/qtaudioplayer.cpp)

qt_add_resources(POKERTH_QML_CLIENT_SRC qml.qrc)
qt_add_resources(POKERTH_QML_CLIENT_SRC cflags.qrc)
qt_add_resources(POKERTH_QML_CLIENT_SRC emotes.qrc)

# SvgIcon: versionsabhängiger SVG-Wrapper. Qt >= 6.8 nutzt das echte
# QtQuick.VectorImage (vektoriell, scharf); ältere Qt (z.B. 6.7 für den
# Android-8-Build) haben das Modul nicht -> Image-basierter Raster-Fallback.
# Die passende .qml-Datei wird unter dem festen Alias components/SvgIcon.qml
# ins Resource eingebunden, sodass die Aufrufstellen unverändert "SvgIcon { }"
# verwenden. (Ein einzelnes SvgIcon.qml ginge nicht: "import QtQuick.VectorImage"
# scheitert auf 6.7 schon beim Laden, daher zwei getrennte Dateien.)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8.0)
    set(SVGICON_VARIANT "${CMAKE_CURRENT_SOURCE_DIR}/components/SvgIconVector.qml")
else()
    set(SVGICON_VARIANT "${CMAKE_CURRENT_SOURCE_DIR}/components/SvgIconImage.qml")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/svgicon.qrc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/svgicon.qrc" @ONLY)
qt_add_resources(POKERTH_QML_CLIENT_SRC "${CMAKE_CURRENT_BINARY_DIR}/svgicon.qrc")

# Android: Das datei-basierte QML-Stil-System (StyleProvider/SettingsManager)
# scannt <AppDataDir>gfx/qml/..., und AppDataDir ist auf Android das Qt-Resource
# :/android/android-data/ (configfile.cpp). Ohne dieses Bundle gibt es auf Android
# kein echtes Dateisystem-data/ → keine Tisch-/Karten-/Rückseiten-Stile.
# Auf Desktop werden die Stile aus dem installierten data/ gelesen (install() unten).
if(ANDROID)
    qt_add_resources(POKERTH_QML_CLIENT_SRC resources/pokerth_qml_android_data.qrc)
endif()

set(QML_FILES
    pokerth.qml
    components/AppText.qml
    components/AppLabel.qml
    components/BetChip.qml
    components/BlindButtonImage.qml
    components/PlayerActionBadge.qml
    components/PlayerTimeoutBar.qml
    components/PlayerBoxBackground.qml
    components/PlayerTurnGlow.qml
    components/PlayerWinnerOverlay.qml
    components/SettingsHeader.qml
    components/ConfigCheckBox.qml
    components/RankingFilterField.qml
    components/CustomCheckBox.qml
    components/CustomComboBox.qml
    components/CustomTabBar.qml
    components/CustomToggle.qml
    components/CustomSpinBox.qml
    components/AvatarCardRow.qml
    components/GamePlayerBox.qml
    components/GamePlayerSelfBox.qml
    components/EmojiPicker.qml
    components/GuiSettings.qml
    components/InternetGameSettings.qml
    components/LocalGameSettings.qml
    components/LogsSettings.qml
    components/NetworkGameSettings.qml
    components/NicknameAvatarSettings.qml
    components/ResetSettings.qml
    components/SideMenu.qml
    components/SoundSettings.qml
    components/CustomButton.qml
    components/StyleSettings.qml
    components/StyleCard.qml
    components/LobbyStatsBar.qml
    pages/AboutPage.qml
    pages/GamePage.qml
    pages/InternetGamePage.qml
    pages/LocalGamePage.qml
    pages/LogsPage.qml
    pages/NetworkGameCreatePage.qml
    pages/NetworkGameEnterPage.qml
    pages/PreLoader.qml
    pages/SettingsPage.qml
    pages/StartPage.qml
    config/StaticData.qml
    config/Parameters.qml
    config/Responsive.qml
    config/Theme.qml)

set(TS_FILES
    i18n/pokerth_de_DE.ts
    i18n/pokerth_fr_FR.ts
    i18n/pokerth_en_US.ts
    i18n/pokerth_it_IT.ts
    i18n/pokerth_es_ES.ts
    i18n/pokerth_pt_PT.ts
    i18n/pokerth_pt_BR.ts
)

qt_add_translations(pokerth_qml-client
    TS_FILES        ${TS_FILES}
    SOURCES         ${POKERTH_QML_CLIENT_SRC} ${QML_FILES}
    RESOURCE_PREFIX "/i18n"
)

# Für Android muss qt_add_executable mit MANUAL_FINALIZATION verwendet werden
if(ANDROID)
    qt_add_executable(pokerth_qml-client MANUAL_FINALIZATION ${POKERTH_QML_CLIENT_SRC})
    target_include_directories(pokerth_qml-client PRIVATE "${Protobuf_INCLUDE_DIR}")
    target_link_libraries(pokerth_qml-client PUBLIC protobuf::libprotobuf)
elseif(WIN32)
    # Windows GUI app ohne Konsolenfenster. pokerth.rc bettet pokerth.ico als
    # App-Icon in die .exe ein (Explorer, Taskbar, Titelleiste). windres findet
    # die in der .rc relativ referenzierte .ico über den Include-Pfad auf den
    # Repo-Root; der Pfad zur .rc selbst ist absolut (CWD-unabhängig).
    set_source_files_properties("${CMAKE_SOURCE_DIR}/pokerth.rc" PROPERTIES
        COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}")
    add_executable(pokerth_qml-client WIN32 ${POKERTH_QML_CLIENT_SRC} "${CMAKE_SOURCE_DIR}/pokerth.rc")
else()
    add_executable(pokerth_qml-client ${POKERTH_QML_CLIENT_SRC})
endif()

target_include_directories(pokerth_qml-client PUBLIC
    ../../config
    ../../core
    ../../core
    ../../core/qttools
    ../../core/qttools/qthelper
    ../../
    ../../engine
    ../../engine/local_engine
    ../../engine/network_engine
    ../../net
    ../
    ../qt/sound
    .
    cpp)

# Fix für Android: Abseil-Bibliotheken die von Protobuf benötigt werden
if(ANDROID)
    # Suche nach verfügbaren Abseil-Bibliotheken (optional)
    find_library(ABSL_RAW_HASH_SET absl_raw_hash_set PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib NO_DEFAULT_PATH)
    find_library(ABSL_HASHTABLEZ_SAMPLER absl_hashtablez_sampler PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib NO_DEFAULT_PATH)
    find_library(ABSL_LOW_LEVEL_HASH absl_low_level_hash PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib NO_DEFAULT_PATH)
    
    # Erstelle eine Liste der gefundenen Bibliotheken
    set(ABSL_EXTRA_LIBS)
    if(ABSL_RAW_HASH_SET)
        list(APPEND ABSL_EXTRA_LIBS ${ABSL_RAW_HASH_SET})
        message(STATUS "Found absl_raw_hash_set: ${ABSL_RAW_HASH_SET}")
    endif()
    if(ABSL_HASHTABLEZ_SAMPLER)
        list(APPEND ABSL_EXTRA_LIBS ${ABSL_HASHTABLEZ_SAMPLER})
        message(STATUS "Found absl_hashtablez_sampler: ${ABSL_HASHTABLEZ_SAMPLER}")
    endif()
    if(ABSL_LOW_LEVEL_HASH)
        list(APPEND ABSL_EXTRA_LIBS ${ABSL_LOW_LEVEL_HASH})
        message(STATUS "Found absl_low_level_hash: ${ABSL_LOW_LEVEL_HASH}")
    endif()
    
    # Linke die gefundenen Bibliotheken
    if(ABSL_EXTRA_LIBS)
        target_link_libraries(pokerth_qml-client PRIVATE ${ABSL_EXTRA_LIBS})
    else()
        message(WARNING "No additional Abseil libraries found - linking may fail")
    endif()
endif()

target_link_libraries(pokerth_qml-client PUBLIC pokerth_lib)
target_link_libraries(pokerth_qml-client PUBLIC pokerth_db)

target_link_libraries(pokerth_qml-client PUBLIC pokerth_protocol)
target_link_libraries(pokerth_qml-client PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Sql Qt6::Xml Qt6::Svg Qt6::QuickControls2 Qt6::Widgets Qt6::Multimedia)

# Win32: WinMM (waveOut*) + Mswsock (AcceptEx via boost::asio)
if(WIN32)
    target_link_libraries(pokerth_qml-client PRIVATE winmm mswsock)
endif()

target_compile_definitions(pokerth_qml-client PUBLIC QML_CLIENT)

# qDebug()-Aufrufe der Client-eigenen Quellen (z. B. die per-Aktion-
# Instrumentierung [ACTDBG] im GameHandler aus der gelösten Freeze-Untersuchung)
# zu Compile-Zeit zu No-ops machen → keine String-Formatierung/I/O pro Aktion
# im Spiel. qInfo/qWarning/qCritical/qFatal bleiben erhalten.
target_compile_definitions(pokerth_qml-client PRIVATE QT_NO_DEBUG_OUTPUT)

# Android: Konfiguriere und finalisiere Target
if(ANDROID)
    # Setze Properties VOR qt_finalize_target
    set_target_properties(pokerth_qml-client PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION "28"
        QT_ANDROID_TARGET_SDK_VERSION "35"
    )
    
    # Finalisiere das Target (generiert deployment-settings.json)
    qt_finalize_target(pokerth_qml-client)
endif()

install(TARGETS pokerth_qml-client DESTINATION bin OPTIONAL)
install(DIRECTORY ../../../data DESTINATION share/pokerth)
install(FILES ../../../pokerth_qml.desktop DESTINATION share/applications)
install(FILES ../../../pokerth.png DESTINATION share/pixmaps)
