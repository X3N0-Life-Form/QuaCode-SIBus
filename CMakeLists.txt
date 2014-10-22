#
# Main authors:
#    Vincent Barichard <Vincent.Barichard@univ-angers.fr>
#
# Copyright:
#    Vincent Barichard, 2013
#
# Last modified:
#    $Date$ by $Author$
#    $Revision$
#
# This file is part of Quacode:
#    http://quacode.barichard.com
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(GECODE_SRC "${CMAKE_SOURCE_DIR}" CACHE PATH "Path where GeCode source is installed")
SET(GECODE_BIN "${GECODE_SRC}" CACHE PATH "Path where GeCode libs and binaries are installed")

# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()
#
IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  # Check if Gecode is configured with --enable-debug
  FILE(STRINGS ${GECODE_BIN}/config.status GECODE_DEBUG_BUILD REGEX "S\\[\"DEBUG_BUILD\"\\]=")
  IF(GECODE_DEBUG_BUILD MATCHES "yes")
    SET(SIBUS_BUILD_TYPE "Debug")
  ELSE()
    SET(SIBUS_BUILD_TYPE "Release")
  ENDIF()

  SET(CMAKE_BUILD_TYPE "${SIBUS_BUILD_TYPE}" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

PROJECT(SIBus)

SET(CMAKE_VERBOSE_MAKEFILE TRUE)
SET(SIBUS_THREAD FALSE CACHE BOOL "Set to true to use thread in SIBus.")

# Check if Gecode is configured with --enable-audit
FILE(STRINGS ${GECODE_BIN}/config.status GECODE_AUDIT REGEX "D\\[\"GECODE_AUDIT\"\\]=")
IF(GECODE_AUDIT)
  SET(SIBUS_AUDIT TRUE)
ELSE()
  SET(SIBUS_AUDIT FALSE)
ENDIF()
SET(LOG_AUDIT ${SIBUS_AUDIT} CACHE BOOL "Set to true to generate log output in parallel qcsp algorithm")

IF(UNIX)
  # determine, whether we want a static binary
  SET(STATIC_LINKING FALSE CACHE BOOL "Build a static binary?")
  # do we want static libraries?
  IF(STATIC_LINKING)
    SET(BUILD_SHARED_LIBS OFF)
    # To know in source file that we compil static
    ADD_DEFINITIONS(-DQUACODE_STATIC_LIBS)
    # When STATIC_LINKING is TRUE, than cmake looks for libraries ending
    # with .a. This is for linux only!
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
    # Remove flags to get rid off all the -Wl,Bydnamic
    SET(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)
    SET(CMAKE_EXE_LINK_DYNAMIC_CXX_FLAGS)
    # Use static libs for Boost
    SET(Boost_USE_STATIC_LIBS ON)
    SET(Boost_USE_STATIC_RUNTIME ON)
  ELSE(STATIC_LINKING)
    SET(BUILD_SHARED_LIBS ON)
  ENDIF(STATIC_LINKING)
ELSE(UNIX)
  SET(BUILD_SHARED_LIBS ON)
ENDIF(UNIX)

INCLUDE(CheckCXXCompilerFlag)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

#  OPTION(BUILD_EXAMPLES OFF BOOL)
OPTION(BUILD_EXAMPLES "Disable Quacode examples build." OFF)
IF (SIBUS_THREAD)
  ADD_DEFINITIONS(-DSIBUS_THREAD)
ENDIF()
ADD_SUBDIRECTORY(quacode)
GET_DIRECTORY_PROPERTY(GECODE_LIBRARIES DIRECTORY quacode DEFINITION GECODE_LIBRARIES)
GET_DIRECTORY_PROPERTY(QUACODE_LIBRARIES DIRECTORY quacode DEFINITION QUACODE_LIBRARIES)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/quacode)
INCLUDE_DIRECTORIES(${GECODE_BIN})
INCLUDE_DIRECTORIES(${GECODE_SRC})

