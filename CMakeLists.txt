cmake_minimum_required (VERSION 2.8)
project(DPF)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
set(CMAKE_CXX_STANDARD 14)

include(DPFFunctions) ## for Function use documentation look into DPFFuncitons or in the Examples.
##########SETTINGS############
option(DPF_BUILD_EXAMPLES "Build Examples" ON)
option(DPF_BUILD_DOC "Build Documentation" ON)
option(DPF_INSTALL "Install DPF and Examples, if Build" ON)

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


########################FUNCTIONS############################

# Sets GET_LIBS_AND_DEFINES_LIBS and GET_LIBS_AND_DEFINES_DEFINES variables in parentscope. Link to this !
function(GET_LIBS_AND_DEFINES UI_BACKEND)
    if(NOT UI_BACKEND)
        set(UI_BACKEND NONE)
    endif(NOT UI_BACKEND)
    if(${UI_BACKEND} STREQUAL "NONE")
        set(GET_LIBS_AND_DEFINES_LIBS DPF PARENT_SCOPE)
    elseif(${UI_BACKEND} STREQUAL "OPENGL")
        set(GET_LIBS_AND_DEFINES_LIBS dgl-opengl DPF_UI PARENT_SCOPE)
    elseif(${UI_BACKEND} STREQUAL "CAIRO")
        set(GET_LIBS_AND_DEFINES_LIBS dgl-cairo DPF_UI PARENT_SCOPE)
    elseif(${UI_BACKEND} STREQUAL "EXTERNAL")
        set(GET_LIBS_AND_DEFINES_LIBS DPF_UI PARENT_SCOPE)
        set(GET_LIBS_AND_DEFINES_DEFINES DGL_EXTERNAL PARENT_SCOPE)
    else()
        message(ERROR "No vaild UI_BACKEND, choose NONE OPENGL CAIRO or EXTERNAL")
    endif(${UI_BACKEND} STREQUAL "NONE")
endfunction(GET_LIBS_AND_DEFINES UI_BACKEND)

# Parameter to use:
#           TARGET: Name for the generated VST
#           UI_BACKEND; weather it should be NONE,EXTERNAL,OPENGL or CAIRO
#           SOURCES:  Plugin Sources, to compile in the vst
#           INCLUDE_DIRECTORIES: Directories to include to the vst.
#           DEFINE: Define Preprocessor defines
function(BUILD_JACK)
    set(oneValueArgs TARGET UI_BACKEND)
    set(multiValueArgs SOURCES INCLUDE_DIRECTORIES DEFINES)
    cmake_parse_arguments(PLUGIN_PARAM "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    get_libs_and_defines(${PLUGIN_PARAM_UI_BACKEND} )
    add_executable(${PLUGIN_PARAM_TARGET}  ${PLUGIN_PARAM_SOURCES})
    set_target_properties(${PLUGIN_PARAM_TARGET} PROPERTIES PREFIX "")
    target_link_libraries(${PLUGIN_PARAM_TARGET} ${GET_LIBS_AND_DEFINES_LIBS} DPF ${JACK_LIBRARIES} )
    target_include_directories(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES})
    target_compile_definitions(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_JACK)
endfunction(BUILD_JACK)

# Parameter to use:
#           TARGET: Name for the generated VST
#           UI_BACKEND; weather it should be NONE,EXTERNAL,OPENGL or CAIRO
#           SOURCES:  Plugin Sources, to compile in the vst
#           INCLUDE_DIRECTORIES: Directories to include to the vst.
#           DEFINE: Define Preprocessor defines
function(BUILD_VST2)
    set(oneValueArgs TARGET UI_BACKEND)
    set(multiValueArgs SOURCES INCLUDE_DIRECTORIES DEFINES)
    cmake_parse_arguments(PLUGIN_PARAM "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    get_libs_and_defines(${PLUGIN_PARAM_UI_BACKEND} )
    add_library(${PLUGIN_PARAM_TARGET} MODULE  ${PLUGIN_PARAM_SOURCES})
    set_target_properties(${PLUGIN_PARAM_TARGET} PROPERTIES PREFIX "")
    target_link_libraries(${PLUGIN_PARAM_TARGET} ${GET_LIBS_AND_DEFINES_LIBS} DPF)
    target_include_directories(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES})
    target_compile_definitions(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_VST)
endfunction(BUILD_VST2)


