cmake_minimum_required(VERSION 3.7)

project(DPF)

option(DPF_EXAMPLES "Build the examples" OFF)

set(DPF_ROOT_DIR "${PROJECT_SOURCE_DIR}" CACHE INTERNAL
  "Root directory of the DISTRHO Plugin Framework.")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(DPF-plugin)

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
