# -*- cmake -*-

project(libpathing)
include(00-Common)

if( MSVC )
   add_definitions(-D_SECURE_SCL=0 -D_CRT_SECURE_NO_WARNINGS=1)
endif( MSVC )

set(libpathing_SOURCE_FILES
    llpathinglib.cpp
    llphysicsextensions.cpp
)

set(libpathing_HEADER_FILES
    llpathinglib.h
    llphysicsextensions.h
)

set_source_files_properties(${libpathing_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(nd_Pathing STATIC ${libpathing_SOURCE_FILES} ${libpathing_HEADER_FILES} )

