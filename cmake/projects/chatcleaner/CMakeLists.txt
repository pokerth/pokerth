project(chatcleaner)

find_package(Qt5Network)
find_package(Qt5Gui)

find_package(TinyXML REQUIRED)
find_package(Protobuf REQUIRED)

add_definitions(-DTIXML_USE_STL -DPROTOBUF_INCLUDE_RELATIVE)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/net
    ${CMAKE_SOURCE_DIR}/src/chatcleaner
    ${protocol_BINARY_DIR}
    )

include_directories(
    ${TINYXML_INCLUDE_DIR}
    )

if(WIN32)
    include_directories(../boost/)
endif(WIN32)

set(chatcleaner_HDRS
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/cleanerserver.h
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/messagefilter.h
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/badwordcheck.h
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/textfloodcheck.h
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/cleanerconfig.h
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/capsfloodcheck.h
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/letterrepeatingcheck.h
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/urlcheck.h
    )

set(chatcleaner_SRCS
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/chatcleaner.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/cleanerserver.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/messagefilter.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/badwordcheck.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/textfloodcheck.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/cleanerconfig.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/capsfloodcheck.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/letterrepeatingcheck.cpp
    ${CMAKE_SOURCE_DIR}/src/chatcleaner/urlcheck.cpp
    )

set(LIBS_TO_COPY ${TINYXML_LIBRARIES} ${PROTOBUF_LIBRARIES})

set(LIBS pokerth_lib pokerth_protocol ${LIBS_TO_COPY})

add_executable(chatcleaner
    ${chatcleaner_MOC_SRCS}
    ${chatcleaner_SRCS}
    ${chatcleaner_HDRS}
    )

qt5_use_modules(chatcleaner Gui Network)

target_link_libraries(chatcleaner
    ${LIBS})

install(TARGETS chatcleaner
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/)