# Parameter to use:
#           TARGET: Name for the generated VST
#           UI_BACKEND; weather it should be NONE,EXTERNAL,OPENGL or CAIRO because its an ladspa... set it to NONE or External...
#           SOURCES:  Plugin Sources, to compile in the vst
#           INCLUDE_DIRECTORIES: Directories to include to the vst.
#           DEFINE: Define Preprocessor defines
function(BUILD_LADSPA  )
    set(oneValueArgs TARGET UI_BACKEND)
    set(multiValueArgs SOURCES INCLUDE_DIRECTORIES DEFINES)
    cmake_parse_arguments(PLUGIN_PARAM "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    get_libs_and_defines(${PLUGIN_PARAM_UI_BACKEND} )

    add_library(${PLUGIN_PARAM_TARGET} MODULE  ${PLUGIN_PARAM_SOURCES})
    set_target_properties(${PLUGIN_PARAM_TARGET} PROPERTIES PREFIX "")
    target_link_libraries(${PLUGIN_PARAM_TARGET} ${GET_LIBS_AND_DEFINES_LIBS})
    target_include_directories(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES})
    target_compile_definitions(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_LADSPA)
endfunction(BUILD_LADSPA)

# Parameter to use:
#           TARGET: Name for the generated VST
#           UI_BACKEND: weather it should be NONE,EXTERNAL,OPENGL or CAIRO
#           UI_SOURCES: Plugin UI Sources
#           SOURCES:  Plugin Sources, to compile in the vst
#           INCLUDE_DIRECTORIES: Directories to include to the vst.
#           DEFINE: Define Preprocessor defines
# fills the Variable ${TARGET}_TTL_FILES with the generated files.
# if a ui is specified(with UI_SOURCES) 2 targets are build the TARGET and {TARGET}_ui
# if UI_SOURCES is not specified, a unified ui lv2 plugin will be build. This will also add DISTHRO_WANT_DIRECT_ACCESS=1 to the Project.
function(BUILD_LV2 )
    set(oneValueArgs TARGET UI_BACKEND)
    set(multiValueArgs SOURCES UI_SOURCES INCLUDE_DIRECTORIES DEFINES)
    cmake_parse_arguments(PLUGIN_PARAM "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    get_libs_and_defines(${PLUGIN_PARAM_UI_BACKEND})
    if(PLUGIN_PARAM_UI_SOURCES) # IF UI Files are present, build seperated UI Files.
        add_library(${PLUGIN_PARAM_TARGET}_ui MODULE  ${PLUGIN_PARAM_UI_SOURCES})
        set_target_properties(${PLUGIN_PARAM_TARGET}_ui PROPERTIES PREFIX "")
        target_link_libraries(${PLUGIN_PARAM_TARGET}_ui ${GET_LIBS_AND_DEFINES_LIBS} )
        target_include_directories(${PLUGIN_PARAM_TARGET}_ui PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES})
        target_compile_definitions(${PLUGIN_PARAM_TARGET}_ui PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_LV2)

        add_library(${PLUGIN_PARAM_TARGET} MODULE  ${PLUGIN_PARAM_SOURCES}  ${PLUGIN_PARAM_UI_SOURCES})
        set_target_properties(${PLUGIN_PARAM_TARGET} PROPERTIES PREFIX "")
        target_link_libraries(${PLUGIN_PARAM_TARGET} ${GET_LIBS_AND_DEFINES_LIBS} DPF DPF_UI)
        target_include_directories(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES})
        target_compile_definitions(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_LV2)
        set(${PLUGIN_PARAM_TARGET}_TTL_FILES
            $<TARGET_FILE_DIR:${PLUGIN_PARAM_TARGET}>/manifest.ttl
            $<TARGET_FILE_DIR:${PLUGIN_PARAM_TARGET}>/${PLUGIN_PARAM_TARGET}.ttl
            $<TARGET_FILE_DIR:${PLUGIN_PARAM_TARGET}>/${PLUGIN_PARAM_TARGET}_ui.ttl PARENT_SCOPE)
    else()
        add_library(${PLUGIN_PARAM_TARGET} MODULE  ${PLUGIN_PARAM_SOURCES})
        set_target_properties(${PLUGIN_PARAM_TARGET} PROPERTIES PREFIX "")
        target_link_libraries(${PLUGIN_PARAM_TARGET} ${GET_LIBS_AND_DEFINES_LIBS} DPF)
        target_include_directories(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES})
        target_compile_definitions(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_LV2 DISTRHO_PLUGIN_WANT_DIRECT_ACCESS=1 )
        set(${PLUGIN_PARAM_TARGET}_TTL_FILES
            $<TARGET_FILE_DIR:${PLUGIN_PARAM_TARGET}>/manifest.ttl
            $<TARGET_FILE_DIR:${PLUGIN_PARAM_TARGET}>/${PLUGIN_PARAM_TARGET}.ttl PARENT_SCOPE)
    endif(PLUGIN_PARAM_UI_SOURCES)
    add_custom_command(TARGET ${PLUGIN_PARAM_TARGET} POST_BUILD COMMAND $<TARGET_FILE:lv2_ttl_generator> ARGS $<TARGET_FILE:${PLUGIN_PARAM_TARGET}>  )
endfunction(BUILD_LV2)

