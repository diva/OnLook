# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(ndPhysicsStub)

set(LLPHYSICSEXTENSIONS_LIBRARIES nd_hacdConvexDecomposition hacd nd_Pathing )
set(LLPHYSICSEXTENSIONS_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include/ )



