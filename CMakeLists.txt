cmake_minimum_required(VERSION 3.15)

# Minimum Qt version. Single source of truth: only defined here (scripts do not pass -DQT_MIN_VERSION).
# Needed for building on Ubuntu 24.04 LTS with system Qt package version 6.4.2
set(QT_MIN_VERSION "6.4.2" CACHE STRING "Minimum Qt version")

project(POKERTH)

# So clangd/Cursor use the same includes and flags as the build (see scripts that symlink to repo root)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable vcpkg applocal deployment for cross-compilation
if(CMAKE_CROSSCOMPILING)
    set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "" FORCE)
    set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "" FORCE)
endif()

# When cross-compiling for Windows with MinGW, CMake should be configured with
# CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY to prevent host system headers from being found.
# If configured correctly, __GLIBC__ should not be defined and Boost will use std::strerror().

option(CMAKE_AUTORCC "" ON)
option(CMAKE_AUTOUIC "" ON)
option(CMAKE_AUTOMOC "" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "uics/")
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "src/gui/qt")
list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "src/gui/qt/gui_800x480")

if(GUI_800_480)
    add_definitions(-DGUI_800x480)
endif()

# macOS-only workaround for Qt 6.9 setStyleSheet crashes on Monterey
# Using __APPLE__ preprocessor define in source code instead
# if(APPLE)
#     add_definitions(-DDISABLE_COMPLEX_STYLESHEETS_MACOS)
# endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FindBoost)

# On macOS, prefer system frameworks (e.g. OpenGL.framework) so Qt's FindWrapOpenGL
# does not pick up X11's /usr/X11R6/lib and trigger "linking to directory" warnings.
if(APPLE)
    set(CMAKE_FIND_FRAMEWORK FIRST)
endif()

find_package(Boost REQUIRED COMPONENTS iostreams random thread filesystem program_options)
find_package(OpenSSL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Network)

if(ANDROID)
    # Qt 6.6+ QTP0002: OLD deprecated; NEW allows generator expressions in Android paths.
    qt_policy(SET QTP0002 NEW)
    set(Protobuf_USE_STATIC_LIBS ON)
    set(protobuf_MODULE_COMPATIBLE ON)
    # Force Protobuf to use emulated TLS instead of native TLS
    add_compile_options(-femulated-tls)  # WICHTIG: Emuliere TLS für Android
endif()

# vcpkg uses CONFIG mode for all platforms
if(DEFINED VCPKG_TOOLCHAIN OR DEFINED CMAKE_TOOLCHAIN_FILE)
    # When using vcpkg, use CONFIG mode only (don't include FindProtobuf)
    find_package(Protobuf CONFIG REQUIRED)
else()
    # When not using vcpkg, use the traditional FindProtobuf module
    include(FindProtobuf)
    find_package(Protobuf REQUIRED)
endif()

if(ANDROID)
    # Find abseil and utf8_range (required for protobuf >= 5.27)
    find_package(absl CONFIG REQUIRED)
    find_package(utf8_range CONFIG REQUIRED)
endif()

# Generate protobuf files in build directory (platform-specific)
# This allows sharing build directories across different targets
# Generate in subdirectory structure to match include paths: <third_party/protobuf/...>
set(PROTOBUF_GEN_DIR ${CMAKE_BINARY_DIR}/generated/third_party/protobuf)
file(MAKE_DIRECTORY ${PROTOBUF_GEN_DIR})

# Generate protobuf files
# When using vcpkg CONFIG mode, we need to use protobuf::protoc target
# When using FindProtobuf, use protobuf_generate_cpp function
if(DEFINED VCPKG_TOOLCHAIN OR DEFINED CMAKE_TOOLCHAIN_FILE)
    # vcpkg CONFIG mode - use custom commands with protobuf::protoc
    set(PROTOBUF_GEN_SRC "")
    set(PROTOBUF_GEN_HDRS "")

    foreach(PROTO_FILE chatcleaner.proto pokerth.proto)
        get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
        set(PROTO_SRC "${PROTOBUF_GEN_DIR}/${PROTO_NAME}.pb.cc")
        set(PROTO_HDR "${PROTOBUF_GEN_DIR}/${PROTO_NAME}.pb.h")

        list(APPEND PROTOBUF_GEN_SRC ${PROTO_SRC})
        list(APPEND PROTOBUF_GEN_HDRS ${PROTO_HDR})

        add_custom_command(
            OUTPUT ${PROTO_SRC} ${PROTO_HDR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTOBUF_GEN_DIR}
            COMMAND protobuf::protoc
            ARGS --cpp_out=${PROTOBUF_GEN_DIR}
                 --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
                 ${PROTO_FILE}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PROTO_FILE}
            COMMENT "Generating ${PROTO_NAME}.pb.cc and ${PROTO_NAME}.pb.h"
        )
    endforeach()
