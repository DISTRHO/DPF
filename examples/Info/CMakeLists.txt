cmake_minimum_required (VERSION 2.8)
project(Info)


BUILD_LV2(TARGET Info UI_BACKEND OPENGL
    SOURCES InfoExamplePlugin.cpp
    UI_SOURCES  InfoExampleUI.cpp
    INCLUDE_DIRECTORIES "./")

BUILD_VST2(TARGET InfoVst UI_BACKEND OPENGL
    SOURCES InfoExamplePlugin.cpp InfoExampleUI.cpp
    INCLUDE_DIRECTORIES "./")

BUILD_JACK(TARGET InfoJack UI_BACKEND OPENGL
    SOURCES InfoExamplePlugin.cpp InfoExampleUI.cpp
    INCLUDE_DIRECTORIES "./")

if(DPF_INSTALL)
INSTALL(TARGETS InfoJack DESTINATION bin)
INSTALL(TARGETS InfoVst DESTINATION lib/vst)
INSTALL(TARGETS Info Info_ui DESTINATION lib/lv2/Info.lv2)
INSTALL(FILES ${Info_TTL_FILES} modgui.ttl DESTINATION lib/lv2/Info.lv2)
endif(DPF_INSTALL)
