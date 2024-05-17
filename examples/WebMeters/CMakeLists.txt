# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_webmeters
  UI_TYPE webview
  TARGETS jack lv2 vst2 vst3 clap au
  FILES_DSP
      ExamplePluginWebMeters.cpp
  FILES_UI
      ExampleUIWebMeters.cpp)

target_include_directories(
  d_webmeters PUBLIC ".")
