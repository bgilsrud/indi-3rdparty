PROJECT(libfreetoup C)
cmake_minimum_required(VERSION 3.2)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/")
include(GNUInstallDirs)

find_package(INDI REQUIRED)
find_package(USB1 REQUIRED)

SET(PACKAGE_VERSION "1.0.0")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

ADD_DEFINITIONS(-Wall -O2 -fPIC -g)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

set(freetoup_LIB_SRCS
    freetoup.c)

#build a shared library
ADD_LIBRARY(freetoup SHARED ${freetoup_LIB_SRCS})

set_target_properties(freetoup PROPERTIES VERSION 1.0.0 SOVERSION 0)

#need to link to some other libraries ? just add them here
TARGET_LINK_LIBRARIES(freetoup ${USB1_LIBRARIES})

#add an install target here
INSTALL(FILES freetoup.h DESTINATION include)

INSTALL(TARGETS freetoup LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
