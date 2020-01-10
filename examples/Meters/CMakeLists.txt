cmake_minimum_required (VERSION 2.8)
project(Meters)

## ADD LV2 Plugin wiht UI, but with just one Library
build_lv2(TARGET Meters UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp
     ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")

build_vst2(TARGET MetersVst UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")

if(DSSI_SUPPORT)
build_dssi(TARGET Meters-DSSI UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp
    UI_SOURCES ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")
endif(DSSI_SUPPORT)

if(DPF_INSTALL)
    install(TARGETS MetersVst DESTINATION lib/vst)
    install(TARGETS Meters  DESTINATION lib/lv2/Meters.lv2)
    #When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
    install(FILES ${Meters_TTL_FILES} DESTINATION lib/lv2/Meters.lv2)
    if(DSSI_SUPPORT)
        install(TARGETS Meters-DSSI Meters-DSSI_ui DESTINATION lib/dssi)
    endif(DSSI_SUPPORT)
endif(DPF_INSTALL)
