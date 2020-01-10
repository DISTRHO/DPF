# Use pkg-config to get hints about paths
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
        pkg_check_modules(JACK_PKGCONF jack)
endif(PKG_CONFIG_FOUND)

# Include dir
find_path(JACK_INCLUDE_DIR
	NAMES jack/jack.h
	PATH_SUFFIXES include includes
        PATHS  ${JACK_PKGCONF_INCLUDEDIR}
)

# Library
find_library(JACK_LIBRARY
	NAMES jack jack64 libjack libjack64
	PATH_SUFFIXES lib
        PATHS ${JACK_PKGCONF_LIBDIR}
)

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(Jack  DEFAULT_MSG  JACK_LIBRARY JACK_INCLUDE_DIR)

if(JACK_FOUND)
  set(JACK_LIBRARIES ${JACK_LIBRARY})
  set(JACK_INCLUDE_DIRS ${JACK_INCLUDE_DIR})
endif(JACK_FOUND)

mark_as_advanced(JACK_LIBRARY JACK_INCLUDE_DIR
    #JACK_LIBRARIES JACK_INCLUDE_DIRS
    )
