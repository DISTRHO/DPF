include(CMakeParseArguments)

# ------------------------------------------------------------------------------
# DPF public functions
# ------------------------------------------------------------------------------

# dpf_add_plugin(name <args...>)
# ------------------------------------------------------------------------------
#
# Add a plugin built using the DISTRHO Plugin Framework.
#
# ------------------------------------------------------------------------------
# Created targets:
#
#   `<name>`
#       static library: the common part of the plugin
#       The public properties set on this target apply to both DSP and UI.
#
#   `<name>-dsp`
#       static library: the DSP part of the plugin
#       The public properties set on this target apply to the DSP only.
#
#   `<name>-ui`
#       static library: the UI part of the plugin
#       The public properties set on this target apply to the UI only.
#
#   `<name>-<target>` for each target specified with the `TARGETS` argument.
#       This is target-dependent and not intended for public use.
#
# ------------------------------------------------------------------------------
# Arguments:
#
#   `TARGETS` <tgt1>...<tgtN>
#       a list of one of more of the following target types:
#       `jack`, `ladspa`, `dssi`, `lv2`, `vst`
#
#   `UI_TYPE` <type>
#       the user interface type: `opengl` (default), `cairo`
#
#   `MONOLITHIC`
#       build LV2 as a single binary for UI and DSP
#
#   `FILES_DSP` <file1>...<fileN>
#       list of sources which are part of the DSP
#
#   `FILES_UI` <file1>...<fileN>
#       list of sources which are part of the UI
#       empty indicates the plugin does not have UI
#
#   `FILES_COMMON` <file1>...<fileN>
#       list of sources which are part of both DSP and UI
#
function(dpf_add_plugin NAME)
  set(options MONOLITHIC)
  set(oneValueArgs UI_TYPE)
  set(multiValueArgs TARGETS FILES_DSP FILES_UI)
  cmake_parse_arguments(_dpf_plugin "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if("${_dpf_plugin_UI_TYPE}" STREQUAL "")
    set(_dpf_plugin_UI_TYPE "opengl")
  endif()

  set(_dgl_library)
  if(_dpf_plugin_FILES_UI)
    if(_dpf_plugin_UI_TYPE STREQUAL "cairo")
      dpf__add_dgl_cairo()
      set(_dgl_library dgl-cairo)
    elseif(_dpf_plugin_UI_TYPE STREQUAL "opengl")
      dpf__add_dgl_opengl()
      set(_dgl_library dgl-opengl)
    else()
      message(FATAL_ERROR "Unrecognized UI type for plugin: ${_dpf_plugin_UI_TYPE}")
    endif()
  endif()

  ###
  dpf__ensure_sources_non_empty(_dpf_plugin_FILES_COMMON)
  dpf__ensure_sources_non_empty(_dpf_plugin_FILES_DSP)
  dpf__ensure_sources_non_empty(_dpf_plugin_FILES_UI)

  ###
  dpf__add_static_library("${NAME}" ${_dpf_plugin_FILES_COMMON})
  target_include_directories("${NAME}" PUBLIC
    "${DPF_ROOT_DIR}/distrho")

  if(_dgl_library)
    # make sure that all code will see DGL_* definitions
    target_link_libraries("${NAME}" PUBLIC "${_dgl_library}-definitions")
  endif()

  dpf__add_static_library("${NAME}-dsp" ${_dpf_plugin_FILES_DSP})
  target_link_libraries("${NAME}-dsp" PUBLIC "${NAME}")

  if(_dgl_library)
    dpf__add_static_library("${NAME}-ui" ${_dpf_plugin_FILES_UI})
    target_link_libraries("${NAME}-ui" PUBLIC "${NAME}" ${_dgl_library})
  else()
    add_library("${NAME}-ui" INTERFACE)
  endif()

  ###
  foreach(_target ${_dpf_plugin_TARGETS})
    if(_target STREQUAL "jack")
      dpf__build_jack("${NAME}" "${_dgl_library}")
    elseif(_target STREQUAL "ladspa")
      dpf__build_ladspa("${NAME}")
    elseif(_target STREQUAL "dssi")
      dpf__build_dssi("${NAME}" "${_dgl_library}")
    elseif(_target STREQUAL "lv2")
      dpf__build_lv2("${NAME}" "${_dgl_library}" "${_dpf_plugin_MONOLITHIC}")
    elseif(_target STREQUAL "vst")
      dpf__build_vst("${NAME}" "${_dgl_library}")
    else()
      message(FATAL_ERROR "Unrecognized target type for plugin: ${_target}")
    endif()
  endforeach()
endfunction()

# ------------------------------------------------------------------------------
# DPF private functions (prefixed with `dpf__`)
# ------------------------------------------------------------------------------

# Note: The $<0:> trick is to prevent MSVC from appending the build type
#       to the output directory.
#

# dpf__build_jack
# ------------------------------------------------------------------------------
#
# Add build rules for a JACK program.
#
function(dpf__build_jack NAME DGL_LIBRARY)
  find_package(PkgConfig)
  pkg_check_modules(JACK "jack")
  if(NOT JACK_FOUND)
    dpf__warn_once_only(missing_jack
      "JACK is not found, skipping the `jack` plugin targets")
    return()
  endif()

  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_executable("${NAME}-jack" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-jack" "jack")
  dpf__add_ui_main("${NAME}-jack" "jack" "${DGL_LIBRARY}")
  target_link_libraries("${NAME}-jack" PRIVATE "${NAME}-dsp" "${NAME}-ui")
  set_target_properties("${NAME}-jack" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    OUTPUT_NAME "${NAME}")

  target_include_directories("${NAME}-jack" PRIVATE ${JACK_INCLUDE_DIRS})
  target_link_libraries("${NAME}-jack" PRIVATE ${JACK_LIBRARIES})
  link_directories(${JACK_LIBRARY_DIRS})
endfunction()

# dpf__build_ladspa
# ------------------------------------------------------------------------------
#
# Add build rules for a DSSI plugin.
#
function(dpf__build_ladspa NAME)
  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-ladspa" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-ladspa" "ladspa")
  target_link_libraries("${NAME}-ladspa" PRIVATE "${NAME}-dsp")
  set_target_properties("${NAME}-ladspa" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    OUTPUT_NAME "${NAME}-ladspa"
    PREFIX "")
endfunction()

# dpf__build_dssi
# ------------------------------------------------------------------------------
#
# Add build rules for a DSSI plugin.
#
function(dpf__build_dssi NAME DGL_LIBRARY)
  find_package(PkgConfig)
  pkg_check_modules(LIBLO "liblo")
  if(NOT LIBLO_FOUND)
    dpf__warn_once_only(missing_liblo
      "liblo is not found, skipping the `dssi` plugin targets")
    return()
  endif()

  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-dssi" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-dssi" "dssi")
  target_link_libraries("${NAME}-dssi" PRIVATE "${NAME}-dsp")
  set_target_properties("${NAME}-dssi" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    OUTPUT_NAME "${NAME}-dssi"
    PREFIX "")

  if(DGL_LIBRARY)
    dpf__add_executable("${NAME}-dssi-ui" ${_no_srcs})
    dpf__add_ui_main("${NAME}-dssi-ui" "dssi" "${DGL_LIBRARY}")
    target_link_libraries("${NAME}-dssi-ui" PRIVATE "${NAME}-ui")
    set_target_properties("${NAME}-dssi-ui" PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}-dssi/$<0:>"
      OUTPUT_NAME "${NAME}_ui")

    target_include_directories("${NAME}-dssi-ui" PRIVATE ${LIBLO_INCLUDE_DIRS})
    target_link_libraries("${NAME}-dssi-ui" PRIVATE ${LIBLO_LIBRARIES})
    link_directories(${LIBLO_LIBRARY_DIRS})
  endif()
