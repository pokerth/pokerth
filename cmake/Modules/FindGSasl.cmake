# - Try to find the GNU sasl library (gsasl)
#
# Once done this will define
#
#  GNUTLS_FOUND - System has gnutls
#  GNUTLS_INCLUDE_DIR - The gnutls include directory
#  GNUTLS_LIBRARIES - The libraries needed to use gnutls
#  GNUTLS_DEFINITIONS - Compiler switches required for using gnutls

# Adapted from FindGnuTLS.cmake, which is:
#   Copyright 2009, Brad Hards, <bradh@kde.org>
#
# Changes are Copyright 2009, Michele Caini, <skypjack@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


IF (GSASL_INCLUDE_DIR AND GSASL_LIBRARIES)
   # in cache already
   SET(GSasl_FIND_QUIETLY TRUE)
ENDIF (GSASL_INCLUDE_DIR AND GSASL_LIBRARIES)

IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_GSASL libgsasl)
   SET(GSASL_DEFINITIONS ${PC_GSASL_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(GSASL_INCLUDE_DIR gsasl.h
   HINTS
   ${PC_GSASL_INCLUDEDIR}
   ${PC_GSASL_INCLUDE_DIRS}
   PATH_SUFFIXES gsasl
   )

FIND_LIBRARY(GSASL_LIBRARIES NAMES gsasl libgsasl
    HINTS
    ${PC_GSASL_LIBDIR}
    ${PC_GSASL_LIBRARY_DIRS}
  )

INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set GSASL_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GSASL DEFAULT_MSG GSASL_LIBRARIES GSASL_INCLUDE_DIR)

MARK_AS_ADVANCED(GSASL_INCLUDE_DIR GSASL_LIBRARIES)
