cmake_minimum_required(VERSION 2.6.4)

project(libhacd CXX C)
include(00-Common)

file (GLOB SOURCE_FILES *.cpp )
file (GLOB INCLUDE_FILES *.h )

IF(WINDOWS)
   add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF(WINDOWS)

add_library(hacd ${SOURCE_FILES} ${INCLUDE_FILES})
