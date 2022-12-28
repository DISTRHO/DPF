# DISTRHO Plugin Framework (DPF)
# Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
#
# SPDX-License-Identifier: ISC

cmake_minimum_required(VERSION 3.7)

project(DPF)

# ensure c++11 at minimum, the parent project can override
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()

# check if we are building from this project, or are imported by another
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(DPF_BUILD_FROM_HERE TRUE)
else()
  set(DPF_BUILD_FROM_HERE FALSE)
endif()

option(DPF_LIBRARIES "Build the libraries" "${DPF_BUILD_FROM_HERE}")
option(DPF_EXAMPLES "Build the examples" "${DPF_BUILD_FROM_HERE}")

set(DPF_ROOT_DIR "${PROJECT_SOURCE_DIR}" CACHE INTERNAL
  "Root directory of the DISTRHO Plugin Framework.")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(DPF-plugin)

if(DPF_LIBRARIES)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(CAIRO "cairo")
    if(CAIRO_FOUND)
      dpf__add_dgl_cairo(FALSE)
    endif()
  endif()
  dpf__add_dgl_opengl(FALSE)
endif()

if(DPF_EXAMPLES)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(CAIRO "cairo")
    if(CAIRO_FOUND)
      add_subdirectory("examples/CairoUI")
    endif()
  endif()
  add_subdirectory("examples/ExternalUI")
  add_subdirectory("examples/EmbedExternalUI")
  add_subdirectory("examples/FileHandling")
  add_subdirectory("examples/Info")
  add_subdirectory("examples/Latency")
  add_subdirectory("examples/Meters")
  add_subdirectory("examples/MidiThrough")
  add_subdirectory("examples/Parameters")
  add_subdirectory("examples/States")
endif()
