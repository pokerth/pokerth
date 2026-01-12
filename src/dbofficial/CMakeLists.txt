# POKERTH.official_server

set(POKERTH_OFFICIAL_SERVER_SRC
    ../pokerth_server.cpp
    ../session.cpp
    asyncdbcreategame.cpp
    asyncdbgameplace.cpp
    asyncdbupdatescore.cpp
    asyncdbquery.cpp
    serverdbthread.cpp
    serverdbfactoryinternal.cpp
    singleasyncdbquery.cpp
    compositeasyncdbquery.cpp
    querycontext.cpp
    asyncdbendgame.cpp
    asyncdblogin.cpp
    asyncdbreportavatar.cpp
    asyncdbreportgame.cpp
    asyncdbavatarblacklist.cpp
    asyncdbadminplayers.cpp
    asyncdbblockplayer.cpp
    asyncdbplayerlastgames.cpp
    dbidmanager.cpp
    ../gui/qt/qttools/nonqttoolswrapper.cpp
    ../gui/qt/qttools/nonqthelper/nonqthelper.cpp
    ../core/loghelper_server.cpp
    ../net/net_helper_server.cpp
    ../net/servermanager.cpp
    ../net/servermanagerfactoryserver.cpp
    ../net/serverlobbythread.cpp
    ../net/serverexception.cpp 
    ../net/serveracceptinterface.cpp 
    ../net/serveracceptwebhelper.cpp 
    ../net/servergame.cpp 
    ../net/servergamestate.cpp 
    ../net/serverbanmanager.cpp 
    ../core/linux/convhelper.cpp)

add_executable(pokerth_official_server ${POKERTH_OFFICIAL_SERVER_SRC} )

target_include_directories(pokerth_official_server PRIVATE "${Boost_INCLUDE_DIRS}")
target_include_directories(pokerth_official_server PUBLIC
    ../ 
    ../engine 
    ../gui
    ../gui/qt
    ../gui/qt/qttools
    ../gui/qt/qttools/nonqthelper
    ../net 
    ../engine/local_engine 
    ../engine/network_engine 
    ../config 
    ../core)

target_link_libraries(pokerth_official_server PUBLIC Qt6::Network)
target_link_libraries(pokerth_official_server PUBLIC pokerth_dbofficial)
target_link_libraries(pokerth_official_server PUBLIC pokerth_lib)
target_link_libraries(pokerth_official_server PUBLIC pokerth_db)
target_link_libraries(pokerth_official_server PUBLIC pokerth_protocol)
target_link_libraries(pokerth_official_server PRIVATE Qt6::Xml Qt6::Sql)

target_compile_definitions(pokerth_official_server PUBLIC POKERTH_OFFICIAL_SERVER POKERTH_DEDICATED_SERVER HAVE_OPENSSL)

install(TARGETS pokerth_official_server DESTINATION bin OPTIONAL COMPONENT pokerth_official_server)
