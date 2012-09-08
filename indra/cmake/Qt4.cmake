# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  set(Qt4_FIND_REQUIRED ON)

  include(FindQt4)

  find_package(Qt4 4.7.0 COMPONENTS QtCore QtGui QtNetwork QtOpenGL QtWebKit REQUIRED)
  include(${QT_USE_FILE})
  add_definitions(${QT_DEFINITIONS})
endif (STANDALONE)
