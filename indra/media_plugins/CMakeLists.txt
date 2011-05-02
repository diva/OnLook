# -*- cmake -*-

add_subdirectory(base)

add_subdirectory(webkit)

if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (DARWIN OR WINDOWS)
    add_subdirectory(quicktime)
endif (DARWIN OR WINDOWS)

add_subdirectory(example)
