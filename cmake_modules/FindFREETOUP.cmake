# - Try to find FREETOUP Camera Library
# Once done this will define
#
#  FREETOUP_FOUND - system has Levenhuk
#  FREETOUP_INCLUDE_DIR - the Levenhuk include directory
#  FREETOUP_LIBRARIES - Link these to use Levenhuk

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (FREETOUP_INCLUDE_DIR AND FREETOUP_LIBRARIES)

  # in cache already
  set(FREETOUP_FOUND TRUE)
  message(STATUS "Found libfreetoup: ${FREETOUP_LIBRARIES}")

else (FREETOUP_INCLUDE_DIR AND FREETOUP_LIBRARIES)

  find_path(FREETOUP_INCLUDE_DIR freetoup.h
    PATH_SUFFIXES libfreetoup
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(FREETOUP_LIBRARIES NAMES freetoup
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(FREETOUP_INCLUDE_DIR AND FREETOUP_LIBRARIES)
    set(FREETOUP_FOUND TRUE)
  else (FREETOUP_INCLUDE_DIR AND FREETOUP_LIBRARIES)
    set(FREETOUP_FOUND FALSE)
  endif(FREETOUP_INCLUDE_DIR AND FREETOUP_LIBRARIES)

  if (FREETOUP_FOUND)
    if (NOT FREETOUP_FIND_QUIETLY)
      message(STATUS "Found FreeToup: ${FREETOUP_LIBRARIES}")
    endif (NOT FREETOUP_FIND_QUIETLY)
  else (FREETOUP_FOUND)
    if (FREETOUP_FIND_REQUIRED)
      message(FATAL_ERROR "FreeToup not found. Please install FreeToup Library http://www.indilib.org")
    endif (FREETOUP_FIND_REQUIRED)
  endif (FREETOUP_FOUND)

  mark_as_advanced(FREETOUP_INCLUDE_DIR FREETOUP_LIBRARIES)

endif (FREETOUP_INCLUDE_DIR AND FREETOUP_LIBRARIES)