endfunction()

# dpf__build_lv2
# ------------------------------------------------------------------------------
#
# Add build rules for a LV2 plugin.
#
function(dpf__build_lv2 NAME DGL_LIBRARY MONOLITHIC)
  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-lv2" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-lv2" "lv2")
  target_link_libraries("${NAME}-lv2" PRIVATE "${NAME}-dsp")
  set_target_properties("${NAME}-lv2" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.lv2/$<0:>"
    OUTPUT_NAME "${NAME}_dsp"
    PREFIX "")

  if(DGL_LIBRARY)
    if(MONOLITHIC)
      dpf__add_ui_main("${NAME}-lv2" "lv2" "${DGL_LIBRARY}")
      target_link_libraries("${NAME}-lv2" PRIVATE "${NAME}-ui")
      set_target_properties("${NAME}-lv2" PROPERTIES
        OUTPUT_NAME "${NAME}")
    else()
      dpf__add_module("${NAME}-lv2-ui" ${_no_srcs})
      dpf__add_ui_main("${NAME}-lv2-ui" "lv2" "${DGL_LIBRARY}")
      target_link_libraries("${NAME}-lv2-ui" PRIVATE "${NAME}-ui")
      set_target_properties("${NAME}-lv2-ui" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.lv2/$<0:>"
        OUTPUT_NAME "${NAME}_ui"
        PREFIX "")
    endif()
  endif()

  dpf__add_lv2_ttl_generator()
  add_dependencies("${NAME}-lv2" lv2_ttl_generator)

  add_custom_command(TARGET "${NAME}-lv2" POST_BUILD
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
    "$<TARGET_FILE:lv2_ttl_generator>"
    "$<TARGET_FILE:${NAME}-lv2>"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.lv2")
endfunction()

# dpf__build_vst
# ------------------------------------------------------------------------------
#
# Add build rules for a VST plugin.
#
function(dpf__build_vst NAME DGL_LIBRARY)
  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-vst" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-vst" "vst")
  dpf__add_ui_main("${NAME}-vst" "vst" "${DGL_LIBRARY}")
  target_link_libraries("${NAME}-vst" PRIVATE "${NAME}-dsp" "${NAME}-ui")
  set_target_properties("${NAME}-vst" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    OUTPUT_NAME "${NAME}-vst"
    PREFIX "")
