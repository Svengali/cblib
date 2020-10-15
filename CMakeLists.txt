cmake_minimum_required(VERSION 3.0.0)
project(cblib VERSION 0.1.0)

include(CTest)
enable_testing()

file(GLOB_RECURSE SOURCES
    *.h
    *.cpp
    *.asm
    *.inc
)


add_library(cblib ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
