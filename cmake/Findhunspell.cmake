if(TARGET hunspell)
  return()
endif()

find_path(HUNSPELL_INCLUDE_DIR
  NAME hunspell.hxx
  PATH_SUFFIXES include hunspell
  HINTS "${HUNSPELL_INSTALL_DIR}" ENV HUNSPELL_INSTALL_DIR "${CMAKE_PREFIX_PATH}"
)

find_library(HUNSPELL_LIB
  NAMES libhunspell-1.7.a hunspell-1.7 libhunspell
  PATH_SUFFIXES lib
  HINTS "${HUNSPELL_INSTALL_DIR}" ENV HUNSPELL_INSTALL_DIR "${CMAKE_PREFIX_PATH}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hunspell
  DEFAULT_MSG HUNSPELL_INCLUDE_DIR HUNSPELL_LIB
)

if(hunspell_FOUND)
  if (NOT TARGET hunspell::hunspell)
    add_library(hunspell::hunspell UNKNOWN IMPORTED)
    set_target_properties(hunspell::hunspell
      PROPERTIES
        IMPORTED_LOCATION "${HUNSPELL_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${HUNSPELL_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(HUNSPELL_INCLUDE_DIR HUNSPELL_LIB)

include(FeatureSummary)
set_package_properties(hunspell PROPERTIES
  URL "https://hunspell.github.io"
  DESCRIPTION "Hunspell spell checker"
)
