# -*- cmake -*-
include(Prebuilt)

set(CURL_FIND_QUIETLY OFF)
set(CURL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindCURL)
else (STANDALONE)
  use_prebuilt_binary(curl)
  if (WINDOWS)
    set(CURL_LIBRARIES 
    debug libcurld
    optimized libcurl)
  else (WINDOWS)
    set(CURL_LIBRARIES curl)
    if(LINUX AND WORD_SIZE EQUAL 64)
      list(APPEND CURL_LIBRARIES idn)
    endif(LINUX AND WORD_SIZE EQUAL 64)
  endif (WINDOWS)
  set(CURL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
endif (STANDALONE)
