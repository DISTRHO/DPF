cmake_minimum_required (VERSION 2.8)
project(ExternalUI)


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
INSTALL(TARGETS ExternalUIVst DESTINATION lib/vst)
INSTALL(TARGETS ExternalUI ExternalUI_ui DESTINATION lib/lv2/ExternalUI.lv2)
INSTALL(TARGETS External-DSSI External-DSSI_ui DESTINATION lib/dssi)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${ExternalUI_TTL_FILES} DESTINATION lib/lv2/ExternalUI.lv2)
endif(DPF_INSTALL)
