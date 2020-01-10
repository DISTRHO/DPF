find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
        pkg_check_modules(CAIRO_PKGCONF cairo)
        pkg_check_modules(EXPAT_PKGCONF expat)
        pkg_check_modules(FONT_CONFIG_PKGCONF fontconfig)
        pkg_check_modules(PIXMAN_PKGCONF pixman)
endif(PKG_CONFIG_FOUND)

# Include dir
find_path(CAIRO_INCLUDE_DIR
        NAMES cairo/cairo.h
        PATH_SUFFIXES include includes
        PATHS  ${CAIRO_PKGCONF_INCLUDEDIR}
)

# Library
find_library(CAIRO_LIBRARY
        NAMES cairo
        PATH_SUFFIXES lib
        PATHS ${CAIRO_PKGCONF_LIBDIR}
)
find_library(EXPAT_RUNTIME_LIBRARY
        NAMES expat
        PATH_SUFFIXES lib
        PATHS ${EXPAT_PKGCONF_LIBDIR}
)
find_library(FONTCONFIG_RUNTIME_LIBRARY
        NAMES fontconfig
        PATH_SUFFIXES lib
        PATHS ${FONT_CONFIG_PKGCONF_LIBDIR}
)
find_library(PIXMAN_RUNTIME_LIBRARY
        NAMES pixman
        PATH_SUFFIXES lib
        PATHS ${PIXMAN_PKGCONF_LIBDIR}
)
#find_file(CAIRO_RUNTIME_LIBRARY NAMES cairo.dll)
#find_file(PIXMAN_RUNTIME_LIBRARY NAMES pixman-1.dll)

include(FindPackageHandleStandardArgs)
if (WIN32)
    find_package_handle_standard_args(CAIRO DEFAULT_MSG
        CAIRO_INCLUDE_DIR
        CAIRO_LIBRARY

        CAIRO_RUNTIME_LIBRARY
        EXPAT_RUNTIME_LIBRARY
        FONTCONFIG_RUNTIME_LIBRARY
        PIXMAN_RUNTIME_LIBRARY
    )
else()
    #TODO EXPAR AND FONTCONFIG Required?
    find_package_handle_standard_args(CAIRO DEFAULT_MSG
        CAIRO_INCLUDE_DIR
        CAIRO_LIBRARY
    )
endif()

mark_as_advanced(
    CAIRO_INCLUDE_DIR
    CAIRO_LIBRARY

    CAIRO_RUNTIME_LIBRARY
    EXPAT_RUNTIME_LIBRARY
    FONTCONFIG_RUNTIME_LIBRARY
    PIXMAN_RUNTIME_LIBRARY
)

set(CAIRO_INCLUDE_DIRS ${CAIRO_INCLUDE_DIR})

set(CAIRO_LIBRARIES ${CAIRO_LIBRARY})

set(CAIRO_RUNTIME_LIBRARIES
    ${CAIRO_RUNTIME_LIBRARY}
    ${EXPAT_RUNTIME_LIBRARY}
    ${FONTCONFIG_RUNTIME_LIBRARY}
    ${PIXMAN_RUNTIME_LIBRARY}
)
