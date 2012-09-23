# -*- cmake -*-

if (STANDALONE)
  set(LLQTWEBKIT_INCLUDE_DIR
      ${LIBS_OPEN_DIR}/llqtwebkit
      )

  set(LLQTWEBKIT_LIBRARY
      llqtwebkit
      )
endif (STANDALONE)
