# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_info
  TARGETS jack lv2 vst2 vst3 clap
  FILES_DSP
      InfoExamplePlugin.cpp
  FILES_UI
      InfoExampleUI.cpp)

target_include_directories(
  d_info PUBLIC ".")
