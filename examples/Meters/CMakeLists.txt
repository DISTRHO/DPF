cmake_minimum_required (VERSION 2.8)
project(Meters)


ADD_LV2(TARGET Meters UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp
    UI_SOURCES ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")

ADD_VST2(TARGET MetersVst UI_BACKEND OPENGL
    SOURCES ExamplePluginMeters.cpp ExampleUIMeters.cpp
    INCLUDE_DIRECTORIES "./")


#ADD_DSSI(TARGET CairoUI-DSSI UI_BACKEND CAIRO
#    SOURCES CairoExamplePlugin.cpp
#    UI_SOURCES  DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
#    INCLUDE_DIRECTORIES "./")



INSTALL(TARGETS MetersVst DESTINATION lib/vst)
INSTALL(TARGETS Meters Meters_ui DESTINATION lib/lv2/Meters.lv2)
#INSTALL(TARGETS CairoUI-DSSI CairoUI-DSSI_ui DESTINATION lib/dssi)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${Meters_TTL_FILES} DESTINATION lib/lv2/Meters.lv2)
