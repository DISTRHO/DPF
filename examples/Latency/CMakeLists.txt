# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_latency
  TARGETS ladspa lv2 vst
  FILES_DSP
      LatencyExamplePlugin.cpp)

target_include_directories(
  d_latency PUBLIC ".")
