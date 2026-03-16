cmake_minimum_required(VERSION 3.15)

set(QT_MIN_VERSION "6.7.0")

project(POKERTH)

# Disable vcpkg applocal deployment for cross-compilation
if(CMAKE_CROSSCOMPILING)
    set(VCPKG_APPLOCAL_DEPS OFF CACHE BOOL "" FORCE)
    set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "" FORCE)
endif()

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

find_package(Boost REQUIRED COMPONENTS iostreams random thread filesystem program_options)
find_package(OpenSSL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Network)

if(ANDROID)
    set(Protobuf_USE_STATIC_LIBS ON)
    set(protobuf_MODULE_COMPATIBLE ON)
    # Force Protobuf to use emulated TLS instead of native TLS
    add_compile_options(-femulated-tls)  # WICHTIG: Emuliere TLS für Android
endif()

# vcpkg uses CONFIG mode for all platforms
if(DEFINED VCPKG_TOOLCHAIN OR DEFINED CMAKE_TOOLCHAIN_FILE)
    find_package(Protobuf CONFIG REQUIRED)
else()
    find_package(Protobuf REQUIRED)
endif()

if(ANDROID)
    # Find abseil and utf8_range (required for protobuf >= 5.27)
    find_package(absl CONFIG REQUIRED)
    find_package(utf8_range CONFIG REQUIRED)
endif()

# Stelle sicher, dass das Ausgabeverzeichnis existiert
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/src/third_party/protobuf)

protobuf_generate_cpp(PROTOBUF_GEN_SRC PROTOBUF_GEN_HDRS
    chatcleaner.proto pokerth.proto
    PROTOC_OUT_DIR  ${CMAKE_SOURCE_DIR}/src/third_party/protobuf)

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

# Füge Include-Pfad für generierte Protobuf-Header hinzu
target_include_directories(pokerth_lib PRIVATE "${CMAKE_SOURCE_DIR}/src/third_party/protobuf")
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

message(STATUS "Protobuf include dir: ${Protobuf_INCLUDE_DIR}")

target_include_directories(pokerth_protocol PRIVATE "${Protobuf_INCLUDE_DIR}")
target_include_directories(pokerth_protocol PUBLIC "src/third_party/protobuf")

target_link_libraries(pokerth_protocol PUBLIC protobuf::libprotobuf)

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

set_property(SOURCE src/third_party/protobuf/chatcleaner.pb.cc PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE src/third_party/protobuf/chatcleaner.pb.h PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE src/third_party/protobuf/pokerth.pb.cc PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE src/third_party/protobuf/pokerth.pb.h PROPERTY SKIP_AUTOGEN ON)

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

