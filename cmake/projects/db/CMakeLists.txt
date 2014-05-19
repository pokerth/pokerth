project(pokerth_db)

find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)

add_definitions(-DENABLE_IPV6 -DTIXML_USE_STL)
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    )

if(WIN32)
    add_definitions(-DCURL_STATICLIB -D_WIN32_WINNT=0x0501)
    include_directories(../boost/ ../GnuTLS/include
        ../curl/include ../zlib)
endif(WIN32)

set(pokerth_db_HDRS
    ${CMAKE_SOURCE_DIR}/src/db/serverdbcallback.h
    ${CMAKE_SOURCE_DIR}/src/db/serverdbfactory.h
    ${CMAKE_SOURCE_DIR}/src/db/serverdbinterface.h
    ${CMAKE_SOURCE_DIR}/src/db/serverdbgeneric.h
    ${CMAKE_SOURCE_DIR}/src/db/serverdbfactorygeneric.h
    ${CMAKE_SOURCE_DIR}/src/db/serverdbnoaction.h)

set(pokerth_db_SRCS
    ${CMAKE_SOURCE_DIR}/src/db/common/serverdbcallback.cpp
    ${CMAKE_SOURCE_DIR}/src/db/common/serverdbfactory.cpp
    ${CMAKE_SOURCE_DIR}/src/db/common/serverdbinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/db/common/serverdbgeneric.cpp
    ${CMAKE_SOURCE_DIR}/src/db/common/serverdbfactorygeneric.cpp
    ${CMAKE_SOURCE_DIR}/src/db/common/serverdbnoaction.cpp)

add_library(pokerth_db STATIC
    ${pokerth_db_MOC_SRCS}
    ${pokerth_db_SRCS}
    ${pokerth_db_HDRS}
    )

qt5_use_modules(pokerth_db Gui Core)

#target_link_libraries(pokerth_db
#    ${LIBS})
