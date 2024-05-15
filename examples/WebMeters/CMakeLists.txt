# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_meters
  TARGETS jack dssi lv2 vst2 vst3 clap
  FILES_DSP
      ExamplePluginMeters.cpp
  FILES_UI
      ExampleUIMeters.cpp)

target_include_directories(
  d_meters PUBLIC ".")