endfunction()

# dpf__add_dgl_cairo
# ------------------------------------------------------------------------------
#
# Add the Cairo variant of DGL, if not already available.
#
function(dpf__add_dgl_cairo)
  if(TARGET dgl-cairo)
    return()
  endif()

  find_package(PkgConfig)
  pkg_check_modules(CAIRO "cairo" REQUIRED)

  dpf__add_static_library(dgl-cairo STATIC
    "${DPF_ROOT_DIR}/dgl/src/Application.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Color.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Geometry.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ImageBase.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Resources.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Widget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Cairo.cpp"
    "${DPF_ROOT_DIR}/dgl/src/WidgetPrivateData.cpp")
  if(NOT APPLE)
    target_sources(dgl-cairo PRIVATE
      "${DPF_ROOT_DIR}/dgl/src/Window.cpp")
  else()
    target_sources(dgl-cairo PRIVATE
      "${DPF_ROOT_DIR}/dgl/src/Window.mm")
  endif()
  target_include_directories(dgl-cairo PUBLIC
    "${DPF_ROOT_DIR}/dgl")

  dpf__add_dgl_system_libs()
  target_link_libraries(dgl-cairo PRIVATE dgl-system-libs)

  add_library(dgl-cairo-definitions INTERFACE)
  target_compile_definitions(dgl-cairo-definitions INTERFACE "DGL_CAIRO" "HAVE_CAIRO")

  target_include_directories(dgl-cairo PUBLIC ${CAIRO_INCLUDE_DIRS})
  target_link_libraries(dgl-cairo PRIVATE dgl-cairo-definitions ${CAIRO_LIBRARIES})
  link_directories(${CAIRO_LIBRARY_DIRS})
