cmake_minimum_required (VERSION 2.8)
project(Latency)

#Short version:
#BUILD_VST2(TARGET LatencyVST UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
#BUILD_DSSI(TARGET LatencyDSSI UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
#BUILD_LADSPA(TARGET LatencyLADSPA UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
#BUILD_LV2(TARGET Latency UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")


#Longer but just Build, what you can build. In Examples this is quite useful.
if(VST2_SUPPORT)
BUILD_VST2(TARGET LatencyVST UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(VST2_SUPPORT)

if(DSSI_SUPPORT)
BUILD_DSSI(TARGET LatencyDSSI UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(DSSI_SUPPORT)

if(LADSPA_SUPPORT)
BUILD_LADSPA(TARGET LatencyLADSPA UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(LADSPA_SUPPORT)

if(LV2_SUPPORT)
BUILD_LV2(TARGET Latency UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(LV2_SUPPORT)

if(DPF_INSTALL)
    if(VST2_SUPPORT)
        INSTALL(TARGETS LatencyVST DESTINATION lib/vst)
    endif(VST2_SUPPORT)
    if(DSSI_SUPPORT)
        INSTALL(TARGETS LatencyDSSI  DESTINATION lib/dssi)
    endif(DSSI_SUPPORT)
    if(LADSPA_SUPPORT)
        INSTALL(TARGETS LatencyLADSPA DESTINATION lib/ladspa)
    endif(LADSPA_SUPPORT)
    if(LV2_SUPPORT)
        INSTALL(TARGETS Latency DESTINATION lib/lv2/Latency.lv2)
        #When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
        INSTALL(FILES ${Latency_TTL_FILES} DESTINATION lib/lv2/Latency.lv2)
    endif(LV2_SUPPORT)
endif(DPF_INSTALL)
