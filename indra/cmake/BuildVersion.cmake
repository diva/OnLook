# -*- cmake -*-

# Read version components from the header file.
file(STRINGS ${LIBS_OPEN_DIR}/llcommon/llversionviewer.h.in lines
   REGEX " LL_VERSION_")
foreach(line ${lines})
  string(REGEX REPLACE ".*LL_VERSION_([A-Z]+).*" "\\1" comp "${line}")
  string(REGEX REPLACE ".* = ([0-9]+);.*" "\\1" value "${line}")
  set(v${comp} "${value}")
endforeach(line)

execute_process(
    COMMAND git rev-list HEAD
    OUTPUT_VARIABLE GIT_REV_LIST_STR
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(GIT_REV_LIST_STR)
  string(REPLACE "\n" ";" GIT_REV_LIST ${GIT_REV_LIST_STR})
else()
  string(REPLACE "\n" ";" GIT_REV_LIST "")
endif()

if(GIT_REV_LIST)
  list(LENGTH GIT_REV_LIST vBUILD)
else()
  set(vBUILD 99)
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/llcommon/llversionviewer.h.in
    ${CMAKE_SOURCE_DIR}/llcommon/llversionviewer.h
)

if (WINDOWS)
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/res/viewerRes.rc.in
       ${CMAKE_SOURCE_DIR}/newview/res/viewerRes.rc
   )
   
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/res/viewerRes_bc.rc.in
       ${CMAKE_SOURCE_DIR}/newview/res/viewerRes_bc.rc
   )
endif (WINDOWS)

if (DARWIN)
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/English.lproj/InfoPlist.strings.in
       ${CMAKE_SOURCE_DIR}/newview/English.lproj/InfoPlist.strings
   )
endif (DARWIN)

if (LINUX)
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/wrapper.sh.in
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/wrapper.sh
       @ONLY
   )
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/handle_secondlifeprotocol.sh.in
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/handle_secondlifeprotocol.sh
       @ONLY
   )
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/install.sh.in
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/install.sh
       @ONLY
   )
   configure_file(
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/refresh_desktop_app_entry.sh.in
       ${CMAKE_SOURCE_DIR}/newview/linux_tools/refresh_desktop_app_entry.sh
       @ONLY
   )
endif (LINUX)

# Compose the version.
set(viewer_VERSION "${vMAJOR}.${vMINOR}.${vPATCH}.${vBUILD}")
if (viewer_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(STATUS "Version is ${viewer_VERSION}")
else (viewer_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(FATAL_ERROR "Could not determine version (${viewer_VERSION})")
endif (viewer_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")

# Report version to caller.
#set(viewer_VERSION "${viewer_VERSION}" PARENT_SCOPE)
