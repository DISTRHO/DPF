cmake_minimum_required (VERSION 2.8)
project(States)


ADD_LV2(TARGET States UI_BACKEND OPENGL
    SOURCES ExamplePluginStates.cpp
    UI_SOURCES  ExampleUIStates.cpp
    INCLUDE_DIRECTORIES "./")

ADD_VST2(TARGET StatesVst UI_BACKEND OPENGL
    SOURCES ExamplePluginStates.cpp ExampleUIStates.cpp
    INCLUDE_DIRECTORIES "./")


#ADD_DSSI(TARGET CairoUI-DSSI UI_BACKEND CAIRO
#    SOURCES CairoExamplePlugin.cpp
#    UI_SOURCES  DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
#    INCLUDE_DIRECTORIES "./")



INSTALL(TARGETS StatesVst DESTINATION lib/vst)
INSTALL(TARGETS States States_ui DESTINATION lib/lv2/States.lv2)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${States_TTL_FILES} DESTINATION lib/lv2/States.lv2)
