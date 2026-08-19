# POKERTH.qml-client ... unfinisihed

set(POKERTH_QML_CLIENT_SRC ../../pokerth.cpp
    ../../session.cpp 
    ../../core/loghelper_client.cpp
    ../../net/net_helper_client.cpp
    # Server files needed for embedded local network game server
    ../../net/serverexception.cpp
    ../../net/serveracceptinterface.cpp
    ../../net/serveracceptwebhelper.cpp
    ../../net/servertlsutil.cpp
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
    cpp/settingsmanager.cpp
    cpp/ziputils.cpp
    cpp/styleprovider.cpp
    cpp/serverconnectionhandler.cpp
    cpp/lobbyhandler.cpp
    cpp/gamehandler.cpp
    cpp/chattranslator.cpp
    cpp/texttranslator.cpp
    ../chattranslator/chattranslatorcore.cpp
    cpp/loghandler.cpp
    cpp/networkgamehandler.cpp
    cpp/qmlguiinterface.cpp
    cpp/screenhelper.cpp
    cpp/androidconnectionservice.cpp
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
#
# Die Dateiliste wird zur Configure-Zeit aus data/ generiert (frueher eine von Hand
# gepflegte .qrc, die bei jedem neuen Stil vergessen wurde). CONFIGURE_DEPENDS laesst
# CMake bei neuen/entfernten Dateien selbst neu konfigurieren, damit neue Kartendecks,
# Rueckseiten und Tisch-Stile ohne Zutun im APK landen.
if(ANDROID)
    set(POKERTH_ANDROID_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../data")
    file(GLOB_RECURSE POKERTH_ANDROID_DATA_FILES CONFIGURE_DEPENDS
         "${POKERTH_ANDROID_DATA_DIR}/gfx/avatars/*"
         "${POKERTH_ANDROID_DATA_DIR}/gfx/qml/*"
         # misc/cacert.pem: Rueckfall-Wurzelzertifikate fuer den HTTPS-Download
         # der Serverliste, falls der Android-Trust-Store nicht lesbar ist
         # (siehe LoadSystemCaCertificates in downloadhelper.cpp).
         "${POKERTH_ANDROID_DATA_DIR}/misc/*")
    list(SORT POKERTH_ANDROID_DATA_FILES)

    set(POKERTH_ANDROID_DATA_ENTRIES "")
    foreach(_data_file IN LISTS POKERTH_ANDROID_DATA_FILES)
        file(RELATIVE_PATH _data_rel "${POKERTH_ANDROID_DATA_DIR}" "${_data_file}")
        string(APPEND POKERTH_ANDROID_DATA_ENTRIES
               "        <file alias=\"android-data/${_data_rel}\">${_data_file}</file>\n")
    endforeach()

    list(LENGTH POKERTH_ANDROID_DATA_FILES _data_count)
    message(STATUS "Android data bundle: ${_data_count} files from ${POKERTH_ANDROID_DATA_DIR}")

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/resources/pokerth_qml_android_data.qrc.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/pokerth_qml_android_data.qrc" @ONLY)
    qt_add_resources(POKERTH_QML_CLIENT_SRC "${CMAKE_CURRENT_BINARY_DIR}/pokerth_qml_android_data.qrc")
endif()

set(QML_FILES
    pokerth.qml
    components/AppCheckBox.qml
    components/AppLabel.qml
    components/AppText.qml
    components/AvatarCardRow.qml
    components/BetChip.qml
    components/BlindButtonImage.qml
    components/BrandHeader.qml
    components/CardImage.qml
    components/ChatBox.qml
    components/CommunityCards.qml
    components/CommunityPlayerView.qml
    components/CommunityRankingView.qml
    components/CommunitySwitch.qml
    components/ConfigCheckBox.qml
    components/ConfirmPopup.qml
    components/CustomButton.qml
    components/CustomCheckBox.qml
    components/CustomComboBox.qml
    components/CustomSpinBox.qml
    components/CustomTabBar.qml
    components/CustomToggle.qml
    components/EmojiPicker.qml
    components/ExampleAvatarPicker.qml
    components/ForumBadge.qml
    components/GameActionBar.qml
    components/GameAdminBadge.qml
    components/GameInfoPanel.qml
    components/GameListItem.qml
    components/GamePlayerBox.qml
    components/GamePlayerSelfBox.qml
    components/GameReactionFx.qml
    components/GameRoundIconButton.qml
    components/GameSidePanel.qml
    components/GameStatusBar.qml
    components/GlobalNoticePopup.qml
    components/GuiSettings.qml
    components/InternetGameSettings.qml
    components/LobbyStatsBar.qml
    components/LocalGameSettings.qml
    components/LogsSettings.qml
    components/NetworkGameSettings.qml
    components/NicknameAvatarSettings.qml
    components/PanelShadow.qml
    components/PlacementPieChart.qml
    components/PlacementResult.qml
    components/PlayerActionBadge.qml
    components/PlayerActionIcon.qml
    components/PlayerBoxBackground.qml
    components/PlayerListItem.qml
    components/PlayerSeasonCard.qml
    components/PlayerTimeoutBar.qml
    components/PlayerTurnGlow.qml
    components/PlayerWinnerOverlay.qml
    components/RankingFilterField.qml
    components/RankingHeaderCell.qml
    components/ReactionPicker.qml
    components/ResetSettings.qml
    components/ResizeHandle.qml
    components/SeasonStatsSection.qml
    components/SectionDivider.qml
    components/SettingsHeader.qml
    components/SideMenu.qml
    components/SoundSettings.qml
    components/StartFooter.qml
    components/StatTile.qml
    components/StyleCard.qml
    components/StyleSettings.qml
    components/SvgIconImage.qml
    components/SvgIconVector.qml
    components/WinningHandBadge.qml
    pages/AboutPage.qml
    pages/BbcRankingPage.qml
    pages/CommunityRankingPage.qml
    pages/ForumNewsPage.qml
    pages/ForumPostPage.qml
    pages/GamePage.qml
    pages/GameTableStatsPage.qml
    pages/GameWaitPage.qml
    pages/InternetGamePage.qml
    pages/LobbyCreateGamePage.qml
    pages/LobbyPage.qml
    pages/LocalGamePage.qml
    pages/LogsPage.qml
    pages/NetworkGameCreatePage.qml
    pages/NetworkGameEnterPage.qml
    pages/PokerthPlayerPage.qml
    pages/PreLoader.qml
    pages/RankingPage.qml
    pages/ServerConnectionDialog.qml
    pages/SettingsPage.qml
    pages/StartPage.qml
    pages/WecRankingPage.qml
    config/BotSuggest.qml
    config/Community.qml
    config/ForumNews.qml
    config/Parameters.qml
    config/Responsive.qml
    config/StaticData.qml
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
elseif(IOS)
    # iOS: Qt liegt als STATISCHE Bibliothek vor. Ein statisches Qt linkt keine
    # Plugins von sich aus – weder die QPA-Plattform noch die QML-Plugins von
    # QtQuick/Controls/Effects/VectorImage. Genau das erledigt qt_finalize_target()
    # (unten): es laesst qmlimportscanner ueber das Target-Quellverzeichnis laufen
    # und linkt die gefundenen Module dazu. Ohne diesen Weg (also mit einem
    # nackten add_executable) baut die App zwar, findet zur Laufzeit aber weder
    # Plattform-Plugin noch "module QtQuick is not installed".
    qt_add_executable(pokerth_qml-client MANUAL_FINALIZATION MACOSX_BUNDLE ${POKERTH_QML_CLIENT_SRC})
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
    ../chattranslator
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

# miniz: Public-Domain-Single-File-ZIP-Bibliothek für Stil-Import/-Export (.zip).
# Nötig, weil der Flatpak-Portal-Dateidialog nur die EINE gewählte Datei in die
# Sandbox reicht – ein ganzer Stil-Ordner (52 Karten + XML + Preview) ist so nicht
# importierbar, ein einzelnes Archiv dagegen schon. MINIZ_NO_ZLIB_COMPATIBLE_NAMES:
# ohne dieses Define exportiert miniz zlib-Namen (compress/inflate/crc32/…), die mit
# dem über boost::iostreams gelinkten echten zlib kollidieren würden.
# MINIZ_NO_STDIO: ziputils.cpp arbeitet ausschließlich im Speicher (init_mem/init_heap),
# das Datei-I/O von miniz wird nie benutzt – ohne dieses Define wählt miniz dafür je nach
# Plattform einen fopen/ftello-Pfad und warnt per #pragma message vor fehlendem LFS.
add_library(miniz STATIC ${CMAKE_SOURCE_DIR}/src/third_party/miniz/miniz.c)
target_include_directories(miniz PUBLIC ${CMAKE_SOURCE_DIR}/src/third_party/miniz)
target_compile_definitions(miniz PUBLIC MINIZ_NO_ZLIB_COMPATIBLE_NAMES MINIZ_NO_STDIO)
set_target_properties(miniz PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(pokerth_qml-client PUBLIC pokerth_lib)
target_link_libraries(pokerth_qml-client PUBLIC pokerth_db)
target_link_libraries(pokerth_qml-client PRIVATE miniz)

target_link_libraries(pokerth_qml-client PUBLIC pokerth_protocol)
target_link_libraries(pokerth_qml-client PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Sql Qt6::Xml Qt6::Svg Qt6::QuickControls2 Qt6::Widgets Qt6::Multimedia Qt6::Network)

# Win32: WinMM (waveOut*) + Mswsock (AcceptEx via boost::asio)
if(WIN32)
    target_link_libraries(pokerth_qml-client PRIVATE winmm mswsock)
endif()

target_compile_definitions(pokerth_qml-client PUBLIC QML_CLIENT)

# qDebug()-Aufrufe der Client-eigenen Quellen (z. B. die per-Aktion-
# Instrumentierung [ACTDBG] im GameHandler aus der gelösten Freeze-Untersuchung)
# zu Compile-Zeit zu No-ops machen → keine String-Formatierung/I/O pro Aktion
# im Spiel. qInfo/qWarning/qCritical/qFatal bleiben erhalten.
#
# POKERTH_DIAG_LOG=ON kehrt das für Diagnose-Builds um: qDebug() bleibt
# übersetzt UND der Message-Handler in pokerth.cpp laesst QtDebugMsg durch
# (beides ist noetig – sonst gibt es entweder keine Aufrufe oder sie werden
# verworfen). Damit landen die [ACTDBG]-Zeilen in pokerth-debug.log und auf
# stderr (auf iOS live in der Konsole.app). Nur fuer die Fehlersuche
# einschalten: die Instrumentierung schreibt pro Spieler-Aktion synchron.
option(POKERTH_DIAG_LOG "Diagnose-Build: [ACTDBG]-qDebug-Instrumentierung aktiv" OFF)
if(POKERTH_DIAG_LOG)
    target_compile_definitions(pokerth_qml-client PRIVATE POKERTH_DIAG_LOG)
    message(STATUS "PokerTH: Diagnose-Logging AKTIV ([ACTDBG] qDebug wird geloggt)")
else()
    target_compile_definitions(pokerth_qml-client PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

# iOS: App-Bundle konfigurieren und finalisieren
if(IOS)
    # iosbackgroundsession.mm ist Objective-C++ (UIKit-Zugriff). Ohne
    # enable_language(OBJCXX) wuerde CMake die .mm-Datei als reines C++
    # uebersetzen und an den #import-Direktiven scheitern.
    enable_language(OBJCXX)
    target_sources(pokerth_qml-client PRIVATE cpp/iosbackgroundsession.mm)
    # ARC (Automatic Reference Counting) fuer diese Datei: die von
    # addObserverForName: gelieferten Observer-Objekte sind autoreleased. Ohne
    # ARC wuerden die globalen Referenzen darauf ins Leere zeigen, sobald der
    # Autorelease-Pool laeuft (Absturz beim removeObserver:). Mit ARC haelt die
    # starke globale Variable das Objekt korrekt am Leben.
    set_source_files_properties(cpp/iosbackgroundsession.mm PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc")
    # screenhelper.cpp enthaelt auf iOS einen UIKit-Zweig (idleTimerDisabled,
    # verhindert Bildschirmsperre waehrend des Spiels). Die Datei bleibt eine
    # .cpp – der Android-Zweig und die gemeinsame Klasse stehen dort –, wird auf
    # iOS aber als Objective-C++ uebersetzt, damit #import/UIKit funktioniert.
    set_source_files_properties(cpp/screenhelper.cpp PROPERTIES LANGUAGE OBJCXX)

    # UIApplication/NSNotificationCenter: UIKit + Foundation.
    target_link_libraries(pokerth_qml-client PRIVATE
        "-framework UIKit" "-framework Foundation")

    # Gleiche App-Identität wie auf Android (net.pokerth.PokerTH_QML). Für den
    # unsignierten Sideload-Flow (Sideloadly/AltStore re-signieren) funktioniert
    # der Unterstrich. Hinweis: Apple erlaubt in der CFBundleIdentifier offiziell
    # nur A–Z a–z 0–9 . und - — bei einer echten App-Store-Einreichung müsste der
    # "_" zu "-" werden (net.pokerth.PokerTH-QML).
    set(POKERTH_IOS_BUNDLE_ID "net.pokerth.PokerTH_QML" CACHE STRING "CFBundleIdentifier der iOS-App")
    set(POKERTH_IOS_VERSION   "2.1.7"           CACHE STRING "CFBundleShortVersionString der iOS-App")
    set(POKERTH_IOS_BUILD     "27"              CACHE STRING "CFBundleVersion (Build-Nummer) der iOS-App")

    # Asset-Katalog mit dem App-Icon (1024er PNG, ohne Alpha – actool lehnt
    # transparente iOS-App-Icons ab). Xcode kompiliert ihn in Assets.car.
    set(POKERTH_IOS_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets")
    target_sources(pokerth_qml-client PRIVATE "${POKERTH_IOS_ASSETS}")
    set_source_files_properties("${POKERTH_IOS_ASSETS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set_target_properties(pokerth_qml-client PROPERTIES
        # Bundle heisst PokerTH.app (nicht pokerth_qml-client.app); der Name steht
        # auf dem Home-Screen unter dem Icon.
        OUTPUT_NAME "PokerTH"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
        QT_IOS_LAUNCH_SCREEN     "${CMAKE_CURRENT_SOURCE_DIR}/ios/LaunchScreen.storyboard"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${POKERTH_IOS_BUNDLE_ID}"
        XCODE_ATTRIBUTE_MARKETING_VERSION         "${POKERTH_IOS_VERSION}"
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION   "${POKERTH_IOS_BUILD}"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"   # iPhone + iPad
    )

    # data/ ins App-Bundle kopieren. Anders als auf Android braucht es dafuer KEIN
    # Qt-Resource: iOS hat ein echtes Dateisystem, und getDataPathStdString()
    # (qthelper.cpp) liefert im __APPLE__-Zweig fuer alles, was nicht auf
    # Contents/MacOS endet, bereits "<applicationDirPath>/data/" – bei einem
    # flachen iOS-Bundle also <PokerTH.app>/data/. Die Dateien muessen daher nur
    # dorthin, und AppDataDir/StyleProvider funktionieren unveraendert.
    # POST_BUILD statt ~1000 Einzeldateien als Bundle-Resources: das haelt die
    # Xcode-Projektdatei klein. Xcode signiert das Bundle erst nach allen
    # Build-Phasen, ein spaeter ergaenztes Signieren erfasst die Daten also mit.
    add_custom_command(TARGET pokerth_qml-client POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_SOURCE_DIR}/data"
                "$<TARGET_BUNDLE_CONTENT_DIR:pokerth_qml-client>/data"
        COMMENT "Kopiere data/ (Stile, Karten, Sounds, Fonts) ins iOS-App-Bundle")

    # Multimedia-Backend: QtMultimedia hat seit 6.5 FFmpeg als Default-Backend, und
    # das statische Qt linkt dessen Plugin automatisch mit ein. Die dazugehoerigen
    # FFmpeg-Bibliotheken (libavcodec/libswscale/…) liefert das aqt-iOS-Paket aber
    # NICHT mit → der Link scheitert an undefinierten Symbolen (sws_scale, …).
    # Das Darwin-Backend (AVFoundation) ist auf iOS der native Weg, ist im
    # Qt-iOS-Paket enthalten und reicht fuer die WAV-Sounds des Clients vollstaendig.
    # INCLUDE_BY_TYPE ersetzt die Default-Pluginliste dieses Typs, das FFmpeg-Plugin
    # bleibt damit aussen vor.
    if(NOT TARGET Qt6::QDarwinMediaPlugin)
        message(FATAL_ERROR
            "Qt6::QDarwinMediaPlugin nicht gefunden. Ohne das AVFoundation-Backend "
            "wuerde das FFmpeg-Plugin gelinkt, dessen Bibliotheken die Qt-iOS-"
            "Installation nicht enthaelt. Qt mit dem Modul qtmultimedia installieren.")
    endif()
    qt_import_plugins(pokerth_qml-client
        INCLUDE_BY_TYPE multimedia Qt6::QDarwinMediaPlugin)

    qt_finalize_target(pokerth_qml-client)
endif()

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
