cmake_minimum_required(VERSION 3.7)

project(DPF)

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
  dpf__add_dgl_cairo()
  dpf__add_dgl_opengl()
endif()

if(DPF_EXAMPLES)
  add_subdirectory("examples/CairoUI")
  #add_subdirectory("examples/ExternalUI")
  add_subdirectory("examples/FileHandling")
  add_subdirectory("examples/Info")
  add_subdirectory("examples/Latency")
  add_subdirectory("examples/Meters")
  add_subdirectory("examples/MidiThrough")
  add_subdirectory("examples/Parameters")
  add_subdirectory("examples/States")
endif()
