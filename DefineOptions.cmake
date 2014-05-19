include(CMakeDependentOption)
option(GUI_800x480 "Build 800x480 gui version" OFF)
option(ONLY_CLIENT "Only build the client and not the server" OFF)
if(NOT APPLE)
    CMAKE_DEPENDENT_OPTION(BUILD_SERVER "Build the server" ON
        "NOT ONLY_CLIENT;NOT GUI_800x480" OFF)
elseif(NOT APPLE)
    CMAKE_DEPENDENT_OPTION(BUILD_SERVER "Build the server" OFF
        "NOT ONLY_CLIENT;NOT GUI_800x480" OFF)
endif(NOT APPLE)
