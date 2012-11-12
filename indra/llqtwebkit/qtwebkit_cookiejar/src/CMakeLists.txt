# -*- cmake -*-

project(networkcookiejar)

set(networkcookiejar_SOURCE_FILES
      networkcookiejar.cpp
    )

set(networkcookiejar_HEADER_FILES
      networkcookiejar.h
      networkcookiejar_p.h
      trie_p.h
      twoleveldomains_p.h

    )

QT4_WRAP_CPP(networkcookiejar_HEADERS_MOC ${networkcookiejar_HEADER_FILES})

add_library(networkcookiejar
    ${networkcookiejar_SOURCE_FILES}
    ${networkcookiejar_HEADERS_MOC}
    ${networkcookiejar_UI_MOC}
)

add_dependencies(networkcookiejar prepare)

target_link_libraries(networkcookiejar)