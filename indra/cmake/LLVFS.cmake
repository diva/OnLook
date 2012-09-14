# -*- cmake -*-

include(Boost)

set(LLVFS_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llvfs
    )

set(LLVFS_LIBRARIES
    llvfs
    ${Boost_REGEX_LIBRARY}
    )
