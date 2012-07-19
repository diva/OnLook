# -*- cmake -*-
include(Prebuilt)

set(PNG_FIND_QUIETLY ON)
set(PNG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindPNG)
else (STANDALONE)
  use_prebuilt_binary(libpng)
  if (WINDOWS)
    set(PNG_LIBRARIES libpng15)
  elseif(DARWIN)
    set(PNG_LIBRARIES png15)
  else(LINUX)
    set(PNG_LIBRARIES png15)
  endif()
  set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/)
endif (STANDALONE)
