cmake_minimum_required (VERSION 2.8)
project(Parameters)


ADD_LV2(TARGET Parameters UI_BACKEND OPENGL
    SOURCES ExamplePluginParameters.cpp
    UI_SOURCES ExampleUIParameters.cpp
    INCLUDE_DIRECTORIES "./")

ADD_VST2(TARGET ParametersVst UI_BACKEND OPENGL
    SOURCES ExamplePluginParameters.cpp ExampleUIParameters.cpp
    INCLUDE_DIRECTORIES "./")


#ADD_DSSI(TARGET CairoUI-DSSI UI_BACKEND CAIRO
#    SOURCES CairoExamplePlugin.cpp
#    UI_SOURCES  DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
#    INCLUDE_DIRECTORIES "./")



INSTALL(TARGETS ParametersVst DESTINATION lib/vst)
INSTALL(TARGETS Parameters Parameters_ui DESTINATION lib/lv2/Parameters.lv2)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${Parameters_TTL_FILES} DESTINATION lib/lv2/Parameters.lv2)
