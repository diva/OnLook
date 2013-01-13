project(libndhacd)

include_directories(${LIBS_OPEN_DIR}/libhacd)

set (SOURCE_FILES LLConvexDecomposition.cpp nd_hacdConvexDecomposition.cpp nd_hacdStructs.cpp nd_hacdUtils.cpp nd_EnterExitTracer.cpp nd_StructTracer.cpp )
file(GLOB HEADER_FILES *.h)

add_library( nd_hacdConvexDecomposition STATIC ${SOURCE_FILES} ${HEADER_FILES})

