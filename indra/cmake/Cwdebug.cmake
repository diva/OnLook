include_directories (${CMAKE_SOURCE_DIR}/cwdebug)

set(cwdebug_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/cwdebug/debug.cc
    )

set(cwdebug_HEADER_FILES
    ${CMAKE_SOURCE_DIR}/cwdebug/cwdebug.h
    ${CMAKE_SOURCE_DIR}/cwdebug/sys.h
    ${CMAKE_SOURCE_DIR}/cwdebug/debug.h
    ${CMAKE_SOURCE_DIR}/cwdebug/debug_ostream_operators.h
    )

set_source_files_properties(${cwdebug_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND cwdebug_SOURCE_FILES ${cwdebug_HEADER_FILES})

