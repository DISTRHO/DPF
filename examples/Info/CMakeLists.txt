cmake_minimum_required (VERSION 2.8)
project(Info)

if(LV2_SUPPORT)
    BUILD_LV2(TARGET Info UI_BACKEND OPENGL
        SOURCES InfoExamplePlugin.cpp
        UI_SOURCES  InfoExampleUI.cpp
        INCLUDE_DIRECTORIES "./")
endif(LV2_SUPPORT)

if(VST2_SUPPORT)
    BUILD_VST2(TARGET InfoVst UI_BACKEND OPENGL
        SOURCES InfoExamplePlugin.cpp InfoExampleUI.cpp
        INCLUDE_DIRECTORIES "./")
endif(VST2_SUPPORT)

if(JACK_SUPPORT)
    BUILD_JACK(TARGET InfoJack UI_BACKEND OPENGL
        SOURCES InfoExamplePlugin.cpp InfoExampleUI.cpp
        INCLUDE_DIRECTORIES "./")
endif(JACK_SUPPORT)

if(DPF_INSTALL)
    if(JACK_SUPPORT)
        INSTALL(TARGETS InfoJack DESTINATION bin)
    endif(JACK_SUPPORT)

    if(VST2_SUPPORT)
        INSTALL(TARGETS InfoVst DESTINATION lib/vst)
    endif(VST2_SUPPORT)

    if(LV2_SUPPORT)
        INSTALL(TARGETS Info Info_ui DESTINATION lib/lv2/Info.lv2)
        INSTALL(FILES ${Info_TTL_FILES} modgui.ttl DESTINATION lib/lv2/Info.lv2)
    endif(LV2_SUPPORT)
endif(DPF_INSTALL)
