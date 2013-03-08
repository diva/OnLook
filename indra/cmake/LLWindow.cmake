# -*- cmake -*-

include(OpenGL)
include(Prebuilt)

if (STANDALONE)
  include(FindSDL)

  # This should be done by FindSDL.  Sigh.
  mark_as_advanced(
      SDLMAIN_LIBRARY
      SDL_INCLUDE_DIR
      SDL_LIBRARY
      )
else (STANDALONE)
  if (LINUX)
    use_prebuilt_binary(mesa)
    use_prebuilt_binary(SDL)
    set (SDL_FOUND TRUE)
    set (SDL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR})
    if(WORD_SIZE EQUAL 64)
      set (SDL_LIBRARY SDL)
    else(WORD_SIZE EQUAL 64)
      set (SDL_LIBRARY SDL directfb fusion direct)
    endif(WORD_SIZE EQUAL 64)
  endif (LINUX)
endif (STANDALONE)

if (SDL_FOUND)
  add_definitions(-DLL_SDL=1)
  include_directories(${SDL_INCLUDE_DIR})
endif (SDL_FOUND)

set(LLWINDOW_INCLUDE_DIRS
    ${GLEXT_INCLUDE_DIR}
    ${LIBS_OPEN_DIR}/llwindow
    )

set(LLWINDOW_LIBRARIES
    llwindow
    )

if (WINDOWS)
    list(APPEND LLWINDOW_LIBRARIES
        comdlg32
        )
endif (WINDOWS)
