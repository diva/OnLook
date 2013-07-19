# -*- cmake -*-
include(Prebuilt)

set(Boost_FIND_QUIETLY ON)
set(Boost_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindBoost)

  set(Boost_USE_MULTITHREADED ON)
  find_package(Boost 1.40.0 COMPONENTS date_time filesystem program_options regex system thread wave)
else (STANDALONE)
  use_prebuilt_binary(boost)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)

  if (WINDOWS)
    set(BOOST_VERSION 1_45)
    set(BOOST_OPTIM_SUFFIX mt)
    set(BOOST_DEBUG_SUFFIX mt-gd)

    set(Boost_PROGRAM_OPTIONS_LIBRARY 
          optimized libboost_program_options-vc${MSVC_SUFFIX}-${BOOST_OPTIM_SUFFIX}-${BOOST_VERSION}
          debug libboost_program_options-vc${MSVC_SUFFIX}-${BOOST_DEBUG_SUFFIX}-${BOOST_VERSION})
    set(Boost_REGEX_LIBRARY
          optimized libboost_regex-vc${MSVC_SUFFIX}-${BOOST_OPTIM_SUFFIX}-${BOOST_VERSION}
          debug libboost_regex-vc${MSVC_SUFFIX}-${BOOST_DEBUG_SUFFIX}-${BOOST_VERSION})

  elseif (DARWIN)
    set(Boost_FILESYSTEM_LIBRARY boost_filesystem)
    set(Boost_PROGRAM_OPTIONS_LIBRARY boost_program_options)
    set(Boost_REGEX_LIBRARY boost_regex)
    set(Boost_SYSTEM_LIBRARY boost_system)  
    set(Boost_DATE_TIME_LIBRARY boost_date_time)
  elseif (LINUX)
    set(Boost_FILESYSTEM_LIBRARY boost_filesystem-mt)
    set(Boost_PROGRAM_OPTIONS_LIBRARY boost_program_options-mt)
    set(Boost_REGEX_LIBRARY boost_regex-mt)
    set(Boost_SYSTEM_LIBRARY boost_system-mt)  
    set(Boost_DATE_TIME_LIBRARY boost_date_time-mt)
  endif (WINDOWS)
endif (STANDALONE)
