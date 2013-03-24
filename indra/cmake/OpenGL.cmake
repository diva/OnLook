# -*- cmake -*-
include(Prebuilt)

if (NOT (STANDALONE OR DARWIN))
  use_prebuilt_binary(glext)
  set(GLEXT_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
endif (NOT (STANDALONE OR DARWIN))
