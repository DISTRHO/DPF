# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_parameters
  TARGETS jack ladspa dssi lv2 vst2 vst3 clap
  FILES_DSP
     ExamplePluginParameters.cpp
  FILES_UI
     ExampleUIParameters.cpp)

target_include_directories(
  d_parameters PUBLIC ".")
