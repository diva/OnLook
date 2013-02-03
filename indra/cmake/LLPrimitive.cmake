# -*- cmake -*-

include(Colladadom)

set(LLPRIMITIVE_INCLUDE_DIRS
  ${LIBS_OPEN_DIR}/llprimitive
  ${COLLADADOM_INCLUDE_DIRS}
  )

if (WINDOWS)
  set(LLPRIMITIVE_LIBRARIES 
	  debug llprimitive
	  optimized llprimitive
	  ${COLLADADOM_LIBRARIES}
	  )
else (WINDOWS)
  set(LLPRIMITIVE_LIBRARIES 
	  llprimitive
	  ${COLLADADOM_LIBRARIES}
	  )
endif (WINDOWS)

