# CMake file for DISTRHO Plugins #
# ------------------------------ #

dpf_add_plugin(d_embed_external_ui
  TARGETS lv2 vst2 vst3 clap
  UI_TYPE external
  FILES_DSP
    EmbedExternalExamplePlugin.cpp
  FILES_UI
    EmbedExternalExampleUI.cpp)

target_include_directories(
    d_embed_external_ui PUBLIC ".")
