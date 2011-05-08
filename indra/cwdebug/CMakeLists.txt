# -*- cmake -*-

project(cwdebug)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLVFS)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

set(cwdebug_SOURCE_FILES
    debug.cc
    )

set(cwdebug_HEADER_FILES
    CMakeLists.txt

    cwdebug.h
    sys.h
    debug.h
    debug_ostream_operators.h
    )

set_source_files_properties(${cwdebug_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_definitions(-fPIC)

list(APPEND cwdebug_SOURCE_FILES ${cwdebug_HEADER_FILES})

add_library (cwdebug ${cwdebug_SOURCE_FILES})
