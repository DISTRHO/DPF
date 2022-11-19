# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_midiThrough
  TARGETS jack lv2 vst2 vst3 clap
  FILES_DSP
      MidiThroughExamplePlugin.cpp)

target_include_directories(
  d_midiThrough PUBLIC ".")
