# -*- cmake -*-

project(llqtwebkit)

include(00-Common)
include(Qt4)

if(NOT WORD_SIZE EQUAL 32)
  if(WINDOWS)
    add_definitions(/FIXED:NO)
  else(WINDOWS)
    add_definitions(-fPIC)
  endif(WINDOWS)
endif(NOT WORD_SIZE EQUAL 32)

include_directories(${QT_INCLUDES})

add_subdirectory(qtwebkit_cookiejar)
include_directories(qtwebkit_cookiejar/src/)

set(llqtwebkit_SOURCE_FILES
      llembeddedbrowser.cpp
      llembeddedbrowserwindow.cpp
      lljsobject.cpp
      llnetworkaccessmanager.cpp
      llqtwebkit.cpp
      llstyle.cpp
      llwebpage.cpp
      llwebpageopenshim.cpp
    )

set(llqtwebkit_HEADER_FILES
      llembeddedbrowser.h
      llembeddedbrowser_p.h
      llembeddedbrowserwindow.h
      llembeddedbrowserwindow_p.h
      lljsobject.h
      llnetworkaccessmanager.h
      llqtwebkit.h
      llstyle.h
      llwebpage.h
      llwebpageopenshim.h
      pstdint.h
    )

set(llqtwebkit_UI_FILES
      passworddialog.ui
    )

set(llqtwebkit_LINK_LIBRARIES
      networkcookiejar
)

QT4_WRAP_UI(llqtwebkit_UI_MOC ${llqtwebkit_UI_FILES})
QT4_WRAP_CPP(llqtwebkit_HEADERS_MOC ${llqtwebkit_HEADER_FILES})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(llqtwebkit
    ${llqtwebkit_SOURCE_FILES}
    ${llqtwebkit_HEADERS_MOC}
    ${llqtwebkit_UI_MOC}
)

add_dependencies(llqtwebkit prepare)

target_link_libraries(llqtwebkit ${llqtwebkit_LINK_LIBRARIES})

add_dependencies(llqtwebkit
  networkcookiejar
)