IF (CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS(-Wall)
  ADD_DEFINITIONS(-Wextra)
  ADD_DEFINITIONS(-Wno-unused-local-typedefs)
  ADD_DEFINITIONS(-fimplement-inlines)
  ADD_DEFINITIONS(-fno-inline-functions)
  ADD_DEFINITIONS(-pipe)
  ADD_DEFINITIONS(-fPIC)
  SET(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
ENDIF ()

FIND_PACKAGE(Threads)

check_cxx_compiler_flag(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN_FLAG)
IF (HAVE_VISIBILITY_HIDDEN_FLAG)
  SET(GECODE_GCC_HAS_CLASS_VISIBILITY "/**/")
  ADD_DEFINITIONS(-fvisibility=hidden)
ENDIF()

IF (LOG_AUDIT)
  ADD_DEFINITIONS(-DLOG_AUDIT)
ENDIF()

SET(SIBUS_HEADERS
  sibus/sibus.hh
  sibus/receivers/receiver-out.hh
  sibus/receivers/receiver-gecode.hh
  sibus/receivers/receiver-compil-certif.hh
  sibus/receivers/receiver-nodecount.hh
)
SET(SIBUS_HEADERS_NEED_THREADS
  sibus/receivers/receiver-network.hh
)
SET(SIBUS_HPP
  sibus/sibus.hpp
)
SET(SIBUS_SRCS
  sibus/sibus.cpp
  sibus/receivers/receiver-out.cpp
  sibus/receivers/receiver-network.cpp
  sibus/receivers/receiver-gecode.cpp
  sibus/receivers/receiver-compil-certif.cpp
  sibus/receivers/receiver-nodecount.cpp
)
SET(SIBUS_SRCS_NEED_THREADS
  sibus/receivers/receiver-network.cpp
)
SET(SIBUS_EXAMPLES_SRCS
  examples/sample-from-file.cpp
  examples/qbf.cpp
  examples/qdimacs.cpp
  examples/nim-fibo.cpp
  examples/matrix-game.cpp
  examples/connect-four.cpp
  examples/baker.cpp
  examples/speeddating.cpp
)
IF (DISJUNCTION)
  SET(SIBUS_EXAMPLES_SRCS ${SIBUS_EXAMPLES_SRCS} examples/disjunction.cpp)
ENDIF()

SET(SIBUS_EXAMPLES_SRCS_NEED_THREADS
  examples/baker-dbh.cpp
  examples/nim-fibo-dbh.cpp
  examples/matrix-game-dbh.cpp
)
SOURCE_GROUP("Hpp Files" REGULAR_EXPRESSION ".hpp")

SET_SOURCE_FILES_PROPERTIES(${ALL_HEADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
SET_SOURCE_FILES_PROPERTIES(${ALL_HPP} PROPERTIES HEADER_FILE_ONLY TRUE)

SET(SIBUS_LIBS "")
IF (SIBUS_THREAD)
  IF(NOT Threads_FOUND)
    MESSAGE(FATAL_ERROR "Thread (pthread) library are needed, consider to install them or disable SIBus threads.")
  ELSE()
    FIND_PACKAGE(Boost 1.49.0 COMPONENTS system thread)
    IF(NOT Boost_FOUND)
      MESSAGE(STATUS "Try to find local Boost libraries.")
      SET(BOOST_LIBRARYDIR ${CMAKE_CURRENT_BINARY_DIR}/..)
      FIND_PACKAGE(Boost 1.49.0 COMPONENTS system thread)
    ENDIF()
    IF(NOT Boost_FOUND)
      MESSAGE(FATAL_ERROR "Boost thread and system library are needed, consider to install them or disable SIBus threads.")
    ELSE()
      INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIRS} )
      ADD_DEFINITIONS(-DSIBUS_THREAD)
      ADD_LIBRARY(sibus ${SIBUS_SRCS} ${SIBUS_SRCS_NEED_THREADS} ${SIBUS_HEADERS} ${SIBUS_HEADERS_NEED_THREADS} ${SIBUS_HPP})
      SET(SIBUS_LIBS sibus ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
    ENDIF()
  ENDIF()
ELSE()
  ADD_LIBRARY(sibus ${SIBUS_SRCS} ${SIBUS_HEADERS} ${SIBUS_HPP})
  SET(SIBUS_LIBS sibus)
ENDIF ()
SET_TARGET_PROPERTIES (sibus PROPERTIES DEFINE_SYMBOL  "BUILD_SIBUS")
IF (HAVE_VISIBILITY_HIDDEN_FLAG)
  SET_TARGET_PROPERTIES(sibus PROPERTIES COMPILE_FLAGS "-DSIBUS_GCC_HAS_CLASS_VISIBILITY")
ENDIF ()

# Add targets for examples
FOREACH (example ${SIBUS_EXAMPLES_SRCS})
  GET_FILENAME_COMPONENT(exampleBin ${example} NAME_WE)
  ADD_EXECUTABLE(${exampleBin} ${example})
  TARGET_LINK_LIBRARIES(${exampleBin} ${QUACODE_LIBRARIES} ${SIBUS_LIBS} ${GECODE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

  # set -static, when STATIC_LINKING is TRUE and set LINK_SEARCH_END_STATIC
  # to remove the additional -bdynamic from the linker line.
  IF(STATIC_LINKING)
      SET(CMAKE_EXE_LINKER_FLAGS "-static")
      SET_TARGET_PROPERTIES(${exampleBin} PROPERTIES LINK_SEARCH_END_STATIC 1)
  ENDIF(STATIC_LINKING)

  IF (BUILD_SHARED_LIBS)
    ADD_DEPENDENCIES(${exampleBin} quacode)
    ADD_DEPENDENCIES(${exampleBin} sibus)
  ENDIF()
ENDFOREACH ()

# Add targets for examples which need threads
IF (SIBUS_THREAD)
  FOREACH (example ${SIBUS_EXAMPLES_SRCS_NEED_THREADS})
    GET_FILENAME_COMPONENT(exampleBin ${example} NAME_WE)
    ADD_EXECUTABLE(${exampleBin} ${example})
    TARGET_LINK_LIBRARIES(${exampleBin} ${QUACODE_LIBRARIES} ${SIBUS_LIBS} ${GECODE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

    # set -static, when STATIC_LINKING is TRUE and set LINK_SEARCH_END_STATIC
    # to remove the additional -Bdynamic from the linker line.
    IF(UNIX AND STATIC_LINKING)
        SET(CMAKE_EXE_LINKER_FLAGS "-static")
        SET_TARGET_PROPERTIES(${exampleBin} PROPERTIES LINK_SEARCH_END_STATIC 1)
    ENDIF(UNIX AND STATIC_LINKING)

    IF (BUILD_SHARED_LIBS)
      ADD_DEPENDENCIES(${exampleBin} quacode)
      ADD_DEPENDENCIES(${exampleBin} sibus)
    ENDIF()
  ENDFOREACH ()
ENDIF()

