project(pokerth_server)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Network REQUIRED)

find_package(GSasl REQUIRED)
find_package(Sqlite3 REQUIRED)
find_package(TinyXML REQUIRED)
find_package(Protobuf REQUIRED)
find_package(GCrypt REQUIRED)
find_package(Ircclient REQUIRED)

find_package(Boost REQUIRED COMPONENTS filesystem thread program_options iostreams chrono system regex random)

add_definitions(-DENABLE_IPV6 -DTIXML_USE_STL -DBOOST_FILESYSTEM_DEPRECATED -DPOKERTH_DEDICATED_SERVER -DPREFIX=${CMAKE_INSTALL_PREFIX})

set(RESOURCES src/gui/qt/resources/pokerth.qrc)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/qt
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/nonqthelper
    ${CMAKE_SOURCE_DIR}/src/net
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/core
    )

include_directories(
    ${GSASL_INCLUDE_DIR}
    ${SDL_INCLUDE_DIR}
    ${SDL_MIXER_INCLUDE_DIR}
    ${SQLITE3_INCLUDE_DIR}
    ${TINYXML_INCLUDE_DIR}
    ${GCRYPT_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${IRCCLIENT_INCLUDE_DIR}
    )

set(pokerth_server_HDRS
    ${CMAKE_SOURCE_DIR}/src/engine/game.h
    ${CMAKE_SOURCE_DIR}/src/session.h
    ${CMAKE_SOURCE_DIR}/src/playerdata.h
    ${CMAKE_SOURCE_DIR}/src/gamedata.h
    ${CMAKE_SOURCE_DIR}/src/config/configfile.h
    ${CMAKE_SOURCE_DIR}/src/core/thread.h
    ${CMAKE_SOURCE_DIR}/src/engine/boardinterface.h
    ${CMAKE_SOURCE_DIR}/src/engine/enginefactory.h
    ${CMAKE_SOURCE_DIR}/src/engine/handinterface.h
    ${CMAKE_SOURCE_DIR}/src/engine/playerinterface.h
    ${CMAKE_SOURCE_DIR}/src/engine/berointerface.h
    ${CMAKE_SOURCE_DIR}/src/gui/guiinterface.h
    ${CMAKE_SOURCE_DIR}/src/net/clientcallback.h
    ${CMAKE_SOURCE_DIR}/src/net/clientcontext.h
    ${CMAKE_SOURCE_DIR}/src/net/clientexception.h
    ${CMAKE_SOURCE_DIR}/src/net/clientstate.h
    ${CMAKE_SOURCE_DIR}/src/net/clientthread.h
    ${CMAKE_SOURCE_DIR}/src/net/genericsocket.h
    ${CMAKE_SOURCE_DIR}/src/net/netpacket.h
    ${CMAKE_SOURCE_DIR}/src/net/senderhelper.h
    ${CMAKE_SOURCE_DIR}/src/net/serveraccepthelper.h
    ${CMAKE_SOURCE_DIR}/src/net/serverlobbythread.h
    ${CMAKE_SOURCE_DIR}/src/net/socket_helper.h
    ${CMAKE_SOURCE_DIR}/src/net/socket_msg.h
    ${CMAKE_SOURCE_DIR}/src/net/socket_startup.h
    ${CMAKE_SOURCE_DIR}/src/net/net_helper.h
    ${CMAKE_SOURCE_DIR}/src/core/pokerthexception.h
    ${CMAKE_SOURCE_DIR}/src/core/convhelper.h
    ${CMAKE_SOURCE_DIR}/src/core/loghelper.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/cardsvalue.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localboard.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localenginefactory.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localhand.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localplayer.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberopreflop.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberoflop.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberoturn.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberoriver.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localberopostriver.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/tools.h
    ${CMAKE_SOURCE_DIR}/src/engine/local_engine/localbero.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientboard.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientenginefactory.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clienthand.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientplayer.h
    ${CMAKE_SOURCE_DIR}/src/engine/network_engine/clientbero.h
    ${CMAKE_SOURCE_DIR}/src/gui/qttoolsinterface.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/nonqttoolswrapper.h
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/nonqthelper/nonqthelper.h
    ${CMAKE_SOURCE_DIR}/src/gui/generic/serverguiwrapper.h
    ${CMAKE_SOURCE_DIR}/src/net/servermanagerirc.h
    )

set(pokerth_server_SRCS
    ${CMAKE_SOURCE_DIR}/src/pokerth_server.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/nonqttoolswrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/qt/qttools/nonqthelper/nonqthelper.cpp
    ${CMAKE_SOURCE_DIR}/src/net/common/net_helper_server.cpp
    ${CMAKE_SOURCE_DIR}/src/core/common/loghelper_server.cpp
    ${CMAKE_SOURCE_DIR}/src/net/common/ircthread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/common/servermanagerirc.cpp
    ${CMAKE_SOURCE_DIR}/src/net/common/servermanagerfactoryserver.cpp
    )

if(WIN32)
    add_definitions(-DCURL_STATICLIB -D_WIN32_WINNT=0x0501 -DHAVE_OPENSSL)
    include_directories(../boost/ ../GnuTLS/include ../gsasl/include
        ../curl/include ../zlib ../sqlite ../openssl/include)
    set(pokerth_server_SRCS ${pokerth_server_SRCS}
        ${CMAKE_SOURCE_DIR}/src/core/win32/convhelper.cpp
        )
else(WIN32)
    set(pokerth_server_SRCS ${pokerth_server_SRCS}
        ${CMAKE_SOURCE_DIR}/src/core/linux/convhelper.cpp
        )
endif(WIN32)

set(LIBS_TO_COPY ${GSASL_LIBRARIES}
    ${SQLITE3_LIBRARIES} ${TINYXML_LIBRARIES} ${PROTOBUF_LIBRARIES}
    ${GCRYPT_LIBRARIES} ${Boost_LIBRARIES} ${IRCCLIENT_LIBRARY})

set(LIBS pokerth_lib pokerth_db pokerth_protocol curl ${LIBS_TO_COPY})

add_executable(pokerth_server
    ${pokerth_server_MOC_SRCS}
    ${pokerth_server_SRCS}
    ${pokerth_server_HDRS}
    )

qt5_use_modules(pokerth_server Gui Core Network)

target_link_libraries(pokerth_server
    ${LIBS})

install(TARGETS pokerth_server
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/)