else()
    # Traditional FindProtobuf mode
    protobuf_generate_cpp(PROTOBUF_GEN_SRC PROTOBUF_GEN_HDRS
        chatcleaner.proto pokerth.proto
        PROTOC_OUT_DIR  ${PROTOBUF_GEN_DIR})
endif()

# Füge die generierten Dateien zur Quellenliste hinzu
list(APPEND POKERTH_LIB_SRC ${PROTOBUF_GEN_SRC})

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Sql Xml WebSockets Qml Quick QuickControls2 Widgets Svg Network LinguistTools Multimedia REQUIRED)

# POKERTH.lib

set(POKERTH_LIB_SRC
    src/engine/game.cpp
    src/playerdata.cpp
    src/config/configfile.cpp
    src/engine/boardinterface.cpp
    src/engine/enginefactory.cpp
    src/engine/handinterface.cpp
    src/engine/playerinterface.cpp
    src/engine/berointerface.cpp
    src/gui/guiinterface.cpp
    src/core/thread.cpp
    src/core/crypthelper.cpp
    src/core/avatarmanager.cpp
    src/core/pokerthexception.cpp
    src/engine/local_engine/cardsvalue.cpp
    src/engine/local_engine/localboard.cpp
    src/engine/local_engine/localenginefactory.cpp
    src/engine/local_engine/localhand.cpp
    src/engine/local_engine/localplayer.cpp
    src/engine/local_engine/localberopreflop.cpp
    src/engine/local_engine/localberoflop.cpp
    src/engine/local_engine/localberoturn.cpp
    src/engine/local_engine/localberoriver.cpp
    src/engine/local_engine/localberopostriver.cpp
    src/engine/local_engine/localbero.cpp
    src/engine/local_engine/localexception.cpp
    src/engine/local_engine/arraydata.cpp
    src/engine/log.cpp
    src/engine/network_engine/clientboard.cpp
    src/engine/network_engine/clientenginefactory.cpp
    src/engine/network_engine/clienthand.cpp
    src/engine/network_engine/clientplayer.cpp
    src/engine/network_engine/clientbero.cpp
    src/net/chatcleanermanager.cpp
    src/net/chatcleanercallback.cpp
    src/net/discordwebhook.cpp
    src/net/clientcallback.cpp
    src/net/clientcontext.cpp
    src/net/clientstate.cpp
    src/net/clientthread.cpp
    src/net/downloadhelper.cpp
    src/net/downloaderthread.cpp
    src/net/netpacket.cpp
    src/net/netpacketvalidator.cpp
    src/net/senderhelper.cpp
    src/net/sendercallback.cpp
    src/net/servercallback.cpp
    src/net/sessiondata.cpp
    src/net/sessiondatacallback.cpp
    src/net/sessionmanager.cpp
    src/net/socket_startup.cpp
    src/net/clientexception.cpp
    src/net/netcontext.cpp
    src/net/netexception.cpp
    src/net/transferhelper.cpp
    src/net/uploaderthread.cpp
    src/net/uploadhelper.cpp
    src/net/sendbuffer.cpp
    src/net/asiosendbuffer.cpp
    src/net/websendbuffer.cpp
    src/net/receivebuffer.cpp
    src/net/asioreceivebuffer.cpp
    src/net/webreceivebuffer.cpp
    src/net/uploadcallback.cpp
    src/gui/generic/serverguiwrapper.cpp
    src/gui/qttoolsinterface.cpp)

if(ANDROID)
    list(APPEND POKERTH_LIB_SRC "src/engine/local_engine/tools_android.cpp")
else()
    list(APPEND POKERTH_LIB_SRC "src/engine/local_engine/tools.cpp")
endif()

add_library(pokerth_lib STATIC ${POKERTH_LIB_SRC})

# Füge Include-Pfad für generierte Protobuf-Header hinzu (build directory)
# Add parent directory so #include <third_party/protobuf/...> works
target_include_directories(pokerth_lib PRIVATE "${CMAKE_BINARY_DIR}/generated")
target_include_directories(pokerth_lib PRIVATE "${Boost_INCLUDE_DIRS}")
target_include_directories(pokerth_lib PRIVATE "${OPENSSL_INCLUDE_DIR}")
target_include_directories(pokerth_lib PUBLIC
    src
    src/engine src/engine/local_engine
    src/engine/network_engine
    src/gui
    src/gui/qt
    src/gui/qt/qttools
    src/gui/qt/gametable
    src/net
    src/config
    src/core
    src/third_party/websocketpp)

