cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0015 NEW)
project (metar_c_decoder LANGUAGES C)

set (CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set (CMAKE_BUILD_FILES_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set (PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set (PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS "-g")

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_INCLUDE_DIR})

set (SOURCES
        ${PROJECT_INCLUDE_DIR}/metar.h
        ${PROJECT_SOURCE_DIR}/metar.c
        ${CMAKE_SOURCE_DIR}/test/metar_test.c
)

add_executable(metar_test
    ${SOURCES}
)