function(BUILD_DSSI )
    set(oneValueArgs TARGET UI_BACKEND)
    set(multiValueArgs SOURCES  UI_SOURCES INCLUDE_DIRECTORIES DEFINES)
    cmake_parse_arguments(PLUGIN_PARAM "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    get_libs_and_defines(${PLUGIN_PARAM_UI_BACKEND} )

    if(PLUGIN_PARAM_UI_SOURCES)
        add_library(${PLUGIN_PARAM_TARGET}_ui MODULE  ${PLUGIN_PARAM_UI_SOURCES})
        set_target_properties(${PLUGIN_PARAM_TARGET}_ui PROPERTIES PREFIX "")
        target_link_libraries(${PLUGIN_PARAM_TARGET}_ui ${GET_LIBS_AND_DEFINES_LIBS} ${LIBLO_LIBRARIES})
        target_include_directories(${PLUGIN_PARAM_TARGET}_ui PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES} ${LIBLO_INCLUDE_DIRS})
        target_compile_definitions(${PLUGIN_PARAM_TARGET}_ui PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_DSSI)

        add_library(${PLUGIN_PARAM_TARGET} MODULE  ${PLUGIN_PARAM_SOURCES}  ${PLUGIN_PARAM_UI_SOURCES})
        set_target_properties(${PLUGIN_PARAM_TARGET} PROPERTIES PREFIX "")
        target_link_libraries(${PLUGIN_PARAM_TARGET} ${GET_LIBS_AND_DEFINES_LIBS} DPF DPF_UI  ${LIBLO_LIBRARIES})
        target_include_directories(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES}  ${LIBLO_INCLUDE_DIRS})
        target_compile_definitions(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_DSSI)
    else(PLUGIN_PARAM_UI_SOURCES)
        add_library(${PLUGIN_PARAM_TARGET} MODULE  ${PLUGIN_PARAM_SOURCES})
        set_target_properties(${PLUGIN_PARAM_TARGET} PROPERTIES PREFIX "")
        target_link_libraries(${PLUGIN_PARAM_TARGET} ${GET_LIBS_AND_DEFINES_LIBS}  ${LIBLO_LIBRARIES})
        target_include_directories(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_INCLUDE_DIRECTORIES}  ${LIBLO_INCLUDE_DIRS})
        target_compile_definitions(${PLUGIN_PARAM_TARGET} PUBLIC ${PLUGIN_PARAM_DEFINES} ${GET_LIBS_AND_DEFINES_DEFINES} DISTRHO_PLUGIN_TARGET_DSSI)
    endif(PLUGIN_PARAM_UI_SOURCES)
endfunction(BUILD_DSSI)


#Are u so lazy, u have to call all functions? okay.... but declare erverything...
function(BUILD_ALL_PLUGINS)
    set(oneValueArgs TARGET UI_BACKEND)
    set(multiValueArgs SOURCES INCLUDE_DIRECTORIES DEFINES)
    cmake_parse_arguments(PLUGIN_PARAM "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    BUILD_LV2(TARGET ${PLUGIN_PARAM_TARGET}.lv2 UI_BACKEND ${PLUGIN_PARAM_UI_BACKEND} SOURCES ${PLUGIN_PARAM_SOURCES} INCLUDE_DIRECTORIES ${PLUGIN_PARAM_INCLUDE_DIRECTORIES} DEFINES ${PLUGIN_PARAM_DEFINES})
    BUILD_VST2(TARGET ${PLUGIN_PARAM_TARGET}.vst UI_BACKEND ${PLUGIN_PARAM_UI_BACKEND} SOURCES ${PLUGIN_PARAM_SOURCES} INCLUDE_DIRECTORIES ${PLUGIN_PARAM_INCLUDE_DIRECTORIES} DEFINES ${PLUGIN_PARAM_DEFINES})
    BUILD_DSSI(TARGET ${PLUGIN_PARAM_TARGET}.dssi UI_BACKEND ${PLUGIN_PARAM_UI_BACKEND} SOURCES ${PLUGIN_PARAM_SOURCES} INCLUDE_DIRECTORIES ${PLUGIN_PARAM_INCLUDE_DIRECTORIES} DEFINES ${PLUGIN_PARAM_DEFINES})
    if(${PLUGIN_PARAM_UI_BACKEND} STREQUAL "NONE")
        BUILD_LADSPA(TARGET ${PLUGIN_PARAM_TARGET}.ladspa UI_BACKEND ${PLUGIN_PARAM_UI_BACKEND} SOURCES ${PLUGIN_PARAM_SOURCES} INCLUDE_DIRECTORIES ${PLUGIN_PARAM_INCLUDE_DIRECTORIES} DEFINES ${PLUGIN_PARAM_DEFINES})
    endif(${PLUGIN_PARAM_UI_BACKEND} STREQUAL "NONE")
endfunction(BUILD_ALL_PLUGINS)

###################DOC###################
if
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
