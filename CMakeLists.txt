cmake_minimum_required (VERSION 2.8)
project(DPF)

##########INCLUDES############
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake) #Set CMake module path
include(DPFFunctions) ## for Function use documentation look into DPFFuncitons or in the Examples.
include(version)
include(cxx_handling)

##########SETTINGS############
option(DPF_BUILD_EXAMPLES "Build Examples" OFF)
option(DPF_BUILD_DOC "Build Documentation" OFF)
option(DPF_INSTALL "Install DPF and Examples, if Build" OFF)
set_version(1 0 0)
use_cxx(11)


############BUILD##############
add_subdirectory(dgl)
add_library(DPF INTERFACE)
target_include_directories(DPF INTERFACE ${CMAKE_CURRENT_LIST_DIR}/distrho)
target_sources(DPF INTERFACE ${CMAKE_CURRENT_LIST_DIR}/distrho/DistrhoPluginMain.cpp)
target_compile_definitions(DPF INTERFACE HAVE_CPP11_SUPPORT)
add_library(DPF_UI INTERFACE)
target_include_directories(DPF_UI INTERFACE ${CMAKE_CURRENT_LIST_DIR}/distrho)
target_sources(DPF_UI INTERFACE ${CMAKE_CURRENT_LIST_DIR}/distrho/DistrhoUIMain.cpp)

#Helper tool for lv2
add_executable(lv2_ttl_generator utils/lv2-ttl-generator/lv2_ttl_generator.c)
target_compile_definitions(lv2_ttl_generator PUBLIC FORCE_UNIX_STYLE_PATH)
target_link_libraries(lv2_ttl_generator PUBLIC  ${CMAKE_DL_LIBS})



#########################Set Supported Format Variables###########################
find_package(LibLo)
find_package(Jack)
set(VST2_SUPPORT ON CACHE INTERNAL "IS VST2 SUPPORTED?")
set(LV2_SUPPORT ON CACHE INTERNAL "IS LV2 SUPPORTED?")
set(LADSPA_SUPPORT ON CACHE INTERNAL "IS LADSPA SUPPORTED?")
if(LIBLO_FOUND)
    set(DSSI_SUPPORT ON CACHE INTERNAL "IS DSSI SUPPORTED?")
endif(LIBLO_FOUND)
if(JACK_FOUND)
    set(JACK_SUPPORT ON CACHE INTERNAL "IS JACK SUPPORTED?")
endif(JACK_FOUND)


###################DOC###################
if(DPF_BUILD_DOC)
cmake_policy(SET CMP0054 NEW)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME "DISTRHO Plugin Framework")
    set(DOXYGEN_RECURSIVE NO)
    set(DOXYGEN_PREDEFINED DOXYGEN
        HAVE_CAIRO=1
        HAVE_OPENGL=1
        DISTRHO_PLUGIN_NAME=\"Plugin Name\"
        DISTRHO_PLUGIN_NUM_INPUTS=2
        DISTRHO_PLUGIN_NUM_OUTPUTS=2
        DISTRHO_PLUGIN_URI=\"urn:distrho:name\"
        DISTRHO_PLUGIN_HAS_UI=1
        DISTRHO_PLUGIN_HAS_EMBED_UI=1
        DISTRHO_PLUGIN_HAS_EXTERNAL_UI=1
        DISTRHO_PLUGIN_IS_RT_SAFE=1
        DISTRHO_PLUGIN_IS_SYNTH=1
        DISTRHO_PLUGIN_WANT_DIRECT_ACCESS=1
        DISTRHO_PLUGIN_WANT_LATENCY=1
        DISTRHO_PLUGIN_WANT_MIDI_INPUT=1
        DISTRHO_PLUGIN_WANT_MIDI_OUTPUT=1
        DISTRHO_PLUGIN_WANT_PROGRAMS=1
        DISTRHO_PLUGIN_WANT_STATE=1
        DISTRHO_PLUGIN_WANT_TIMEPOS=1
        DISTRHO_PLUGIN_WANT_FULL_STATE=1
        DISTRHO_UI_USE_NANOVG=1)

    doxygen_add_docs(dpfdoc
        ${CMAKE_CURRENT_LIST_DIR}/distrho ${CMAKE_CURRENT_LIST_DIR}/distrho/extra
        ${CMAKE_CURRENT_LIST_DIR}/dgl
        ALL
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Build DPF-Doc")

endif(DOXYGEN_FOUND)
endif(DPF_BUILD_DOC)
################EXAMPLES####################
if(DPF_BUILD_EXAMPLES)
    add_subdirectory(examples/Latency)
    add_subdirectory(examples/CairoUI)
    add_subdirectory(examples/ExternalUI)
    add_subdirectory(examples/Info)
    add_subdirectory(examples/Meters)
    add_subdirectory(examples/MidiThrough)
    add_subdirectory(examples/Parameters)
    add_subdirectory(examples/States)
endif(DPF_BUILD_EXAMPLES)

if(DPF_INSTALL)
#    if(DPF_BUILD_DOC)
#        install()
#    endif(DPF_BUILD_DOC)
endif(DPF_INSTALL)

############STUFF FOR INSOURCE BUILD##########

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
