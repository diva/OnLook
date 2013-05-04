cmake_minimum_required(VERSION 2.6.4)

project(ndPathingLib CXX C)

if( MSVC )
   add_definitions(-D_SECURE_SCL=0 -D_CRT_SECURE_NO_WARNINGS=1)
endif( MSVC )

file (GLOB SOURCE_FILES *.cpp )
file (GLOB INCLUDE_FILES *.h )

add_library(nd_Pathing STATIC ${SOURCE_FILES} ${INCLUDE_FILES} )