endfunction()

# dpf__add_dgl_opengl
# ------------------------------------------------------------------------------
#
# Add the OpenGL variant of DGL, if not already available.
#
function(dpf__add_dgl_opengl)
  if(TARGET dgl-opengl)
    return()
  endif()

  if(NOT OpenGL_GL_PREFERENCE)
    set(OpenGL_GL_PREFERENCE "LEGACY")
  endif()

  find_package(OpenGL REQUIRED)

  dpf__add_static_library(dgl-opengl STATIC
    "${DPF_ROOT_DIR}/dgl/src/Application.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Color.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Geometry.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ImageBase.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Resources.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Widget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/OpenGL.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Image.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ImageWidgets.cpp"
    "${DPF_ROOT_DIR}/dgl/src/NanoVG.cpp"
    "${DPF_ROOT_DIR}/dgl/src/WidgetPrivateData.cpp")
  if(NOT APPLE)
    target_sources(dgl-opengl PRIVATE
      "${DPF_ROOT_DIR}/dgl/src/Window.cpp")
  else()
    target_sources(dgl-opengl PRIVATE
      "${DPF_ROOT_DIR}/dgl/src/Window.mm")
  endif()
  target_include_directories(dgl-opengl PUBLIC
    "${DPF_ROOT_DIR}/dgl")

  dpf__add_dgl_system_libs()
  target_link_libraries(dgl-opengl PRIVATE dgl-system-libs)

  add_library(dgl-opengl-definitions INTERFACE)
  target_compile_definitions(dgl-opengl-definitions INTERFACE "DGL_OPENGL" "HAVE_OPENGL")

  target_include_directories(dgl-opengl PUBLIC "${OPENGL_INCLUDE_DIR}")
  target_link_libraries(dgl-opengl PRIVATE dgl-opengl-definitions "${OPENGL_gl_LIBRARY}")
endfunction()

# dpf__add_dgl_system_libs
# ------------------------------------------------------------------------------
#
# Find system libraries required by DGL and add them as an interface target.
#
function(dpf__add_dgl_system_libs)
  if(TARGET dgl-system-libs)
    return()
  endif()
  add_library(dgl-system-libs INTERFACE)
  if(HAIKU)
    target_link_libraries(dgl-system-libs INTERFACE "be")
  elseif(WIN32)
    target_link_libraries(dgl-system-libs INTERFACE "gdi32" "comdlg32")
  elseif(APPLE)
    find_library(APPLE_COCOA_FRAMEWORK "Cocoa")
    target_link_libraries(dgl-system-libs INTERFACE "${APPLE_COCOA_FRAMEWORK}")
  else()
    find_package(X11 REQUIRED)
    target_include_directories(dgl-system-libs INTERFACE "${X11_INCLUDE_DIR}")
    target_link_libraries(dgl-system-libs INTERFACE "${X11_X11_LIB}")
  endif()
endfunction()

# dpf__add_executable
# ------------------------------------------------------------------------------
#
# Adds an executable target, and set some default properties on the target.
#
function(dpf__add_executable NAME)
  add_executable("${NAME}" ${ARGN})
  set_target_properties("${NAME}" PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN TRUE)
endfunction()

# dpf__add_module
# ------------------------------------------------------------------------------
#
# Adds a module target, and set some default properties on the target.
#
function(dpf__add_module NAME)
  add_library("${NAME}" MODULE ${ARGN})
  set_target_properties("${NAME}" PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN TRUE)
  if ((NOT WIN32 AND NOT APPLE) OR MINGW)
    target_link_libraries("${NAME}" PRIVATE "-Wl,--no-undefined")
  endif()
