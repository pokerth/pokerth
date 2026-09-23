# Windows resource (icon + version info) for the client exes.
#
# pokerth.rc.in becomes one .rc per target, so each exe carries its own
# FileDescription, FileVersion and OriginalFilename. The version numbers come
# from src/game_defs.h:
#   - ProductVersion: the release version, QML_VERSION_* (the QML client
#     carries the release number; the installers use the same one).
#   - FileVersion: the client's own version, which for the Widget client is
#     POKERTH_VERSION_MAJOR/MINOR + POKERTH_BETA_REVISION.

file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../src/game_defs.h" _pokerth_version_defines
     REGEX "^#define[ \t]+(POKERTH_VERSION_MAJOR|POKERTH_VERSION_MINOR|POKERTH_BETA_REVISION|QML_VERSION_MAJOR|QML_VERSION_MINOR|QML_VERSION_REVISION)[ \t]")
foreach(_line IN LISTS _pokerth_version_defines)
    if(_line MATCHES "^#define[ \t]+([A-Z_]+)[ \t]+([0-9]+)")
        set(_PV_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()
foreach(_name POKERTH_VERSION_MAJOR POKERTH_VERSION_MINOR POKERTH_BETA_REVISION
              QML_VERSION_MAJOR QML_VERSION_MINOR QML_VERSION_REVISION)
    if(NOT DEFINED _PV_${_name})
        message(FATAL_ERROR "PokerthWindowsResource: ${_name} not found in src/game_defs.h")
    endif()
endforeach()

set(POKERTH_RELEASE_VERSION "${_PV_QML_VERSION_MAJOR}.${_PV_QML_VERSION_MINOR}.${_PV_QML_VERSION_REVISION}")
set(POKERTH_WIDGET_VERSION "${_PV_POKERTH_VERSION_MAJOR}.${_PV_POKERTH_VERSION_MINOR}.${_PV_POKERTH_BETA_REVISION}")

# pokerth_add_windows_resource(<target> <description> <file version>)
function(pokerth_add_windows_resource target description file_version)
    set(POKERTH_RC_TARGET "${target}")
    set(POKERTH_RC_DESCRIPTION "${description}")
    set(POKERTH_RC_FILE_VERSION "${file_version}")
    set(POKERTH_RC_PRODUCT_VERSION "${POKERTH_RELEASE_VERSION}")
    string(REPLACE "." "," POKERTH_RC_FILE_VERSION_COMMAS "${file_version}")
    string(REPLACE "." "," POKERTH_RC_PRODUCT_VERSION_COMMAS "${POKERTH_RELEASE_VERSION}")
    # Absolute path, so windres needs no include path to find the icon.
    set(POKERTH_RC_ICON "${CMAKE_SOURCE_DIR}/pokerth.ico")

    set(rc_file "${CMAKE_CURRENT_BINARY_DIR}/${target}.rc")
    configure_file("${CMAKE_SOURCE_DIR}/pokerth.rc.in" "${rc_file}" @ONLY)
    target_sources(${target} PRIVATE "${rc_file}")
endfunction()
