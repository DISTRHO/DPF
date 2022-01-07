# DISTRHO Plugin Framework (DPF)
# Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
#
# SPDX-License-Identifier: ISC

# ------------------------------------------------------------------------------
# CMake support module for the DISTRHO Plugin Framework
#
# The purpose of this module is to help building music plugins easily, when the
# project uses CMake as its build system.
#
# In order to use the helpers provided by this module, a plugin author should
# add DPF as a subproject, making the function `dpf_add_plugin` available.
# The usage of this function is documented below in greater detail.
#
# Example project `CMakeLists.txt`:
#
# ```
# cmake_minimum_required(VERSION 3.7)
# project(MyPlugin)
#
# add_subdirectory(DPF)
#
# dpf_add_plugin(MyPlugin
#   TARGETS lv2 vst2 vst3
#   UI_TYPE opengl
#   FILES_DSP
#       src/MyPlugin.cpp
#   FILES_UI
#       src/MyUI.cpp)
#
# target_include_directories(MyPlugin
#   PUBLIC src)
# ```
#
# Important: note that properties, such as include directories, definitions,
# and linked libraries *must* be marked with `PUBLIC` so they take effect and
# propagate into all the plugin targets.

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
#       `jack`, `ladspa`, `dssi`, `lv2`, `vst2`, `vst3`
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
  set(multiValueArgs TARGETS FILES_DSP FILES_UI FILES_COMMON)
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

  if((NOT WIN32) AND (NOT APPLE) AND (NOT HAIKU))
    target_link_libraries("${NAME}" PRIVATE "dl")
  endif()

  if(_dgl_library)
    # make sure that all code will see DGL_* definitions
    target_link_libraries("${NAME}" PUBLIC
      "${_dgl_library}-definitions"
      dgl-system-libs-definitions)
  endif()

  dpf__add_static_library("${NAME}-dsp" ${_dpf_plugin_FILES_DSP})
  target_link_libraries("${NAME}-dsp" PUBLIC "${NAME}")

  if(_dgl_library)
    dpf__add_static_library("${NAME}-ui" ${_dpf_plugin_FILES_UI})
    target_link_libraries("${NAME}-ui" PUBLIC "${NAME}" ${_dgl_library})
    if((NOT WIN32) AND (NOT APPLE) AND (NOT HAIKU))
      target_link_libraries("${NAME}-ui" PRIVATE "dl")
    endif()
    # add the files containing Objective-C classes, recompiled under namespace
    dpf__add_plugin_specific_ui_sources("${NAME}-ui")
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
    elseif(_target STREQUAL "vst2")
      dpf__build_vst2("${NAME}" "${_dgl_library}")
    elseif(_target STREQUAL "vst3")
      dpf__build_vst3("${NAME}" "${_dgl_library}")
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
  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_executable("${NAME}-jack" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-jack" "jack")
  dpf__add_ui_main("${NAME}-jack" "jack" "${DGL_LIBRARY}")
  target_link_libraries("${NAME}-jack" PRIVATE "${NAME}-dsp" "${NAME}-ui")
  set_target_properties("${NAME}-jack" PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    OUTPUT_NAME "${NAME}")

  # for RtAudio native fallback
  if(APPLE)
    find_library(APPLE_COREAUDIO_FRAMEWORK "CoreAudio")
    find_library(APPLE_COREFOUNDATION_FRAMEWORK "CoreFoundation")
    target_link_libraries("${NAME}-jack" PRIVATE "${APPLE_COREAUDIO_FRAMEWORK}" "${APPLE_COREFOUNDATION_FRAMEWORK}")
  endif()
endfunction()

# dpf__build_ladspa
# ------------------------------------------------------------------------------
#
# Add build rules for a LADSPA plugin.
#
function(dpf__build_ladspa NAME)
  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-ladspa" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-ladspa" "ladspa")
  dpf__set_module_export_list("${NAME}-ladspa" "ladspa")
  target_link_libraries("${NAME}-ladspa" PRIVATE "${NAME}-dsp")
  set_target_properties("${NAME}-ladspa" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/obj/ladspa/$<0:>"
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

  link_directories(${LIBLO_LIBRARY_DIRS})

  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-dssi" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-dssi" "dssi")
  dpf__set_module_export_list("${NAME}-dssi" "dssi")
  target_link_libraries("${NAME}-dssi" PRIVATE "${NAME}-dsp")
  set_target_properties("${NAME}-dssi" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/obj/dssi/$<0:>"
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
  endif()
endfunction()

# dpf__build_lv2
# ------------------------------------------------------------------------------
#
# Add build rules for an LV2 plugin.
#
function(dpf__build_lv2 NAME DGL_LIBRARY MONOLITHIC)
  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-lv2" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-lv2" "lv2")
  if(DGL_LIBRARY AND MONOLITHIC)
    dpf__set_module_export_list("${NAME}-lv2" "lv2")
  else()
    dpf__set_module_export_list("${NAME}-lv2" "lv2-dsp")
  endif()
  target_link_libraries("${NAME}-lv2" PRIVATE "${NAME}-dsp")
  set_target_properties("${NAME}-lv2" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.lv2/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/obj/lv2/$<0:>"
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
      dpf__set_module_export_list("${NAME}-lv2-ui" "lv2-ui")
      target_link_libraries("${NAME}-lv2-ui" PRIVATE "${NAME}-ui")
      set_target_properties("${NAME}-lv2-ui" PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.lv2/$<0:>"
        ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/obj/lv2/$<0:>"
        OUTPUT_NAME "${NAME}_ui"
        PREFIX "")
    endif()
  endif()

  dpf__add_lv2_ttl_generator()
  add_dependencies("${NAME}-lv2" lv2_ttl_generator)

  add_custom_command(TARGET "${NAME}-lv2" POST_BUILD
    COMMAND
    "$<TARGET_FILE:lv2_ttl_generator>"
    "$<TARGET_FILE:${NAME}-lv2>"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.lv2"
    DEPENDS lv2_ttl_generator)
endfunction()

# dpf__build_vst2
# ------------------------------------------------------------------------------
#
# Add build rules for a VST2 plugin.
#
function(dpf__build_vst2 NAME DGL_LIBRARY)
  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-vst2" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-vst2" "vst2")
  dpf__add_ui_main("${NAME}-vst2" "vst2" "${DGL_LIBRARY}")
  dpf__set_module_export_list("${NAME}-vst2" "vst2")
  target_link_libraries("${NAME}-vst2" PRIVATE "${NAME}-dsp" "${NAME}-ui")
  set_target_properties("${NAME}-vst2" PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/obj/vst2/$<0:>"
    OUTPUT_NAME "${NAME}-vst2"
    PREFIX "")
  if(APPLE)
    set_target_properties("${NAME}-vst2" PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.vst/Contents/MacOS/$<0:>"
      OUTPUT_NAME "${NAME}"
      SUFFIX "")
    set(INFO_PLIST_PROJECT_NAME "${NAME}")
    configure_file("${DPF_ROOT_DIR}/utils/plugin.vst/Contents/Info.plist"
      "${PROJECT_BINARY_DIR}/bin/${NAME}.vst/Contents/Info.plist" @ONLY)
    file(COPY "${DPF_ROOT_DIR}/utils/plugin.vst/Contents/PkgInfo"
      DESTINATION "${PROJECT_BINARY_DIR}/bin/${NAME}.vst/Contents")
  endif()
