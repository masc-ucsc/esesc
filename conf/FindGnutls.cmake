# - Try to find gnutls
# Once done this will define
#
#  GNUTLS_FOUND - system has gnutls
#  GNUTLS_INCLUDE_DIR - the gnutls include directory
#  GNUTLS_LIBRARIES - Link these to gnutls
#  GNUTLS_DEFINITIONS - Compiler switches required for using gnutls

# Copyright (c) 2009, Michele Caini, <skypjack@gmail.com>
#
# Redistribution and use is allowed according to the terms of the GPLv3 license.


IF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARIES)
   # in cache already
   SET(Gnutls_FIND_QUIETLY TRUE)
ENDIF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARIES)

IF (NOT WIN32)
   # use pkg-config to get the directories and then use these values
   # in the FIND_PATH() and FIND_LIBRARY() calls
   find_package(PkgConfig)
   pkg_check_modules(PC_VMIME gnutls)
   SET(GNUTLS_DEFINITIONS ${PC_GNUTLS_CFLAGS_OTHER})
ENDIF (NOT WIN32)

FIND_PATH(GNUTLS_INCLUDE_DIR gnutls/gnutls.h
    PATHS
    ${PC_GNUTLS_INCLUDEDIR}
    ${PC_GNUTLS_INCLUDE_DIRS}
  )

FIND_LIBRARY(GNUTLS_LIBRARIES NAMES gnutls
    PATHS
    ${PC_GNUTLS_LIBDIR}
    ${PC_GNUTLS_LIBRARY_DIRS}
  )

IF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARIES)
   SET(GNUTLS_FOUND TRUE)
ELSE (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARIES)
   SET(GNUTLS_FOUND FALSE)
ENDIF (GNUTLS_INCLUDE_DIR AND GNUTLS_LIBRARIES)

IF (GNUTLS_FOUND)
   IF (NOT Gnutls_FIND_QUIETLY)
      MESSAGE(STATUS "Found gnutls: ${GNUTLS_LIBRARIES}")
   ENDIF (NOT Gnutls_FIND_QUIETLY)
ELSE (GNUTLS_FOUND)
   IF (Gnutls_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could NOT find gnutls")
   ENDIF (Gnutls_FIND_REQUIRED)
ENDIF (GNUTLS_FOUND)

MARK_AS_ADVANCED(GNUTLS_INCLUDE_DIR GNUTLS_LIBRARIES)
