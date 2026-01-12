# POKERTH.qml-client ... unfinisihed

set(POKERTH_QML_CLIENT_SRC ../../pokerth.cpp
    ../../session.cpp 
    ../../core/loghelper_client.cpp
    ../../net/net_helper_client.cpp
    ../../core/qttools/qthelper/qthelper.cpp
    ../../core/qttools/qttoolswrapper.cpp
    ../../core/qttoolsinterface.cpp
    cpp/retranslate.cpp
    cpp/settingsxmlhandler.cpp
    cpp/settingsmanager.cpp
    cpp/serverconnectionhandler.cpp
    cpp/lobbyhandler.cpp
    cpp/qmlguiinterface.cpp)

qt_add_resources(POKERTH_QML_CLIENT_SRC qml.qrc)

set(QML_FILES
    pokerth.qml
    components/CustomCheckBox.qml
    components/CustomComboBox.qml
    components/CustomTabBar.qml
    components/CustomToggle.qml
    components/GamePlayerBox.qml
    components/GamePlayerSelfBox.qml
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
    config/Parameters.qml)

set(TS_FILES
    i18n/pokerth_de_DE.ts
    i18n/pokerth_fr_FR.ts
    i18n/pokerth_en_US.ts
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
    target_link_libraries(pokerth_qml-client PUBLIC "${Protobuf_LIBRARIES}")
else()
    add_executable(pokerth_qml-client ${POKERTH_QML_CLIENT_SRC})
endif()

target_include_directories(pokerth_qml-client PUBLIC
    ../../config
    ../../core
    ../../core
    ../../core/qttools
    ../../core/qttools/qthelper
    ../
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
target_link_libraries(pokerth_qml-client PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Sql Qt6::Xml Qt6::Svg Qt6::QuickControls2 Qt6::Widgets)

target_compile_definitions(pokerth_qml-client PUBLIC QML_CLIENT)

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