endfunction()

# dpf__determine_vst3_package_architecture
# ------------------------------------------------------------------------------
#
# Determines the package architecture for a VST3 plugin target.
#
function(dpf__determine_vst3_package_architecture OUTPUT_VARIABLE)
  # if set by variable, override the detection
  if(DPF_VST3_ARCHITECTURE)
    set("${OUTPUT_VARIABLE}" "${DPF_VST3_ARCHITECTURE}" PARENT_SCOPE)
    return()
  endif()

  # not used on Apple, which supports universal binary
  if(APPLE)
    set("${OUTPUT_VARIABLE}" "universal" PARENT_SCOPE)
    return()
  endif()

  # identify the target processor (special case of MSVC, problematic sometimes)
  if(MSVC)
    set(vst3_system_arch "${MSVC_CXX_ARCHITECTURE_ID}")
  else()
    set(vst3_system_arch "${CMAKE_SYSTEM_PROCESSOR}")
  endif()

  # transform the processor name to a format that VST3 recognizes
  if(vst3_system_arch MATCHES "^(x86_64|amd64|AMD64|x64|X64)$")
    set(vst3_package_arch "x86_64")
  elseif(vst3_system_arch MATCHES "^(i.86|x86|X86)$")
    if(WIN32)
      set(vst3_package_arch "x86")
    else()
      set(vst3_package_arch "i386")
    endif()
  elseif(vst3_system_arch MATCHES "^(armv[3-8][a-z]*)$")
    set(vst3_package_arch "${vst3_system_arch}")
  elseif(vst3_system_arch MATCHES "^(aarch64)$")
    set(vst3_package_arch "aarch64")
  else()
    message(FATAL_ERROR "We don't know this architecture for VST3: ${vst3_system_arch}.")
  endif()

  # TODO: the detections for Windows arm/arm64 when supported

  set("${OUTPUT_VARIABLE}" "${vst3_package_arch}" PARENT_SCOPE)
