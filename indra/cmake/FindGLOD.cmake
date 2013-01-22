# -*- cmake -*-

# - Find GLOD
# Find the GLOD includes and library
# This module defines
#  GLOD_FOUND, System has libglod.so.
#  GLOD_INCLUDE_DIRS - The GLOD include directories.
#  GLOD_LIBRARIES - The libraries needed to use libglod.
#  GLOD_DEFINITIONS - Compiler switches required for using libglod.

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_GLOD glod)
SET(GLOD_DEFINITIONS ${PC_GLOD_CFLAGS_OTHER})

FIND_PATH(GLOD_INCLUDE_DIR glod/glod.h
  HINTS ${PC_GLOD_INCLUDE_DIR} ${PC_GLOD_INCLUDE_DIRS}
  )

FIND_LIBRARY(GLOD_LIBRARY
  NAMES libGLOD.so
  HINTS ${PC_GLOD_LIBDIR} ${PC_GLOD_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib)

SET(GLOD_LIBRARIES ${GLOD_LIBRARY})
SET(GLOD_INCLUDE_DIRS ${GLOD_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLOD DEFAULT_MSG
  GLOD_LIBRARY GLOD_INCLUDE_DIR)

MARK_AS_ADVANCED(GLOD_LIBRARY GLOD_INCLUDE_DIR)

