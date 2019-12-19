cmake_minimum_required (VERSION 2.8)
project(Latency)

#Short version:
#build_vst2(TARGET LatencyVST UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
#build_dssi(TARGET LatencyDSSI UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
#build_ladspa(TARGET LatencyLADSPA UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
#build_lv2(TARGET Latency UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")


#Longer but just Build, what you can build. In Examples this is quite useful.
if(VST2_SUPPORT)
build_vst2(TARGET LatencyVST UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(VST2_SUPPORT)

if(DSSI_SUPPORT)
build_dssi(TARGET LatencyDSSI UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(DSSI_SUPPORT)

if(LADSPA_SUPPORT)
build_ladspa(TARGET LatencyLADSPA UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(LADSPA_SUPPORT)

if(LV2_SUPPORT)
build_lv2(TARGET Latency UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
endif(LV2_SUPPORT)

if(DPF_INSTALL)
    if(VST2_SUPPORT)
        install(TARGETS LatencyVST DESTINATION lib/vst)
    endif(VST2_SUPPORT)
    if(DSSI_SUPPORT)
        install(TARGETS LatencyDSSI  DESTINATION lib/dssi)
    endif(DSSI_SUPPORT)
    if(LADSPA_SUPPORT)
        install(TARGETS LatencyLADSPA DESTINATION lib/ladspa)
    endif(LADSPA_SUPPORT)
    if(LV2_SUPPORT)
        install(TARGETS Latency DESTINATION lib/lv2/Latency.lv2)
        #When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
        install(FILES ${Latency_TTL_FILES} DESTINATION lib/lv2/Latency.lv2)
    endif(LV2_SUPPORT)
endif(DPF_INSTALL)
