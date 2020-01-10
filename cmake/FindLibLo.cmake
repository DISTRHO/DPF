# Use pkg-config to get hints about paths
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBLO_PKGCONF liblo)
endif(PKG_CONFIG_FOUND)

# Include dir
find_path(LIBLO_INCLUDE_DIR
        NAMES lo/lo.h
        PATH_SUFFIXES include includes
        PATHS  ${LIBLO_PKGCONF_INCLUDEDIR}
)

# Library
find_library(LIBLO_LIBRARY
        NAMES lo liblo
        PATH_SUFFIXES lib
        PATHS ${LIBLO_PKGCONF_LIBDIR}
)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(LibLo  DEFAULT_MSG  LIBLO_LIBRARY LIBLO_INCLUDE_DIR)

if(LIBLO_FOUND)
  set(LIBLO_LIBRARIES ${LIBLO_LIBRARY})
  set(LIBLO_INCLUDE_DIRS ${LIBLO_INCLUDE_DIR})
endif(LIBLO_FOUND)

mark_as_advanced(LIBLO_LIBRARY LIBLO_INCLUDE_DIR
    #JACK_LIBRARIES JACK_INCLUDE_DIRS
    )
