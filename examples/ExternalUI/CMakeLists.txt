cmake_minimum_required (VERSION 2.8)
project(ExternalUI)


ADD_LV2(TARGET ExternalUI UI_BACKEND EXTERNAL
    SOURCES ExternalExamplePlugin.cpp
    UI_SOURCES  ExternalExampleUI.cpp
    INCLUDE_DIRECTORIES "./")

ADD_VST2(TARGET ExternalUIVst UI_BACKEND EXTERNAL
    SOURCES ExternalExamplePlugin.cpp ExternalExampleUI.cpp
    INCLUDE_DIRECTORIES "./")


#ADD_DSSI(TARGET CairoUI-DSSI UI_BACKEND CAIRO
#    SOURCES CairoExamplePlugin.cpp
#    UI_SOURCES  DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
#    INCLUDE_DIRECTORIES "./")



INSTALL(TARGETS ExternalUIVst DESTINATION lib/vst)
INSTALL(TARGETS ExternalUI ExternalUI_ui DESTINATION lib/lv2/ExternalUI.lv2)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${ExternalUI_TTL_FILES} DESTINATION lib/lv2/ExternalUI.lv2)
