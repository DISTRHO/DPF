cmake_minimum_required (VERSION 2.8)
project(CairoUI)

if(CAIRO_SUPPORT) #Just build when Cairo is Installed. Expected to have all Stuff installed, so no checking of deps.
    BUILD_LV2(TARGET CairoUI UI_BACKEND CAIRO
        SOURCES CairoExamplePlugin.cpp
        UI_SOURCES  DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
        INCLUDE_DIRECTORIES "./")

    BUILD_VST2(TARGET CairoUIVst UI_BACKEND CAIRO
        SOURCES CairoExamplePlugin.cpp DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
        INCLUDE_DIRECTORIES "./")

    BUILD_DSSI(TARGET CairoUI-DSSI UI_BACKEND CAIRO
        SOURCES CairoExamplePlugin.cpp
        UI_SOURCES  DemoWidgetBanner.cpp DemoWidgetClickable.cpp CairoExampleUI.cpp
        INCLUDE_DIRECTORIES "./")


    if(DPF_INSTALL)
        INSTALL(TARGETS CairoUIVst DESTINATION lib/vst)
        INSTALL(TARGETS CairoUI CairoUI_ui DESTINATION lib/lv2/CairoUI.lv2)
        INSTALL(TARGETS CairoUI-DSSI CairoUI-DSSI_ui DESTINATION lib/dssi)
        #When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
        INSTALL(FILES ${CairoUI_TTL_FILES} DESTINATION lib/lv2/CairoUI.lv2)
    endif(DPF_INSTALL)
endif(CAIRO_SUPPORT)