endfunction()

# dpf__add_static_library
# ------------------------------------------------------------------------------
#
# Adds a static library target, and set some default properties on the target.
#
function(dpf__add_static_library NAME)
  add_library("${NAME}" STATIC ${ARGN})
  set_target_properties("${NAME}" PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN TRUE)
endfunction()

# dpf__add_plugin_main
# ------------------------------------------------------------------------------
#
# Adds plugin code to the given target.
#
function(dpf__add_plugin_main NAME TARGET)
  target_sources("${NAME}" PRIVATE
    "${DPF_ROOT_DIR}/distrho/DistrhoPluginMain.cpp")
  dpf__add_plugin_target_definition("${NAME}" "${TARGET}")
endfunction()

# dpf__add_ui_main
# ------------------------------------------------------------------------------
#
# Adds UI code to the given target (only if the target has UI).
#
function(dpf__add_ui_main NAME TARGET HAS_UI)
  if(HAS_UI)
    target_sources("${NAME}" PRIVATE
      "${DPF_ROOT_DIR}/distrho/DistrhoUIMain.cpp")
    dpf__add_plugin_target_definition("${NAME}" "${TARGET}")
  endif()
endfunction()

# dpf__add_plugin_target_definition
# ------------------------------------------------------------------------------
#
# Adds the plugins target macro definition.
# This selects which entry file is compiled according to the target type.
#
function(dpf__add_plugin_target_definition NAME TARGET)
  string(TOUPPER "${TARGET}" _upperTarget)
  target_compile_definitions("${NAME}" PRIVATE "DISTRHO_PLUGIN_TARGET_${_upperTarget}")
endfunction()

# dpf__add_lv2_ttl_generator
# ------------------------------------------------------------------------------
#
# Build the LV2 TTL generator.
#
function(dpf__add_lv2_ttl_generator)
  if(TARGET lv2_ttl_generator)
    return()
  endif()
  add_executable(lv2_ttl_generator "${DPF_ROOT_DIR}/utils/lv2-ttl-generator/lv2_ttl_generator.c")
  if(NOT WINDOWS AND NOT APPLE AND NOT HAIKU)
    target_link_libraries(lv2_ttl_generator "dl")
  endif()
endfunction()

# dpf__ensure_sources_non_empty
# ------------------------------------------------------------------------------
#
# Ensure the given source list contains at least one file.
# The function appends an empty source file to the list if necessary.
# This is useful when CMake does not permit to add targets without sources.
#
function(dpf__ensure_sources_non_empty VAR)
  if(NOT "" STREQUAL "${${VAR}}")
    return()
  endif()
  set(_file "${CMAKE_CURRENT_BINARY_DIR}/_dpf_empty.c")
  if(NOT EXISTS "${_file}")
    file(WRITE "${_file}" "")
  endif()
  set("${VAR}" "${_file}" PARENT_SCOPE)
endfunction()

# dpf__create_dummy_source_list
# ------------------------------------------------------------------------------
#
# Create a dummy source list which is equivalent to compiling nothing.
# This is only for compatibility with older CMake versions, which refuse to add
# targets without any sources.
#
macro(dpf__create_dummy_source_list VAR)
  set("${VAR}")
  if(CMAKE_VERSION VERSION_LESS "3.11")
    dpf__ensure_sources_non_empty("${VAR}")
  endif()
endmacro()

# dpf__warn_once
# ------------------------------------------------------------------------------
#
# Prints a warning message once only.
#
function(dpf__warn_once_only TOKEN MESSAGE)
  get_property(_warned GLOBAL PROPERTY "dpf__have_warned_${TOKEN}")
  if(NOT _warned)
    set_property(GLOBAL PROPERTY "dpf__have_warned_${TOKEN}" TRUE)
    message(WARNING "${MESSAGE}")
  endif()
endfunction()
