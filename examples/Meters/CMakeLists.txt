cmake_minimum_required (VERSION 2.8)
project(Meters)


BUILD_LV2(TARGET Meters UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp
    UI_SOURCES ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")

BUILD_VST2(TARGET MetersVst UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")

if(DSSI_SUPPORT)
BUILD_DSSI(TARGET Meters-DSSI UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp
    UI_SOURCES ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")
endif(DSSI_SUPPORT)

if(DPF_INSTALL)
    INSTALL(TARGETS MetersVst DESTINATION lib/vst)
    INSTALL(TARGETS Meters Meters_ui DESTINATION lib/lv2/Meters.lv2)
    #When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
    INSTALL(FILES ${Meters_TTL_FILES} DESTINATION lib/lv2/Meters.lv2)
    if(DSSI_SUPPORT)
        INSTALL(TARGETS Meters-DSSI Meters-DSSI_ui DESTINATION lib/dssi)
    endif(DSSI_SUPPORT)
endif(DPF_INSTALL)
