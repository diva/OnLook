# -*- cmake -*-
include(Prebuilt)

set(GLOD_FIND_QUIETLY OFF)
set(GLOD_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindGLOD)
else (STANDALONE)
  use_prebuilt_binary(GLOD)
  set(GLOD_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
  set(GLOD_LIBRARIES glod)
endif (STANDALONE)
