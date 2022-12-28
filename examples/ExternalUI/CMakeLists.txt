# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_external_ui
  TARGETS lv2 vst2 vst3 clap
  UI_TYPE external
  FILES_DSP
    ExternalExamplePlugin.cpp
  FILES_UI
    ExternalExampleUI.cpp)

target_include_directories(
    d_external_ui PUBLIC ".")
