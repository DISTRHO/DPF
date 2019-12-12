cmake_minimum_required (VERSION 2.8)
project(Latency)

#Short version:
#ADD_ALL_PLUGINS(TARGET Latency UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")

#More readable Version:
ADD_VST2(TARGET LatencyVST UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
ADD_DSSI(TARGET LatencyDSSI UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
ADD_LADSPA(TARGET LatencyLADSPA UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
ADD_LV2(TARGET Latency UI_BACKEND NONE SOURCES "LatencyExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")

INSTALL(TARGETS LatencyDSSI  DESTINATION lib/dssi)
INSTALL(TARGETS LatencyLADSPA DESTINATION lib/ladspa)
INSTALL(TARGETS LatencyVST DESTINATION lib/vst)
INSTALL(TARGETS Latency DESTINATION lib/lv2/Latency.lv2)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${Latency_TTL_FILES} DESTINATION lib/lv2/Latency.lv2)
