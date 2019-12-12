cmake_minimum_required (VERSION 2.8)
project(Info)


ADD_LV2(TARGET Info UI_BACKEND OPENGL
    SOURCES InfoExamplePlugin.cpp
    UI_SOURCES  InfoExampleUI.cpp
    INCLUDE_DIRECTORIES "./")

ADD_VST2(TARGET InfoVst UI_BACKEND OPENGL
    SOURCES InfoExamplePlugin.cpp InfoExampleUI.cpp
    INCLUDE_DIRECTORIES "./")

ADD_JACK(TARGET InfoJack UI_BACKEND OPENGL
    SOURCES InfoExamplePlugin.cpp InfoExampleUI.cpp
    INCLUDE_DIRECTORIES "./")
#ADD_DSSI(TARGET CairoUI-DSSI UI_BACKEND CAIRO
#    SOURCES CairoExamplePlugin.cpp
#    UI_SOURCES  DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
#    INCLUDE_DIRECTORIES "./")


INSTALL(TARGETS InfoJack DESTINATION bin)
INSTALL(TARGETS InfoVst DESTINATION lib/vst)
INSTALL(TARGETS Info Info_ui DESTINATION lib/lv2/Info.lv2)
#INSTALL(TARGETS CairoUI-DSSI CairoUI-DSSI_ui DESTINATION lib/dssi)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${Info_TTL_FILES} modgui.ttl DESTINATION lib/lv2/Info.lv2)
#INSTALL(DIR modgui DESTINATION lib/lv2/Info.lv2)
#INSTALL(DIR modgui DESTINATION lib/vst)