target_link_libraries(pokerth_lib PUBLIC Boost::iostreams Boost::random Boost::thread Boost::filesystem Boost::program_options)
if(TARGET OpenSSL::SSL)
    target_link_libraries(pokerth_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else()
    target_link_libraries(pokerth_lib PUBLIC "${OPENSSL_LIBRARIES}")
endif()
target_link_libraries(pokerth_lib PUBLIC Qt6::Network)
target_link_libraries(pokerth_lib PUBLIC pokerth_protocol)
target_link_libraries(pokerth_lib PRIVATE Qt6::Xml Qt6::Sql)
target_compile_options(pokerth_lib PUBLIC -fexceptions -frtti)
target_compile_definitions(pokerth_lib PUBLIC ENABLE_IPV6 HAVE_OPENSSL BOOST_FILESYSTEM_DEPRECATED)


# POKERTH.dbofficial

file(GLOB POKERTH_DBOFFICIAL_SRC src/dbofficial/*.cpp)

add_library(pokerth_dbofficial "${POKERTH_DBOFFICIAL_SRC}")

# Use Qt6::Sql instead of mysql++ (compat header in src/dbofficial)
target_include_directories(pokerth_dbofficial PRIVATE "${Boost_INCLUDE_DIRS}")
target_include_directories(pokerth_dbofficial PUBLIC src/)
target_link_libraries(pokerth_dbofficial PRIVATE Qt6::Sql)

# POKERTH.db

file(GLOB POKERTH_DB_SRC src/db/common/*.cpp)

add_library(pokerth_db "${POKERTH_DB_SRC}")

target_include_directories(pokerth_db PRIVATE "${Boost_INCLUDE_DIRS}")
target_include_directories(pokerth_db PUBLIC src/)


# POKERTH.protocol

add_library(pokerth_protocol STATIC ${PROTOBUF_GEN_SRC})

# Skip autogen for generated protobuf files (they're in build directory, not source tree)
set_source_files_properties(${PROTOBUF_GEN_SRC} ${PROTOBUF_GEN_HDRS} PROPERTIES SKIP_AUTOGEN ON)

message(STATUS "Protobuf include dir: ${Protobuf_INCLUDE_DIR}")

target_include_directories(pokerth_protocol PRIVATE "${Protobuf_INCLUDE_DIR}")
# Add parent directory so #include <third_party/protobuf/...> works
target_include_directories(pokerth_protocol PUBLIC "${CMAKE_BINARY_DIR}/generated")

# Use imported target when using CONFIG mode (vcpkg), otherwise use variable
if(TARGET protobuf::libprotobuf)
    target_link_libraries(pokerth_protocol PUBLIC protobuf::libprotobuf)
else()
    target_link_libraries(pokerth_protocol PUBLIC "${Protobuf_LIBRARIES}")
endif()

if(ANDROID)
    target_compile_definitions(pokerth_protocol PUBLIC PROTOBUF_USE_DLLS)
    # Link abseil and utf8_range libraries (required by protobuf >= 5.27)
    target_link_libraries(pokerth_protocol PUBLIC
        absl::base
        absl::log_internal_message
        absl::log_internal_check_op
        absl::strings
        absl::cord
        absl::status
        absl::statusor
        absl::hash
        absl::flat_hash_map
        absl::raw_hash_set
        utf8_range::utf8_range
        utf8_range::utf8_validity
    )
elseif(APPLE)
    # macOS: protobuf (from vcpkg/aqt) may require abseil and utf8_range symbols
    find_package(absl CONFIG REQUIRED)
    find_package(utf8_range CONFIG REQUIRED)
    target_link_libraries(pokerth_protocol PUBLIC
        absl::base
        absl::log_internal_message
        absl::log_internal_check_op
        absl::strings
        absl::cord
        absl::status
        absl::statusor
        absl::hash
        absl::flat_hash_map
        absl::raw_hash_set
        utf8_range::utf8_range
        utf8_range::utf8_validity
    )
elseif(WIN32)
    # Für Windows: Verlinke nur die Abseil-Bibliotheken ohne PROTOBUF_USE_DLLS
    target_link_libraries(pokerth_protocol PUBLIC
        absl::base
        absl::log_internal_message
        absl::log_internal_check_op
        absl::strings
        absl::cord
        absl::status
        absl::statusor
        absl::hash
        absl::flat_hash_map
        absl::raw_hash_set
        utf8_range::utf8_range
        utf8_range::utf8_validity
    )
endif()

add_subdirectory(src)
add_subdirectory(src/chatcleaner)
add_subdirectory(src/dbofficial)
add_subdirectory(src/gui/qt)
add_subdirectory(src/gui/qt6-qml)

# zlib_compress utility tool for compressing serverlist
add_executable(zlib_compress src/zlib_compress.cpp)
target_link_libraries(zlib_compress
    PRIVATE
    Boost::filesystem
    Boost::iostreams
)

# pokerth_bot - Headless bot client for automated testing
add_executable(pokerth_bot src/pokerth_bot.cpp)
target_link_libraries(pokerth_bot
    PRIVATE
    pokerth_lib
    Boost::program_options
)
target_include_directories(pokerth_bot PRIVATE ${CMAKE_SOURCE_DIR}/src)
