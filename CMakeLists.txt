########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.0)
project(revdgst C)

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

#local include directories first
#include_directories(${PROJECT_SOURCE_DIR}/src/utils)

#include local cmake modules
#list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

########################################################################
# Dependencies
########################################################################

#link with libm when available
find_library(
    MATH_LIBRARIES NAMES m
    PATHS /usr/lib /usr/lib64
)
if (MATH_LIBRARIES)
    list(APPEND TOOLS_LIBS ${MATH_LIBRARIES})
endif ()

#link with pthreads
set(THREADS_USE_PTHREADS_WIN32 true)
find_package(Threads)
if (NOT THREADS_FOUND)
    message(FATAL_ERROR "pthreads development files not found...")
endif ()
include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
list(APPEND TOOLS_LIBS ${CMAKE_THREAD_LIBS_INIT})
message(STATUS "THREADS_PTHREADS_INCLUDE_DIR: ${THREADS_PTHREADS_INCLUDE_DIR}")
message(STATUS "CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}")

# target_compile_definitions was only added with CMake 2.8.11
#if(THREADS_FOUND)
#	set_target_properties(tx_srv PROPERTIES COMPILE_DEFINITIONS "THREADS")
#endif()
#if(THREADS_HAVE_PTHREAD_ARG)
#    set_target_properties(tx_srv PROPERTIES COMPILE_OPTIONS "-pthread")
#    set_target_properties(tx_srv PROPERTIES INTERFACE_COMPILE_OPTIONS "-pthread")
#endif()
#if(CMAKE_THREAD_LIBS_INIT)
#    target_link_libraries(tx_srv "${CMAKE_THREAD_LIBS_INIT}")
#endif()

#windows getopt compatibility
#if (WIN32)
#    include_directories(${PROJECT_SOURCE_DIR}/src/getopt)
#    list(APPEND COMMON_SOURCES src/getopt/getopt.c)
#endif ()

########################################################################
# Helper library
########################################################################
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
list(APPEND COMMON_SOURCES src/codes.c)
#add_library(common STATIC ${COMMON_SOURCES})
#list(APPEND TOOLS_LIBS common)

message(STATUS "CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")

########################################################################
# Compiler specific setup
########################################################################
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" MATCHES "Clang") AND NOT WIN32)
    ADD_DEFINITIONS(-pedantic)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-Wextra)
    ADD_DEFINITIONS(-Wno-sign-compare)
    ADD_DEFINITIONS(-Wno-unused-parameter)
    ADD_DEFINITIONS(-Wno-unused-function)
    ADD_DEFINITIONS(-Wdouble-promotion)
    ADD_DEFINITIONS(-Wformat-nonliteral)
    ADD_DEFINITIONS(-Wformat-security)

    # CMake Release default for GCC/Clang is "-O3 -DNDEBUG"
    # set(CMAKE_C_FLAGS_RELEASE -O2)
    # CMake Debug default for GCC/Clang is "-g -DNDEBUG"
    # set(CMAKE_C_FLAGS_DEBUG -g3 -O0)

    # make use of ASAN
    set(CMAKE_C_FLAGS_DEBUG "-ggdb -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer")
endif()
if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
    # make sure we don't accidentally copy more than an int
    ADD_DEFINITIONS(-Wlarge-by-value-copy=8)
    ADD_DEFINITIONS(-Weverything)
    ADD_DEFINITIONS(-Wno-conversion)
    ADD_DEFINITIONS(-Wno-sign-conversion)
endif()

########################################################################
# Build executables
########################################################################
add_executable(shft src/shft.c ${COMMON_SOURCES})
target_link_libraries(shft ${TOOLS_LIBS})

add_executable(bitbrk src/bitbrk.c ${COMMON_SOURCES})
target_link_libraries(bitbrk ${TOOLS_LIBS})

add_executable(revsum src/revsum.c ${COMMON_SOURCES})
target_link_libraries(revsum ${TOOLS_LIBS})

add_executable(revdgst src/revdgst.c ${COMMON_SOURCES})
target_link_libraries(revdgst ${TOOLS_LIBS})

add_executable(revdgst16 src/revdgst16.c ${COMMON_SOURCES})
target_link_libraries(revdgst16 ${TOOLS_LIBS})

add_executable(keylst src/keylst.c ${COMMON_SOURCES})
target_link_libraries(keylst ${TOOLS_LIBS})

add_executable(chkcrc src/chkcrc.c ${COMMON_SOURCES})
target_link_libraries(chkcrc ${TOOLS_LIBS})

########################################################################
# Install executables
########################################################################
install(TARGETS bitbrk revdgst revdgst16 DESTINATION bin)
