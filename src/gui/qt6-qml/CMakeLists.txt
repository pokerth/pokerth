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

# SvgIcon: a version dependent SVG wrapper. Qt >= 6.8 uses the real
# QtQuick.VectorImage (vectorial, sharp); older Qt (e.g. 6.7 for the
# Android 8 build) does not have the module -> an image based raster fallback.
# The matching .qml file is embedded into the resource under the fixed alias
# components/SvgIcon.qml, so that the call sites use "SvgIcon { }" unchanged.
# (A single SvgIcon.qml would not work: "import QtQuick.VectorImage"
# already fails on 6.7 when loading, hence two separate files.)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.8.0)
    set(SVGICON_VARIANT "${CMAKE_CURRENT_SOURCE_DIR}/components/SvgIconVector.qml")
else()
    set(SVGICON_VARIANT "${CMAKE_CURRENT_SOURCE_DIR}/components/SvgIconImage.qml")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/svgicon.qrc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/svgicon.qrc" @ONLY)
qt_add_resources(POKERTH_QML_CLIENT_SRC "${CMAKE_CURRENT_BINARY_DIR}/svgicon.qrc")

# Android: the file based QML style system (StyleProvider/SettingsManager)
# scans <AppDataDir>gfx/qml/..., and on Android AppDataDir is the Qt resource
# :/android/android-data/ (configfile.cpp). Without this bundle there is no real
# file system data/ on Android → no table/card/card back styles.
# On the desktop the styles are read from the installed data/ (install() below).
#
# The file list is generated from data/ at configure time (previously a hand
# maintained .qrc that was forgotten with every new style). CONFIGURE_DEPENDS lets
# CMake reconfigure by itself on new/removed files, so that new card decks,
# card backs and table styles end up in the APK without any action.
if(ANDROID)
    set(POKERTH_ANDROID_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../data")
    file(GLOB_RECURSE POKERTH_ANDROID_DATA_FILES CONFIGURE_DEPENDS
         "${POKERTH_ANDROID_DATA_DIR}/gfx/avatars/*"
         "${POKERTH_ANDROID_DATA_DIR}/gfx/qml/*"
         # misc/cacert.pem: fallback root certificates for the HTTPS download
         # of the server list, in case the Android trust store is not readable
         # (see LoadSystemCaCertificates in downloadhelper.cpp).
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
    components/SafeAreaInsets.qml
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
    components/SeatBox.qml
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
    components/PlayerBetStrip.qml
    components/PlayerBoxBackground.qml
    components/PlayerListItem.qml
    components/PlayerNoteBadge.qml
    components/PlayerNoteDialog.qml
    components/PlayerRatingStars.qml
    components/PlayerSeasonCard.qml
    components/PlayerTimeoutBar.qml
    components/PlayerTurnGlow.qml
    components/PlayerWinnerOverlay.qml
    components/PrivateMessageDialog.qml
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
    config/ReactionCatalog.qml
    config/Responsive.qml
    config/SeatStyle.qml
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
    i18n/pokerth_ru_RU.ts
    i18n/pokerth_pl_PL.ts
    i18n/pokerth_tr_TR.ts
    i18n/pokerth_nl_NL.ts
    i18n/pokerth_el_GR.ts
    i18n/pokerth_hu_HU.ts
    i18n/pokerth_cs_CZ.ts
    i18n/pokerth_sv_SE.ts
    i18n/pokerth_da_DK.ts
    i18n/pokerth_ca_ES.ts
    i18n/pokerth_bg_BG.ts
    i18n/pokerth_fi_FI.ts
    i18n/pokerth_nb_NO.ts
    i18n/pokerth_sk_SK.ts
    i18n/pokerth_lt_LT.ts
    i18n/pokerth_gl_ES.ts
    i18n/pokerth_gd_GB.ts
    i18n/pokerth_ja_JP.ts
    i18n/pokerth_ta_IN.ts
    i18n/pokerth_vi_VN.ts
    i18n/pokerth_af_ZA.ts
    i18n/pokerth_zh_CN.ts
)

qt_add_translations(pokerth_qml-client
    TS_FILES        ${TS_FILES}
    SOURCES         ${POKERTH_QML_CLIENT_SRC} ${QML_FILES}
    RESOURCE_PREFIX "/i18n"
)

# For Android, qt_add_executable has to be used with MANUAL_FINALIZATION
if(ANDROID)
    qt_add_executable(pokerth_qml-client MANUAL_FINALIZATION ${POKERTH_QML_CLIENT_SRC})
    target_include_directories(pokerth_qml-client PRIVATE "${Protobuf_INCLUDE_DIR}")
    target_link_libraries(pokerth_qml-client PUBLIC protobuf::libprotobuf)
elseif(IOS)
    # iOS: Qt is present as a STATIC library. A static Qt links no
    # plugins by itself – neither the QPA platform nor the QML plugins of
    # QtQuick/Controls/Effects/VectorImage. That is exactly what qt_finalize_target()
    # (below) does: it runs qmlimportscanner over the target source directory
    # and links the modules it finds. Without this path (i.e. with a
    # bare add_executable) the app does build, but at runtime it finds neither the
    # platform plugin nor "module QtQuick is not installed".
    qt_add_executable(pokerth_qml-client MANUAL_FINALIZATION MACOSX_BUNDLE ${POKERTH_QML_CLIENT_SRC})
elseif(WIN32)
    # Windows GUI app without a console window. pokerth.rc embeds pokerth.ico as
    # the app icon into the .exe (explorer, taskbar, title bar). windres finds
    # the .ico referenced relatively in the .rc via the include path pointing to
    # the repo root; the path to the .rc itself is absolute (independent of the CWD).
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

# A fix for Android: Abseil libraries needed by protobuf
if(ANDROID)
    # Look for available Abseil libraries (optional)
    find_library(ABSL_RAW_HASH_SET absl_raw_hash_set PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib NO_DEFAULT_PATH)
    find_library(ABSL_HASHTABLEZ_SAMPLER absl_hashtablez_sampler PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib NO_DEFAULT_PATH)
    find_library(ABSL_LOW_LEVEL_HASH absl_low_level_hash PATHS ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib NO_DEFAULT_PATH)
    
    # Create a list of the libraries that were found
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
    
    # Link the libraries that were found
    if(ABSL_EXTRA_LIBS)
        target_link_libraries(pokerth_qml-client PRIVATE ${ABSL_EXTRA_LIBS})
    else()
        message(WARNING "No additional Abseil libraries found - linking may fail")
    endif()
endif()

# miniz: a public domain single file ZIP library for the style import/export (.zip).
# Necessary because the Flatpak portal file dialog only passes the ONE chosen file into
# the sandbox – a whole style folder (52 cards + XML + preview) cannot be
# imported that way, a single archive can. MINIZ_NO_ZLIB_COMPATIBLE_NAMES:
# without this define miniz exports zlib names (compress/inflate/crc32/…) that would collide
# with the real zlib linked via boost::iostreams.
# MINIZ_NO_STDIO: ziputils.cpp works exclusively in memory (init_mem/init_heap),
# the file I/O of miniz is never used – without this define miniz chooses a
# fopen/ftello path for it depending on the platform and warns via #pragma message about a missing LFS.
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

# -rdynamic: it puts the symbols of the client into the dynamic symbol table, so that
# the crash handler (loghelper_install_crash_handler, see loghelper_client.cpp)
# can name our own functions in the backtrace as well and not only those of the Qt
# libraries. It costs only a few KB in the binary. Not on Android (no
# backtrace() in bionic, the handler is inactive there anyway) and not on Windows.
if(UNIX AND NOT ANDROID)
    target_link_options(pokerth_qml-client PRIVATE -rdynamic)
endif()

# Turn qDebug() calls of the client's own sources (e.g. the per-action
# instrumentation [ACTDBG] in the GameHandler from the solved freeze investigation)
# into no-ops at compile time → no string formatting/I/O per action
# in the game. qInfo/qWarning/qCritical/qFatal are kept.
#
# POKERTH_DIAG_LOG=ON reverses that for diagnostic builds: qDebug() stays
# compiled AND the message handler in pokerth.cpp lets QtDebugMsg through
# (both are necessary – otherwise there are either no calls or they are
# discarded). That way the [ACTDBG] lines end up in pokerth-debug.log and on
# stderr (on iOS live in Console.app). Only switch it on for
# troubleshooting: the instrumentation writes synchronously per player action.
option(POKERTH_DIAG_LOG "Diagnostic build: [ACTDBG] qDebug instrumentation active" OFF)
if(POKERTH_DIAG_LOG)
    target_compile_definitions(pokerth_qml-client PRIVATE POKERTH_DIAG_LOG)
    message(STATUS "PokerTH: diagnostic logging ACTIVE ([ACTDBG] qDebug is logged)")
else()
    target_compile_definitions(pokerth_qml-client PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

# iOS: configure and finalize the app bundle
if(IOS)
    # iosbackgroundsession.mm is Objective-C++ (UIKit access). Without
    # enable_language(OBJCXX) CMake would compile the .mm file as pure C++
    # and fail on the #import directives.
    enable_language(OBJCXX)
    target_sources(pokerth_qml-client PRIVATE cpp/iosbackgroundsession.mm)
    # ARC (automatic reference counting) for this file: the observer objects
    # delivered by addObserverForName: are autoreleased. Without
    # ARC the global references to them would point nowhere as soon as the
    # autorelease pool runs (a crash in removeObserver:). With ARC the
    # strong global variable keeps the object alive correctly.
    set_source_files_properties(cpp/iosbackgroundsession.mm PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc")
    # On iOS screenhelper.cpp contains a UIKit branch (idleTimerDisabled,
    # it prevents the screen lock during the game). The file stays a
    # .cpp – the Android branch and the shared class are in it –, but on
    # iOS it is compiled as Objective-C++, so that #import/UIKit works.
    set_source_files_properties(cpp/screenhelper.cpp PROPERTIES LANGUAGE OBJCXX)

    # UIApplication/NSNotificationCenter: UIKit + Foundation.
    target_link_libraries(pokerth_qml-client PRIVATE
        "-framework UIKit" "-framework Foundation")

    # The same app identity as on Android (net.pokerth.PokerTH_QML). For the
    # unsigned sideload flow (Sideloadly/AltStore re-sign it) the underscore
    # works. Note: officially Apple only allows A–Z a–z 0–9 . and - in the
    # CFBundleIdentifier — for a real App Store submission the
    # "_" would have to become a "-" (net.pokerth.PokerTH-QML).
    set(POKERTH_IOS_BUNDLE_ID "net.pokerth.PokerTH_QML" CACHE STRING "CFBundleIdentifier of the iOS app")
    set(POKERTH_IOS_VERSION   "2.1.8"           CACHE STRING "CFBundleShortVersionString of the iOS app")
    set(POKERTH_IOS_BUILD     "28"              CACHE STRING "CFBundleVersion (build number) of the iOS app")

    # An asset catalogue with the app icon (a 1024 PNG, without alpha – actool rejects
    # transparent iOS app icons). Xcode compiles it into Assets.car.
    set(POKERTH_IOS_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/ios/Assets.xcassets")
    target_sources(pokerth_qml-client PRIVATE "${POKERTH_IOS_ASSETS}")
    set_source_files_properties("${POKERTH_IOS_ASSETS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set_target_properties(pokerth_qml-client PROPERTIES
        # The bundle is called PokerTH.app (not pokerth_qml-client.app); the name stands
        # on the home screen below the icon.
        OUTPUT_NAME "PokerTH"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
        QT_IOS_LAUNCH_SCREEN     "${CMAKE_CURRENT_SOURCE_DIR}/ios/LaunchScreen.storyboard"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${POKERTH_IOS_BUNDLE_ID}"
        XCODE_ATTRIBUTE_MARKETING_VERSION         "${POKERTH_IOS_VERSION}"
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION   "${POKERTH_IOS_BUILD}"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"   # iPhone + iPad
    )

    # Copy data/ into the app bundle. Unlike on Android, NO Qt resource is needed
    # for that: iOS has a real file system, and getDataPathStdString()
    # (qthelper.cpp) already delivers "<applicationDirPath>/data/" in the __APPLE__
    # branch for everything that does not end in Contents/MacOS – with a
    # flat iOS bundle that is <PokerTH.app>/data/. So the files only have to go
    # there, and AppDataDir/StyleProvider work unchanged.
    # POST_BUILD instead of ~1000 individual files as bundle resources: that keeps the
    # Xcode project file small. Xcode signs the bundle only after all
    # build phases, so signing added later covers the data as well.
    add_custom_command(TARGET pokerth_qml-client POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${CMAKE_SOURCE_DIR}/data"
                "$<TARGET_BUNDLE_CONTENT_DIR:pokerth_qml-client>/data"
        COMMENT "Kopiere data/ (Stile, Karten, Sounds, Fonts) ins iOS-App-Bundle")

    # Multimedia backend: since 6.5 QtMultimedia has FFmpeg as the default backend, and
    # the static Qt links its plugin in automatically. The corresponding
    # FFmpeg libraries (libavcodec/libswscale/…), however, are NOT shipped by the aqt iOS
    # package → the link fails on undefined symbols (sws_scale, …).
    # The Darwin backend (AVFoundation) is the native way on iOS, is contained in the
    # Qt iOS package and is completely sufficient for the WAV sounds of the client.
    # INCLUDE_BY_TYPE replaces the default plugin list of this type, so the FFmpeg plugin
    # is left out.
    if(NOT TARGET Qt6::QDarwinMediaPlugin)
        message(FATAL_ERROR
            "Qt6::QDarwinMediaPlugin not found. Without the AVFoundation backend "
            "the FFmpeg plugin would be linked, whose libraries the Qt iOS "
            "installation does not contain. Install Qt with the qtmultimedia module.")
    endif()
    qt_import_plugins(pokerth_qml-client
        INCLUDE_BY_TYPE multimedia Qt6::QDarwinMediaPlugin)

    qt_finalize_target(pokerth_qml-client)
endif()

# Android: configure and finalize the target
if(ANDROID)
    # Set the properties BEFORE qt_finalize_target
    set_target_properties(pokerth_qml-client PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_MIN_SDK_VERSION "28"
        QT_ANDROID_TARGET_SDK_VERSION "35"
    )

    # No qt_import_plugins() here, deliberately — the FFmpeg backend is dropped
    # in build_android_qml.sh instead. qt_import_plugins() writes its selection
    # into the deployment settings as "android-deploy-plugins", a flat list of
    # absolute paths whose file names carry the ABI suffix
    # (…_androidmediaplugin_arm64-v8a.so), and that list REPLACES androiddeployqt's
    # own per-ABI plugin discovery. Our bundle deploys every ABI in one run from
    # one settings file, so a single such list either starves the other ABIs of
    # plugins or — if merged — hands each of them all four variants, which
    # androiddeployqt copies without checking their architecture: an x86_64
    # package then holds libplugins_platforms_qtforandroid_x86.so and the app
    # dies at startup with "is 32-bit instead of 64-bit".

    # Finalize the target (it generates deployment-settings.json)
    qt_finalize_target(pokerth_qml-client)
endif()

install(TARGETS pokerth_qml-client DESTINATION bin OPTIONAL)
install(DIRECTORY ../../../data DESTINATION share/pokerth)
install(FILES ../../../pokerth_qml.desktop DESTINATION share/applications)
install(FILES ../../../pokerth.png DESTINATION share/pixmaps)
