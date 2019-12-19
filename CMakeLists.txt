cmake_minimum_required (VERSION 2.8)
project(DPF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_CXX_STANDARD 14)

include(DPFFunctions) ## for Function use documentation look into DPFFuncitons or in the Examples.
##########SETTINGS############
option(DPF_BUILD_EXAMPLES "Build Examples" ON)
option(DPF_INSTALL "Build Examples" ON)

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


####################################################
#if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
#    target_compile_definitions(DPF INTERFACE PLATFORM_WINDOWS)
#     target_compile_definitions(lv2_ttl_generator PUBLIC PLATFORM_WINDOWS)
#endif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

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

endif(DPF_INSTALL)
