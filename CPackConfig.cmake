# For help take a look at:
# http://www.cmake.org/Wiki/CMake:CPackConfiguration

### general settings
set(CPACK_PACKAGE_NAME ${APPLICATION_NAME})
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VENDOR "Sam Segers")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Texas hold'em game")

### versions
set(CPACK_PACKAGE_VERSION_MAJOR ${APPLICATION_VERSION_MAJOR}) 
set(CPACK_PACKAGE_VERSION_MINOR ${APPLICATION_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${APPLICATION_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT ${APPLICAION_EMAIL})


### source generator
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "~$;[.]swp$;/[.]svn/;/[.]git/;.gitignore;/build/;/obj/;tags;cscope.*")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")

    ### nsis generator
    find_package(NSIS)
    if(NSIS_MAKE)
        set(CPACK_GENERATOR "${CPACK_GENERATOR};NSIS")
        set(CPACK_NSIS_DISPLAY_NAME "PokerTH")
        set(CPACK_NSIS_COMPRESSOR "/SOLID zlib")
        set(CPACK_NSIS_MENU_LINKS "www.pokerth.net" "PokerTH homepage")
    endif(NSIS_MAKE)
endif(WIN32)

set(CPACK_PACKAGE_FILE_NAME ${APPLICATION_NAME}-${CPACK_PACKAGE_VERSION})

### DEB
if(UNIX)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "gsfonts-x11, ttf-dejavu-core")
    #set(CPACK_DEBIAN_PACKAGE_SECTION "universe/games")
    set(CPACK_PACKAGE_DESCRIPTION "Texas hold'em game
 pokerth is a free implementation of the Texas hold'em poker game which is
 mostly played in casinos and has a growing popularity worldwide. Texas hold'em
 is easy to learn but needs a good strategy to win and a lot of luck. This
 package helps you when practicing or just playing for fun.")
    set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_LIBRARY_ARCHITECTURE})
endif()

include(CPack)
