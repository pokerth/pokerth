project(pokerth_db)

find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)

add_definitions(-DENABLE_IPV6 -DTIXML_USE_STL)
include_directories(
        src)

if(WIN32)
   add_definitions(-DCURL_STATICLIB -D_WIN32_WINNT=0x0501)
   include_directories(../boost/ ../GnuTLS/include
       ../curl/include ../zlib)
endif(WIN32)

set(pokerth_db_HDRS
        src/db/serverdbcallback.h
        src/db/serverdbfactory.h
        src/db/serverdbinterface.h
        src/db/serverdbgeneric.h
        src/db/serverdbfactorygeneric.h
        src/db/serverdbnoaction.h)

set(pokerth_db_SRCS
        src/db/common/serverdbcallback.cpp
        src/db/common/serverdbfactory.cpp
        src/db/common/serverdbinterface.cpp
        src/db/common/serverdbgeneric.cpp
        src/db/common/serverdbfactorygeneric.cpp
        src/db/common/serverdbnoaction.cpp)

add_library(pokerth_db STATIC
    ${pokerth_db_MOC_SRCS}
    ${pokerth_db_SRCS}
    ${pokerth_db_HDRS}
)

qt5_use_modules(pokerth_db Gui Core)

#target_link_libraries(pokerth_lib
#    ${LIBS})
