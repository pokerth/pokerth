# - Try to find GCrypt
# Once done this will define
#
#  GCRYPT_FOUND - system has GCrypt
#  GCRYPT_INCLUDE_DIRS - the GCrypt include directory
#  GCRYPT_LIBRARIES - Link these to use GCrypt
#  GCRYPT_DEFINITIONS - Compiler switches required for using GCrypt
#
#  Copyright (c) 2009-2010 Andreas Schneider <mail@cynapses.org>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (GCRYPT_LIBRARIES AND GCRYPT_INCLUDE_DIRS)
  # in cache already
  set(GCRYPT_FOUND TRUE)
else (GCRYPT_LIBRARIES AND GCRYPT_INCLUDE_DIRS)

  find_path(GCRYPT_INCLUDE_DIR
    NAMES
      gcrypt.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
      /usr/lib/sfw/include
  )

  find_library(GCRYPT_LIBRARY
    NAMES
      gcrypt
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
      /usr/sfw/lib/64
      /usr/sfw/lib
  )

  set(GCRYPT_INCLUDE_DIRS
    ${GCRYPT_INCLUDE_DIR}
  )

  if (GCRYPT_LIBRARY)
    set(GCRYPT_LIBRARIES
        ${GCRYPT_LIBRARIES}
        ${GCRYPT_LIBRARY}
    )
  endif (GCRYPT_LIBRARY)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GCrypt DEFAULT_MSG GCRYPT_LIBRARIES GCRYPT_INCLUDE_DIRS)

  # show the GCRYPT_INCLUDE_DIRS and GCRYPT_LIBRARIES variables only in the advanced view
  mark_as_advanced(GCRYPT_INCLUDE_DIRS GCRYPT_LIBRARIES)

endif (GCRYPT_LIBRARIES AND GCRYPT_INCLUDE_DIRS)