endfunction()

# dpf__build_vst3
# ------------------------------------------------------------------------------
#
# Add build rules for a VST3 plugin.
#
function(dpf__build_vst3 NAME DGL_LIBRARY)
  dpf__determine_vst3_package_architecture(vst3_arch)

  dpf__create_dummy_source_list(_no_srcs)

  dpf__add_module("${NAME}-vst3" ${_no_srcs})
  dpf__add_plugin_main("${NAME}-vst3" "vst3")
  dpf__add_ui_main("${NAME}-vst3" "vst3" "${DGL_LIBRARY}")
  dpf__set_module_export_list("${NAME}-vst3" "vst3")
  target_link_libraries("${NAME}-vst3" PRIVATE "${NAME}-dsp" "${NAME}-ui")
  set_target_properties("${NAME}-vst3" PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/obj/vst3/$<0:>"
    OUTPUT_NAME "${NAME}"
    PREFIX "")

  if(APPLE)
    set_target_properties("${NAME}-vst3" PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.vst3/Contents/MacOS/$<0:>"
      SUFFIX "")
  elseif(WIN32)
    set_target_properties("${NAME}-vst3" PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.vst3/Contents/${vst3_arch}-win/$<0:>" SUFFIX ".vst3")
  else()
    set_target_properties("${NAME}-vst3" PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/${NAME}.vst3/Contents/${vst3_arch}-linux/$<0:>")
  endif()

  if(APPLE)
    # Uses the same macOS bundle template as VST2
    set(INFO_PLIST_PROJECT_NAME "${NAME}")
    configure_file("${DPF_ROOT_DIR}/utils/plugin.vst/Contents/Info.plist"
     "${PROJECT_BINARY_DIR}/bin/${NAME}.vst3/Contents/Info.plist" @ONLY)
    file(COPY "${DPF_ROOT_DIR}/utils/plugin.vst/Contents/PkgInfo"
     DESTINATION "${PROJECT_BINARY_DIR}/bin/${NAME}.vst3/Contents")
  endif()
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

  link_directories(${CAIRO_LIBRARY_DIRS})

  dpf__add_static_library(dgl-cairo STATIC
    "${DPF_ROOT_DIR}/dgl/src/Application.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ApplicationPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Color.cpp"
    "${DPF_ROOT_DIR}/dgl/src/EventHandlers.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Geometry.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ImageBase.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ImageBaseWidgets.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Resources.cpp"
    "${DPF_ROOT_DIR}/dgl/src/SubWidget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/SubWidgetPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/TopLevelWidget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/TopLevelWidgetPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Widget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/WidgetPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Window.cpp"
    "${DPF_ROOT_DIR}/dgl/src/WindowPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Cairo.cpp")
  if(NOT APPLE)
    target_sources(dgl-cairo PRIVATE
      "${DPF_ROOT_DIR}/dgl/src/pugl.cpp")
  else() # Note: macOS pugl will be built as part of DistrhoUI_macOS.mm
    #target_sources(dgl-opengl PRIVATE
    #  "${DPF_ROOT_DIR}/dgl/src/pugl.mm")
  endif()
  target_include_directories(dgl-cairo PUBLIC
    "${DPF_ROOT_DIR}/dgl")
  target_include_directories(dgl-cairo PUBLIC
    "${DPF_ROOT_DIR}/dgl/src/pugl-upstream/include")

  dpf__add_dgl_system_libs()
  target_link_libraries(dgl-cairo PRIVATE dgl-system-libs)

  add_library(dgl-cairo-definitions INTERFACE)
  target_compile_definitions(dgl-cairo-definitions INTERFACE "DGL_CAIRO" "HAVE_CAIRO" "HAVE_DGL")

  target_include_directories(dgl-cairo PUBLIC ${CAIRO_INCLUDE_DIRS})
  if(MINGW)
    target_link_libraries(dgl-cairo PRIVATE ${CAIRO_STATIC_LIBRARIES})
  else()
    target_link_libraries(dgl-cairo PRIVATE ${CAIRO_LIBRARIES})
  endif()
  target_link_libraries(dgl-cairo PRIVATE dgl-cairo-definitions)
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
    "${DPF_ROOT_DIR}/dgl/src/ApplicationPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Color.cpp"
    "${DPF_ROOT_DIR}/dgl/src/EventHandlers.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Geometry.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ImageBase.cpp"
    "${DPF_ROOT_DIR}/dgl/src/ImageBaseWidgets.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Resources.cpp"
    "${DPF_ROOT_DIR}/dgl/src/SubWidget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/SubWidgetPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/TopLevelWidget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/TopLevelWidgetPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Widget.cpp"
    "${DPF_ROOT_DIR}/dgl/src/WidgetPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/Window.cpp"
    "${DPF_ROOT_DIR}/dgl/src/WindowPrivateData.cpp"
    "${DPF_ROOT_DIR}/dgl/src/OpenGL.cpp"
    "${DPF_ROOT_DIR}/dgl/src/NanoVG.cpp")
  if(NOT APPLE)
    target_sources(dgl-opengl PRIVATE
      "${DPF_ROOT_DIR}/dgl/src/pugl.cpp")
  else() # Note: macOS pugl will be built as part of DistrhoUI_macOS.mm
    #target_sources(dgl-opengl PRIVATE
    #  "${DPF_ROOT_DIR}/dgl/src/pugl.mm")
  endif()
  target_include_directories(dgl-opengl PUBLIC
    "${DPF_ROOT_DIR}/dgl")
  target_include_directories(dgl-opengl PUBLIC
    "${DPF_ROOT_DIR}/dgl/src/pugl-upstream/include")

  if(APPLE)
    target_compile_definitions(dgl-opengl PUBLIC "GL_SILENCE_DEPRECATION")
  endif()

  dpf__add_dgl_system_libs()
  target_link_libraries(dgl-opengl PRIVATE dgl-system-libs)

  add_library(dgl-opengl-definitions INTERFACE)
  target_compile_definitions(dgl-opengl-definitions INTERFACE "DGL_OPENGL" "HAVE_OPENGL" "HAVE_DGL")

  target_include_directories(dgl-opengl PUBLIC "${OPENGL_INCLUDE_DIR}")
  target_link_libraries(dgl-opengl PRIVATE dgl-opengl-definitions "${OPENGL_gl_LIBRARY}")
  
  if(USE_NANOVG_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(dgl-opengl PUBLIC "${FREETYPE_INCLUDE_DIRS}")
    target_link_libraries(dgl-opengl PUBLIC "${FREETYPE_LIBRARIES}")
    add_definitions(-DFONS_USE_FREETYPE)
  endif()
  
endfunction()

# dpf__add_plugin_specific_ui_sources
# ------------------------------------------------------------------------------
#
# Compile plugin-specific UI sources into the target designated by the given
# name. There are some special considerations here:
# - On most platforms, sources can be compiled only once, as part of DGL;
# - On macOS, for any sources which define Objective-C interfaces, these must
#   be recompiled for each plugin under a unique namespace. In this case, the
#   name must be a plugin-specific identifier, and it will be used for computing
#   the unique ID along with the project version.
function(dpf__add_plugin_specific_ui_sources NAME)
  if(APPLE)
    target_sources("${NAME}" PRIVATE
      "${DPF_ROOT_DIR}/distrho/DistrhoUI_macOS.mm")
    string(SHA256 _hash "${NAME}:${PROJECT_VERSION}")
    target_compile_definitions("${NAME}" PUBLIC "PUGL_NAMESPACE=${_hash}")
  endif()
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
  add_library(dgl-system-libs-definitions INTERFACE)
  if(HAIKU)
    target_link_libraries(dgl-system-libs INTERFACE "be")
  elseif(WIN32)
    target_link_libraries(dgl-system-libs INTERFACE "gdi32" "comdlg32")
  elseif(APPLE)
    find_library(APPLE_COCOA_FRAMEWORK "Cocoa")
    find_library(APPLE_COREVIDEO_FRAMEWORK "CoreVideo")
    target_link_libraries(dgl-system-libs INTERFACE "${APPLE_COCOA_FRAMEWORK}" "${APPLE_COREVIDEO_FRAMEWORK}")
  else()
    find_package(X11 REQUIRED)
    target_include_directories(dgl-system-libs INTERFACE "${X11_INCLUDE_DIR}")
    target_link_libraries(dgl-system-libs INTERFACE "${X11_X11_LIB}")
    target_compile_definitions(dgl-system-libs-definitions INTERFACE "HAVE_X11")
    if(X11_Xext_FOUND)
      target_link_libraries(dgl-system-libs INTERFACE "${X11_Xext_LIB}")
      target_compile_definitions(dgl-system-libs-definitions INTERFACE "HAVE_XEXT")
    endif()
    if(X11_XSync_FOUND)
      target_link_libraries(dgl-system-libs INTERFACE "${X11_XSync_LIB}")
      target_compile_definitions(dgl-system-libs-definitions INTERFACE "HAVE_XSYNC")
    endif()
    if(X11_Xrandr_FOUND)
      target_link_libraries(dgl-system-libs INTERFACE "${X11_Xrandr_LIB}")
      target_compile_definitions(dgl-system-libs-definitions INTERFACE "HAVE_XRANDR")
    endif()
    #if(X11_Xcursor_FOUND)
    #  target_link_libraries(dgl-system-libs INTERFACE "${X11_Xcursor_LIB}")
    #  target_compile_definitions(dgl-system-libs-definitions INTERFACE "HAVE_XCURSOR")
    #endif()
   endif()

   if(MSVC)
     file(MAKE_DIRECTORY "${DPF_ROOT_DIR}/khronos/GL")
     foreach(_gl_header "glext.h")
       if(NOT EXISTS "${DPF_ROOT_DIR}/khronos/GL/${_gl_header}")
         file(DOWNLOAD "https://www.khronos.org/registry/OpenGL/api/GL/${_gl_header}" "${DPF_ROOT_DIR}/khronos/GL/${_gl_header}" SHOW_PROGRESS)
       endif()
     endforeach()
     foreach(_khr_header "khrplatform.h")
       if(NOT EXISTS "${DPF_ROOT_DIR}/khronos/KHR/${_khr_header}")
         file(DOWNLOAD "https://www.khronos.org/registry/EGL/api/KHR/${_khr_header}" "${DPF_ROOT_DIR}/khronos/KHR/${_khr_header}" SHOW_PROGRESS)
       endif()
     endforeach()
     target_include_directories(dgl-system-libs-definitions INTERFACE "${DPF_ROOT_DIR}/khronos")
   endif()

  target_link_libraries(dgl-system-libs INTERFACE dgl-system-libs-definitions)
endfunction()

# dpf__add_executable
# ------------------------------------------------------------------------------
#
# Adds an executable target, and set some default properties on the target.
#
function(dpf__add_executable NAME)
  add_executable("${NAME}" ${ARGN})
  dpf__set_target_defaults("${NAME}")
  if(MINGW)
    target_link_libraries("${NAME}" PRIVATE "-static")
  endif()
endfunction()

# dpf__add_module
# ------------------------------------------------------------------------------
#
# Adds a module target, and set some default properties on the target.
#
function(dpf__add_module NAME)
  add_library("${NAME}" MODULE ${ARGN})
  dpf__set_target_defaults("${NAME}")
  if(MINGW)
    target_link_libraries("${NAME}" PRIVATE "-static")
  endif()
endfunction()

# dpf__add_static_library
# ------------------------------------------------------------------------------
#
# Adds a static library target, and set some default properties on the target.
#
function(dpf__add_static_library NAME)
  add_library("${NAME}" STATIC ${ARGN})
  dpf__set_target_defaults("${NAME}")
endfunction()

# dpf__set_module_export_list
# ------------------------------------------------------------------------------
#
# Applies a list of exported symbols to the module target.
#
function(dpf__set_module_export_list NAME EXPORTS)
  if(WIN32)
    target_sources("${NAME}" PRIVATE "${DPF_ROOT_DIR}/utils/symbols/${EXPORTS}.def")
  elseif(APPLE)
    set_property(TARGET "${NAME}" APPEND PROPERTY LINK_OPTIONS
      "-Xlinker" "-exported_symbols_list"
      "-Xlinker" "${DPF_ROOT_DIR}/utils/symbols/${EXPORTS}.exp")
  else()
    set_property(TARGET "${NAME}" APPEND PROPERTY LINK_OPTIONS
      "-Xlinker" "--version-script=${DPF_ROOT_DIR}/utils/symbols/${EXPORTS}.version")
  endif()
endfunction()

# dpf__set_target_defaults
# ------------------------------------------------------------------------------
#
# Set default properties which must apply to all DPF-defined targets.
#
function(dpf__set_target_defaults NAME)
  set_target_properties("${NAME}" PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    C_VISIBILITY_PRESET "hidden"
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN TRUE)
  if(WIN32)
    target_compile_definitions("${NAME}" PUBLIC "NOMINMAX")
  endif()
  if (MINGW)
    target_compile_options("${NAME}" PUBLIC "-mstackrealign")
  endif()
  if (MSVC)
    target_compile_options("${NAME}" PUBLIC "/UTF-8")
    target_compile_definitions("${NAME}" PUBLIC "_CRT_SECURE_NO_WARNINGS")
  endif()
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
  if((NOT WIN32) AND (NOT APPLE) AND (NOT HAIKU))
    target_link_libraries(lv2_ttl_generator PRIVATE "dl")
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
