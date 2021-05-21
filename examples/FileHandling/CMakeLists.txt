# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_files
  TARGETS jack lv2 vst2
  FILES_DSP
      FileHandlingPlugin.cpp
  FILES_UI
      FileHandlingUI.cpp
      NanoButton.cpp)

target_include_directories(
  d_files PUBLIC ".")
