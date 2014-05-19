# Locate ircclient library
# This module defines
#  IRCCLIENT_FOUND, if false, do not try to link to IRCCLIENT
#  IRCCLIENT_LIBRARY, the libircclient variant
#  IRCCLIENT_INCLUDE_DIR, where to find libircclient.h and family)
#
# Note that the expected include convention is
#  #include "libircclient.h"
# and not
#  #include <libircclient/libircclient.h>
# This is because, the ircclient location is not standardized and may exist
# in locations other than libircclient/

#=============================================================================
# Copyright 2011 Mark Vejvoda
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

FIND_PATH(IRCCLIENT_INCLUDE_DIR libircclient.h
  HINTS
  $ENV{IRCCLIENTDIR}
  PATH_SUFFIXES include/libircclient include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

SET(IRCCLIENT_FIND_LIBRARIES ircclient)

OPTION(WANT_STATIC_LIBS "builds as many static libs as possible" OFF)
OPTION(FORCE_IRCCLIENT_DYNAMIC_LIBS "force the use of dynamic libs for IRCCLIENT" OFF)
MESSAGE(STATUS "Force IRCCLient dynamic: ${FORCE_IRCCLIENT_DYNAMIC_LIBS}")

IF(WANT_STATIC_LIBS AND NOT FORCE_IRCCLIENT_DYNAMIC_LIBS)
        SET(IRCCLIENT_FIND_LIBRARIES libircclient.a)
ENDIF()

FIND_LIBRARY(IRCCLIENT_LIBRARY 
  NAMES ${IRCCLIENT_FIND_LIBRARIES}
  HINTS
  $ENV{IRCCLIENTDIR}
  PATH_SUFFIXES lib64 lib libs64 libs libs/Win32 libs/Win64
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)


# handle the QUIETLY and REQUIRED arguments and set IRCCLIENT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IRCCLIENT DEFAULT_MSG IRCCLIENT_LIBRARY IRCCLIENT_INCLUDE_DIR)

IF(IRCCLIENT_FOUND)

        MESSAGE(STATUS "Found IRCClient include path: ${IRCCLIENT_INCLUDE_DIR} libs: ${IRCCLIENT_LIBRARY}")

ELSE()

        MESSAGE(STATUS "**** WARNING: IRCClient NOT FOUND include path: ${IRCCLIENT_INCLUDE_DIR} libs: ${IRCCLIENT_LIBRARY}")

ENDIF()

MARK_AS_ADVANCED(IRCCLIENT_LIBRARY IRCCLIENT_INCLUDE_DIR)
