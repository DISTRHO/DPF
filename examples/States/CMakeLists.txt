# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_states
  TARGETS jack lv2 vst2 vst3 clap
  FILES_DSP
      ExamplePluginStates.cpp
  FILES_UI
      ExampleUIStates.cpp)

target_include_directories(
  d_states PUBLIC ".")
