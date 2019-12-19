cmake_minimum_required (VERSION 2.8)
project(ExternalUI)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows") #Just build on linux. Expected to have all Stuff installed, so no checking of deps.
    BUILD_LV2(TARGET ExternalUI UI_BACKEND EXTERNAL
        SOURCES ExternalExamplePlugin.cpp
        UI_SOURCES  ExternalExampleUI.cpp
        INCLUDE_DIRECTORIES "./")

    BUILD_VST2(TARGET ExternalUIVst UI_BACKEND EXTERNAL
        SOURCES ExternalExamplePlugin.cpp ExternalExampleUI.cpp
        INCLUDE_DIRECTORIES "./")


    BUILD_DSSI(TARGET External-DSSI UI_BACKEND EXTERNAL
        SOURCES ExternalExamplePlugin.cpp
        UI_SOURCES  ExternalExampleUI.cpp
        INCLUDE_DIRECTORIES "./")

    if(DPF_INSTALL)
        install(TARGETS ExternalUIVst DESTINATION lib/vst)
        install(TARGETS ExternalUI ExternalUI_ui DESTINATION lib/lv2/ExternalUI.lv2)
        install(TARGETS External-DSSI External-DSSI_ui DESTINATION lib/dssi)
        #When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
        install(FILES ${ExternalUI_TTL_FILES} DESTINATION lib/lv2/ExternalUI.lv2)
    endif(DPF_INSTALL)
endif(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
