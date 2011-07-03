# -*- cmake -*-

include(APR)
include(Boost)
include(EXPAT)
include(ZLIB)

if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(CORESERVICES_LIBRARY CoreServices)
endif (DARWIN)


set(LLCOMMON_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/cwdebug
    ${LIBS_OPEN_DIR}/llcommon
    ${APR_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    )

set(LLCOMMON_LIBRARIES llcommon)
