cmake_minimum_required(VERSION 3.0.0)

#include(${CMAKE_TOOLS_DIR}/Macros.cmake)
include(Macros.cmake)

project(cblib VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)


include(CTest)
enable_testing()

file(GLOB_RECURSE SOURCES
    *.h
    *.cpp
    *.asm
    *.inc
)

GenerateSourceGroups(.)


add_library(cblib ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
