cmake_minimum_required (VERSION 2.8)
project(States)


BUILD_LV2(TARGET States UI_BACKEND OPENGL
    SOURCES ExamplePluginStates.cpp
    UI_SOURCES  ExampleUIStates.cpp
    INCLUDE_DIRECTORIES "./")

BUILD_VST2(TARGET StatesVst UI_BACKEND OPENGL
    SOURCES ExamplePluginStates.cpp ExampleUIStates.cpp
    INCLUDE_DIRECTORIES "./")

if(DSSI_SUPPORT)
BUILD_DSSI(TARGET States-DSSI UI_BACKEND OPENGL
    SOURCES ExamplePluginStates.cpp
    UI_SOURCES  ExampleUIStates.cpp
    INCLUDE_DIRECTORIES "./")
endif(DSSI_SUPPORT)

if(DPF_INSTALL)
    install(TARGETS StatesVst DESTINATION lib/vst)
    install(TARGETS States States_ui DESTINATION lib/lv2/States.lv2)
    #When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
    install(FILES ${States_TTL_FILES} DESTINATION lib/lv2/States.lv2)
    if(DSSI_SUPPORT)
        install(TARGETS States-DSSI States-DSSI_ui DESTINATION lib/dssi)
    endif(DSSI_SUPPORT)
endif(DPF_INSTALL)
