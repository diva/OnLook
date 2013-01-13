cmake_minimum_required(VERSION 2.6.4)

project(libhacd CXX C)

file (GLOB SOURCE_FILES *.cpp )
file (GLOB INCLUDE_FILES *.h )

add_library(hacd STATIC ${SOURCE_FILES} ${INCLUDE_FILES} )

