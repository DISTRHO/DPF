# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_midiThrough
  TARGETS jack lv2 vst
  FILES_DSP
      MidiThroughExamplePlugin.cpp)

target_include_directories(
  d_midiThrough PUBLIC ".")
