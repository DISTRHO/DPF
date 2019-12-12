cmake_minimum_required (VERSION 2.8)
project(MidiThrogh)


ADD_VST2(TARGET MidiThroughVST UI_BACKEND NONE SOURCES "MidiThroughExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")
ADD_LV2(TARGET MidiThrough UI_BACKEND NONE SOURCES "MidiThroughExamplePlugin.cpp" INCLUDE_DIRECTORIES "./")




INSTALL(TARGETS MidiThroughVST DESTINATION lib/vst)
INSTALL(TARGETS MidiThrough DESTINATION lib/lv2/MidiThrough.lv2)
#When generating files for lv2 plugin, the generated files are saved as ${LV2PLUGINNAME}_TTL_FILES
INSTALL(FILES ${Latency_TTL_FILES} DESTINATION lib/lv2/MidiThrough.lv2)
