#!/usr/bin/make -f
# Makefile for DPF Example Plugins #
# -------------------------------- #
# Created by falkTX
#

# NOTE: NAME, FILES_DSP and FILES_UI must have been defined before including this file!

ifeq ($(DPF_PATH),)
ifneq (,$(wildcard dpf/Makefile.base.mk))
BASE_PATH=.
DPF_PATH=dpf
else ifneq (,$(wildcard ../dpf/Makefile.base.mk))
BASE_PATH=..
DPF_PATH=../dpf
else ifneq (,$(wildcard ../../dpf/Makefile.base.mk))
BASE_PATH=../..
DPF_PATH=../../dpf
else
BASE_PATH=../..
DPF_PATH=../..
endif
endif

include $(DPF_PATH)/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Basic setup

ifeq ($(MODGUI_BUILD),true)
BUILD_DIR_SUFFIX = -modgui
endif

ifneq ($(DPF_BUILD_DIR),)
BUILD_DIR = $(DPF_BUILD_DIR)$(BUILD_DIR_SUFFIX)
else
BUILD_DIR = $(BASE_PATH)/build$(BUILD_DIR_SUFFIX)/$(NAME)
endif

ifneq ($(DPF_TARGET_DIR),)
TARGET_DIR = $(DPF_TARGET_DIR)
else
TARGET_DIR = $(BASE_PATH)/bin
endif

DGL_BUILD_DIR = $(DPF_PATH)/build$(BUILD_DIR_SUFFIX)

BUILD_C_FLAGS   += -I.
BUILD_CXX_FLAGS += -I. -I$(DPF_PATH)/distrho -I$(DPF_PATH)/dgl

ifeq ($(HAVE_ALSA),true)
BASE_FLAGS += -DHAVE_ALSA
endif

ifeq ($(HAVE_JACK),true)
BASE_FLAGS += -DHAVE_JACK
endif

ifeq ($(HAVE_LIBLO),true)
BASE_FLAGS += -DHAVE_LIBLO
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BASE_FLAGS += -DHAVE_PULSEAUDIO
endif

ifeq ($(HAVE_RTAUDIO),true)
BASE_FLAGS += -DHAVE_RTAUDIO
endif

ifeq ($(HAVE_SDL2),true)
BASE_FLAGS += -DHAVE_SDL2
endif

ifneq ($(MODGUI_CLASS_NAME),)
BASE_FLAGS += -DDISTRHO_PLUGIN_MODGUI_CLASS_NAME='"$(MODGUI_CLASS_NAME)"'
endif

# always needed
ifneq ($(HAIKU_OR_MACOS_OR_WASM_OR_WINDOWS),true)
ifneq ($(STATIC_BUILD),true)
LINK_FLAGS += -ldl
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# JACK/Standalone setup

ifeq ($(WASM),true)

JACK_FLAGS += -sUSE_SDL=2
JACK_LIBS  += -sUSE_SDL=2
JACK_LIBS  += -sMAIN_MODULE -ldl

ifneq ($(FILE_BROWSER_DISABLED),true)
JACK_LIBS  += -sEXPORTED_RUNTIME_METHODS=FS,cwrap
endif

else ifneq ($(SKIP_RTAUDIO_FALLBACK),true)

ifeq ($(MACOS),true)
JACK_LIBS  += -framework CoreAudio -framework CoreFoundation -framework CoreMIDI
else ifeq ($(WINDOWS),true)
JACK_LIBS  += -lole32 -lwinmm
# DirectSound
JACK_LIBS  += -ldsound
# WASAPI
# JACK_LIBS  += -lksuser -lmfplat -lmfuuid -lwmcodecdspuuid
else
ifeq ($(HAVE_PULSEAUDIO),true)
JACK_FLAGS += $(PULSEAUDIO_FLAGS)
JACK_LIBS  += $(PULSEAUDIO_LIBS)
endif
ifeq ($(HAVE_ALSA),true)
JACK_FLAGS += $(ALSA_FLAGS)
JACK_LIBS  += $(ALSA_LIBS)
endif
endif

ifeq ($(HAVE_RTAUDIO),true)
ifneq ($(HAIKU),true)
JACK_LIBS  += -lpthread
endif
endif

ifeq ($(HAVE_SDL2),true)
JACK_FLAGS += $(SDL2_FLAGS)
JACK_LIBS  += $(SDL2_LIBS)
endif

endif

# ---------------------------------------------------------------------------------------------------------------------
# Set files to build

OBJS_DSP = $(FILES_DSP:%=$(BUILD_DIR)/%.o)
OBJS_UI  = $(FILES_UI:%=$(BUILD_DIR)/%.o)

ifeq ($(MACOS),true)
OBJS_UI += $(BUILD_DIR)/DistrhoUI_macOS_$(NAME).mm.o
endif

# ---------------------------------------------------------------------------------------------------------------------
# Handle UI stuff, disable UI support automatically

ifeq ($(FILES_UI),)
HAVE_DGL = false
UI_TYPE = none
endif

ifeq ($(UI_TYPE),)
UI_TYPE = opengl
endif

ifeq ($(UI_TYPE),generic)
ifeq ($(HAVE_OPENGL),true)
UI_TYPE = opengl
else ifeq ($(HAVE_CAIRO),true)
UI_TYPE = cairo
endif
endif

ifeq ($(UI_TYPE),cairo)
ifeq ($(HAVE_CAIRO),true)
DGL_FLAGS += -DDGL_CAIRO -DHAVE_DGL
DGL_FLAGS += $(CAIRO_FLAGS)
DGL_LIBS  += $(CAIRO_LIBS)
DGL_LIB    = $(DGL_BUILD_DIR)/libdgl-cairo.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

ifeq ($(UI_TYPE),opengl)
ifeq ($(HAVE_OPENGL),true)
DGL_FLAGS += -DDGL_OPENGL -DHAVE_DGL
DGL_FLAGS += $(OPENGL_FLAGS)
DGL_LIBS  += $(OPENGL_LIBS)
DGL_LIB    = $(DGL_BUILD_DIR)/libdgl-opengl.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

ifeq ($(UI_TYPE),opengl3)
ifeq ($(HAVE_OPENGL),true)
DGL_FLAGS += -DDGL_OPENGL -DDGL_USE_OPENGL3 -DHAVE_DGL
DGL_FLAGS += $(OPENGL_FLAGS)
DGL_LIBS  += $(OPENGL_LIBS)
DGL_LIB    = $(DGL_BUILD_DIR)/libdgl-opengl3.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

ifeq ($(UI_TYPE),vulkan)
ifeq ($(HAVE_VULKAN),true)
DGL_FLAGS += -DDGL_VULKAN -DHAVE_DGL
DGL_FLAGS += $(VULKAN_FLAGS)
DGL_LIBS  += $(VULKAN_LIBS)
DGL_LIB    = $(DGL_BUILD_DIR)/libdgl-vulkan.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

ifeq ($(UI_TYPE),external)
DGL_FLAGS += -DDGL_EXTERNAL
HAVE_DGL   = true
endif

ifeq ($(UI_TYPE),stub)
ifeq ($(HAVE_STUB),true)
DGL_LIB    = $(DGL_BUILD_DIR)/libdgl-stub.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

DGL_LIBS += $(DGL_SYSTEM_LIBS) -lm

# TODO split dsp and ui object build flags
BASE_FLAGS += $(DGL_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------
# Set VST2 filename, either single binary or inside a bundle

ifeq ($(MACOS),true)
VST2_CONTENTS = $(NAME).vst/Contents
VST2_FILENAME = $(VST2_CONTENTS)/MacOS/$(NAME)
else ifeq ($(USE_VST2_BUNDLE),true)
VST2_FILENAME = $(NAME).vst/$(NAME)$(LIB_EXT)
else
VST2_FILENAME = $(NAME)-vst$(LIB_EXT)
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set VST3 filename, see https://vst3sdk-doc.diatonic.jp/doc/vstinterfaces/vst3loc.html

ifeq ($(LINUX),true)
VST3_FILENAME = $(NAME).vst3/Contents/$(TARGET_PROCESSOR)-linux/$(NAME).so
else ifeq ($(MACOS),true)
VST3_CONTENTS = $(NAME).vst3/Contents
VST3_FILENAME = $(VST3_CONTENTS)/MacOS/$(NAME)
else ifeq ($(WASM),true)
VST3_FILENAME = $(NAME).vst3/Contents/wasm/$(NAME).vst3
else ifeq ($(WINDOWS),true)
ifeq ($(CPU_I386),true)
VST3_FILENAME = $(NAME).vst3/Contents/x86-win/$(NAME).vst3
else ifeq ($(CPU_X86_64),true)
VST3_FILENAME = $(NAME).vst3/Contents/x86_64-win/$(NAME).vst3
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set CLAP filename, either single binary or inside a bundle

ifeq ($(MACOS),true)
CLAP_CONTENTS = $(NAME).clap/Contents
CLAP_FILENAME = $(CLAP_CONTENTS)/MacOS/$(NAME)
else ifeq ($(USE_CLAP_BUNDLE),true)
CLAP_FILENAME = $(NAME).clap/$(NAME).clap
else
CLAP_FILENAME = $(NAME).clap
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set plugin binary file targets

ifeq ($(MACOS),true)
ifeq ($(HAVE_DGL),true)
MACOS_APP_BUNDLE = true
endif
endif

ifeq ($(MACOS_APP_BUNDLE),true)
jack       = $(TARGET_DIR)/$(NAME).app/Contents/MacOS/$(NAME)
jackfiles  = $(TARGET_DIR)/$(NAME).app/Contents/Info.plist
else
jack       = $(TARGET_DIR)/$(NAME)$(APP_EXT)
endif
ladspa_dsp = $(TARGET_DIR)/$(NAME)-ladspa$(LIB_EXT)
dssi_dsp   = $(TARGET_DIR)/$(NAME)-dssi$(LIB_EXT)
dssi_ui    = $(TARGET_DIR)/$(NAME)-dssi/$(NAME)_ui$(APP_EXT)
lv2        = $(TARGET_DIR)/$(NAME).lv2/$(NAME)$(LIB_EXT)
lv2_dsp    = $(TARGET_DIR)/$(NAME).lv2/$(NAME)_dsp$(LIB_EXT)
lv2_ui     = $(TARGET_DIR)/$(NAME).lv2/$(NAME)_ui$(LIB_EXT)
vst2       = $(TARGET_DIR)/$(VST2_FILENAME)
ifneq ($(VST3_FILENAME),)
vst3       = $(TARGET_DIR)/$(VST3_FILENAME)
endif
clap       = $(TARGET_DIR)/$(CLAP_FILENAME)
shared     = $(TARGET_DIR)/$(NAME)$(LIB_EXT)
static     = $(TARGET_DIR)/$(NAME).a

ifeq ($(MACOS),true)
vst2files += $(TARGET_DIR)/$(VST2_CONTENTS)/Info.plist
vst2files += $(TARGET_DIR)/$(VST2_CONTENTS)/PkgInfo
vst2files += $(TARGET_DIR)/$(VST2_CONTENTS)/Resources/empty.lproj
vst3files += $(TARGET_DIR)/$(VST3_CONTENTS)/Info.plist
vst3files += $(TARGET_DIR)/$(VST3_CONTENTS)/PkgInfo
vst3files += $(TARGET_DIR)/$(VST3_CONTENTS)/Resources/empty.lproj
clapfiles += $(TARGET_DIR)/$(CLAP_CONTENTS)/Info.plist
clapfiles += $(TARGET_DIR)/$(CLAP_CONTENTS)/PkgInfo
clapfiles += $(TARGET_DIR)/$(CLAP_CONTENTS)/Resources/empty.lproj
endif

ifneq ($(HAVE_DGL),true)
dssi_ui =
lv2_ui =
DGL_LIBS =
OBJS_UI =
endif

ifneq ($(HAVE_LIBLO),true)
dssi_ui =
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set plugin symbols to export

ifeq ($(MACOS),true)
SYMBOLS_LADSPA = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/ladspa.exp
SYMBOLS_DSSI   = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/dssi.exp
SYMBOLS_LV2DSP = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/lv2-dsp.exp
SYMBOLS_LV2UI  = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/lv2-ui.exp
SYMBOLS_LV2    = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/lv2.exp
SYMBOLS_VST2   = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/vst2.exp
SYMBOLS_VST3   = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/vst3.exp
SYMBOLS_CLAP   = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/clap.exp
SYMBOLS_SHARED = -Wl,-exported_symbols_list,$(DPF_PATH)/utils/symbols/shared.exp
else ifeq ($(WASM),true)
SYMBOLS_LADSPA = -sEXPORTED_FUNCTIONS="['ladspa_descriptor']"
SYMBOLS_DSSI   = -sEXPORTED_FUNCTIONS="['ladspa_descriptor','dssi_descriptor']"
SYMBOLS_LV2DSP = -sEXPORTED_FUNCTIONS="['lv2_descriptor','lv2_generate_ttl']"
SYMBOLS_LV2UI  = -sEXPORTED_FUNCTIONS="['lv2ui_descriptor']"
SYMBOLS_LV2    = -sEXPORTED_FUNCTIONS="['lv2_descriptor','lv2_generate_ttl','lv2ui_descriptor']"
SYMBOLS_VST2   = -sEXPORTED_FUNCTIONS="['VSTPluginMain']"
SYMBOLS_VST3   = -sEXPORTED_FUNCTIONS="['GetPluginFactory','ModuleEntry','ModuleExit']"
SYMBOLS_CLAP   = -sEXPORTED_FUNCTIONS="['clap_entry']"
SYMBOLS_SHARED = -sEXPORTED_FUNCTIONS="['createSharedPlugin']"
else ifeq ($(WINDOWS),true)
SYMBOLS_LADSPA = $(DPF_PATH)/utils/symbols/ladspa.def
SYMBOLS_DSSI   = $(DPF_PATH)/utils/symbols/dssi.def
SYMBOLS_LV2DSP = $(DPF_PATH)/utils/symbols/lv2-dsp.def
SYMBOLS_LV2UI  = $(DPF_PATH)/utils/symbols/lv2-ui.def
SYMBOLS_LV2    = $(DPF_PATH)/utils/symbols/lv2.def
SYMBOLS_VST2   = $(DPF_PATH)/utils/symbols/vst2.def
SYMBOLS_VST3   = $(DPF_PATH)/utils/symbols/vst3.def
SYMBOLS_CLAP   = $(DPF_PATH)/utils/symbols/clap.def
SYMBOLS_SHARED = $(DPF_PATH)/utils/symbols/shared.def
else ifneq ($(DEBUG),true)
SYMBOLS_LADSPA = -Wl,--version-script=$(DPF_PATH)/utils/symbols/ladspa.version
SYMBOLS_DSSI   = -Wl,--version-script=$(DPF_PATH)/utils/symbols/dssi.version
SYMBOLS_LV2DSP = -Wl,--version-script=$(DPF_PATH)/utils/symbols/lv2-dsp.version
SYMBOLS_LV2UI  = -Wl,--version-script=$(DPF_PATH)/utils/symbols/lv2-ui.version
SYMBOLS_LV2    = -Wl,--version-script=$(DPF_PATH)/utils/symbols/lv2.version
SYMBOLS_VST2   = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst2.version
SYMBOLS_VST3   = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
SYMBOLS_CLAP   = -Wl,--version-script=$(DPF_PATH)/utils/symbols/clap.version
SYMBOLS_SHARED = -Wl,--version-script=$(DPF_PATH)/utils/symbols/shared.version
endif

# ---------------------------------------------------------------------------------------------------------------------
# Runtime test build

ifeq ($(DPF_RUNTIME_TESTING),true)
BUILD_CXX_FLAGS += -DDPF_RUNTIME_TESTING -Wno-pmf-conversions
endif

# ---------------------------------------------------------------------------------------------------------------------
# all needs to be first

all:

# ---------------------------------------------------------------------------------------------------------------------
# Common

$(BUILD_DIR)/%.S.o: %.S
	-@mkdir -p "$(shell dirname $@)"
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/%.c.o: %.c
	-@mkdir -p "$(shell dirname $@)"
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/%.cc.o: %.cc
	-@mkdir -p "$(shell dirname $@)"
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	-@mkdir -p "$(shell dirname $@)"
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(BUILD_DIR)/%.m.o: %.m
	-@mkdir -p "$(shell dirname $@)"
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -ObjC -c -o $@

$(BUILD_DIR)/%.mm.o: %.mm
	-@mkdir -p "$(shell dirname $@)"
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

clean:
	rm -rf $(BUILD_DIR)
ifeq ($(DPF_BUILD_DIR),)
	rm -rf $(BASE_PATH)/build-modgui/$(NAME)
	rm -rf $(DPF_PATH)/build-modgui
endif
	rm -rf $(TARGET_DIR)/$(NAME)
	rm -rf $(TARGET_DIR)/$(NAME)-*
	rm -rf $(TARGET_DIR)/$(NAME).lv2
	rm -rf $(TARGET_DIR)/$(NAME).vst
	rm -rf $(TARGET_DIR)/$(NAME).vst3
	rm -rf $(TARGET_DIR)/$(NAME).clap

# ---------------------------------------------------------------------------------------------------------------------
# DGL

$(DGL_BUILD_DIR)/libdgl-cairo.a:
	$(MAKE) -C $(DPF_PATH)/dgl cairo

$(DGL_BUILD_DIR)/libdgl-opengl.a:
	$(MAKE) -C $(DPF_PATH)/dgl opengl

$(DGL_BUILD_DIR)/libdgl-opengl3.a:
	$(MAKE) -C $(DPF_PATH)/dgl opengl3

$(DGL_BUILD_DIR)/libdgl-stub.a:
	$(MAKE) -C $(DPF_PATH)/dgl stub

$(DGL_BUILD_DIR)/libdgl-vulkan.a:
	$(MAKE) -C $(DPF_PATH)/dgl vulkan

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/DistrhoPluginMain_%.cpp.o: $(DPF_PATH)/distrho/DistrhoPluginMain.cpp $(EXTRA_DEPENDENCIES) $(EXTRA_DSP_DEPENDENCIES)
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginMain.cpp ($*)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_$* -c -o $@

$(BUILD_DIR)/DistrhoUIMain_%.cpp.o: $(DPF_PATH)/distrho/DistrhoUIMain.cpp $(EXTRA_DEPENDENCIES) $(EXTRA_UI_DEPENDENCIES)
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUIMain.cpp ($*)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_$* -c -o $@

$(BUILD_DIR)/DistrhoUI_macOS_%.mm.o: $(DPF_PATH)/distrho/DistrhoUI_macOS.mm $(EXTRA_DEPENDENCIES) $(EXTRA_UI_DEPENDENCIES)
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUI_macOS.mm ($*)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

$(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.o: $(DPF_PATH)/distrho/DistrhoPluginMain.cpp $(EXTRA_DEPENDENCIES) $(EXTRA_DSP_DEPENDENCIES)
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginMain.cpp (JACK)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_JACK $(JACK_FLAGS) -c -o $@

$(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.o: $(DPF_PATH)/distrho/DistrhoUIMain.cpp $(EXTRA_DEPENDENCIES) $(EXTRA_UI_DEPENDENCIES)
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUIMain.cpp (DSSI)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_DSSI $(LIBLO_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------
# JACK

jack: $(jack) $(jackfiles)

ifeq ($(HAVE_DGL),true)
$(jack): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.o $(BUILD_DIR)/DistrhoUIMain_JACK.cpp.o $(DGL_LIB)
else
$(jack): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating JACK standalone for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(JACK_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LADSPA

ladspa: $(ladspa_dsp)

$(ladspa_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_LADSPA.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LADSPA plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(SHARED) $(SYMBOLS_LADSPA) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# DSSI

dssi:     $(dssi_dsp) $(dssi_ui)
dssi_dsp: $(dssi_dsp)
dssi_ui:  $(dssi_ui)

$(dssi_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_DSSI.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI plugin library for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(SHARED) $(SYMBOLS_DSSI) -o $@

$(dssi_ui): $(OBJS_UI) $(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI UI for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(LIBLO_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LV2

lv2: $(lv2)
lv2_dsp: $(lv2_dsp)
lv2_sep: $(lv2_dsp) $(lv2_ui)

ifeq ($(HAVE_DGL),true)
$(lv2): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.o $(DGL_LIB)
else
$(lv2): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_LV2) -o $@

$(lv2_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin library for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(SHARED) $(SYMBOLS_LV2DSP) -o $@

$(lv2_ui): $(OBJS_UI) $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin UI for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_LV2UI) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LV2 modgui

ifeq ($(MODGUI_BUILD),true)
ifeq ($(MODGUI_CLASS_NAME),)
$(error MODGUI_CLASS_NAME undefined)
endif
endif

# clear all possible flags coming from DPF, while keeping any extra flags specified for this build
MODGUI_IGNORED_FLAGS  = -fdata-sections
MODGUI_IGNORED_FLAGS += -ffast-math
MODGUI_IGNORED_FLAGS += -ffunction-sections
MODGUI_IGNORED_FLAGS += -fno-gnu-unique
MODGUI_IGNORED_FLAGS += -fprefetch-loop-arrays
MODGUI_IGNORED_FLAGS += -fvisibility=hidden
MODGUI_IGNORED_FLAGS += -fvisibility-inlines-hidden
MODGUI_IGNORED_FLAGS += -fPIC
MODGUI_IGNORED_FLAGS += -ldl
MODGUI_IGNORED_FLAGS += -mfpmath=sse
MODGUI_IGNORED_FLAGS += -msse
MODGUI_IGNORED_FLAGS += -msse2
MODGUI_IGNORED_FLAGS += -mtune=generic
MODGUI_IGNORED_FLAGS += -pipe
MODGUI_IGNORED_FLAGS += -std=gnu99
MODGUI_IGNORED_FLAGS += -std=gnu++11
MODGUI_IGNORED_FLAGS += -DDISTRHO_PLUGIN_MODGUI_CLASS_NAME='"$(MODGUI_CLASS_NAME)"'
MODGUI_IGNORED_FLAGS += -DDGL_OPENGL
MODGUI_IGNORED_FLAGS += -DGL_SILENCE_DEPRECATION=1
MODGUI_IGNORED_FLAGS += -DHAVE_ALSA
MODGUI_IGNORED_FLAGS += -DHAVE_DGL
MODGUI_IGNORED_FLAGS += -DHAVE_JACK
MODGUI_IGNORED_FLAGS += -DHAVE_LIBLO
MODGUI_IGNORED_FLAGS += -DHAVE_OPENGL
MODGUI_IGNORED_FLAGS += -DHAVE_PULSEAUDIO
MODGUI_IGNORED_FLAGS += -DHAVE_RTAUDIO
MODGUI_IGNORED_FLAGS += -DHAVE_SDL2
MODGUI_IGNORED_FLAGS += -DNDEBUG
MODGUI_IGNORED_FLAGS += -DPIC
MODGUI_IGNORED_FLAGS += -I.
MODGUI_IGNORED_FLAGS += -I$(DPF_PATH)/distrho
MODGUI_IGNORED_FLAGS += -I$(DPF_PATH)/dgl
MODGUI_IGNORED_FLAGS += -I$(MOD_WORKDIR)/modduo-static/staging/usr/include
MODGUI_IGNORED_FLAGS += -I$(MOD_WORKDIR)/modduox-static/staging/usr/include
MODGUI_IGNORED_FLAGS += -I$(MOD_WORKDIR)/moddwarf/staging/usr/include
MODGUI_IGNORED_FLAGS += -L$(MOD_WORKDIR)/modduo-static/staging/usr/lib
MODGUI_IGNORED_FLAGS += -L$(MOD_WORKDIR)/modduox-static/staging/usr/lib
MODGUI_IGNORED_FLAGS += -L$(MOD_WORKDIR)/moddwarf/staging/usr/lib
MODGUI_IGNORED_FLAGS += -MD
MODGUI_IGNORED_FLAGS += -MP
MODGUI_IGNORED_FLAGS += -O2
MODGUI_IGNORED_FLAGS += -O3
MODGUI_IGNORED_FLAGS += -Wall
MODGUI_IGNORED_FLAGS += -Wextra
MODGUI_IGNORED_FLAGS += -Wl,-O1,--as-needed,--gc-sections
MODGUI_IGNORED_FLAGS += -Wl,-dead_strip,-dead_strip_dylibs
MODGUI_IGNORED_FLAGS += -Wl,-x
MODGUI_IGNORED_FLAGS += -Wl,--gc-sections
MODGUI_IGNORED_FLAGS += -Wl,--no-undefined
MODGUI_IGNORED_FLAGS += -Wl,--strip-all
MODGUI_IGNORED_FLAGS += -Wno-deprecated-declarations
MODGUI_IGNORED_FLAGS += $(DGL_FLAGS)
MODGUI_CFLAGS = $(filter-out $(MODGUI_IGNORED_FLAGS),$(BUILD_C_FLAGS)) -D__MOD_DEVICES__
MODGUI_CXXFLAGS = $(filter-out $(MODGUI_IGNORED_FLAGS),$(BUILD_CXX_FLAGS)) -D__MOD_DEVICES__
MODGUI_LDFLAGS = $(filter-out $(MODGUI_IGNORED_FLAGS),$(LINK_FLAGS))

$(TARGET_DIR)/$(NAME).lv2/modgui/module.js: $(OBJS_UI) $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin modgui for $(NAME)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) \
		-sALLOW_TABLE_GROWTH -sMODULARIZE=1 -sMAIN_MODULE=2 -sDISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0 \
		-sEXPORTED_FUNCTIONS="['_malloc','_free','_modgui_init','_modgui_param_set','_modgui_patch_set','_modgui_cleanup']" \
		-sEXPORTED_RUNTIME_METHODS=['addFunction','lengthBytesUTF8','stringToUTF8','UTF8ToString'] \
		-sEXPORT_NAME="Module_$(MODGUI_CLASS_NAME)" \
		-o $@

modgui:
	$(MAKE) $(TARGET_DIR)/$(NAME).lv2/modgui/module.js \
		EXE_WRAPPER= \
		FILE_BROWSER_DISABLED=true \
		HAVE_OPENGL=true \
		MODGUI_BUILD=true \
		NOOPT=true \
		PKG_CONFIG=false \
		USE_GLES2=true \
		AR=emar \
		CC=emcc \
		CXX=em++ \
		CFLAGS="$(MODGUI_CFLAGS)" \
		CXXFLAGS="$(MODGUI_CXXFLAGS)" \
		LDFLAGS="$(MODGUI_LDFLAGS)"

.PHONY: modgui

# ---------------------------------------------------------------------------------------------------------------------
# VST2

vst2 vst: $(vst2) $(vst2files)

ifeq ($(HAVE_DGL),true)
$(vst2): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_VST2.cpp.o $(BUILD_DIR)/DistrhoUIMain_VST2.cpp.o $(DGL_LIB)
else
$(vst2): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_VST2.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating VST2 plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_VST2) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# VST3

vst3: $(vst3) $(vst3files)

ifeq ($(HAVE_DGL),true)
$(vst3): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_VST3.cpp.o $(BUILD_DIR)/DistrhoUIMain_VST3.cpp.o $(DGL_LIB)
else
$(vst3): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_VST3.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating VST3 plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_VST3) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# CLAP

ifeq ($(HAVE_DGL),true)
ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
CLAP_LIBS = -lpthread
endif
endif
endif

clap: $(clap) $(clapfiles)

ifeq ($(HAVE_DGL),true)
$(clap): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_CLAP.cpp.o $(BUILD_DIR)/DistrhoUIMain_CLAP.cpp.o $(DGL_LIB)
else
$(clap): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_CLAP.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating CLAP plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(CLAP_LIBS) $(SHARED) $(SYMBOLS_CLAP) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# Shared

shared: $(shared)

ifeq ($(HAVE_DGL),true)
$(shared): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_SHARED.cpp.o $(BUILD_DIR)/DistrhoUIMain_SHARED.cpp.o $(DGL_LIB)
else
$(shared): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_SHARED.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating shared library for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(EXTRA_LIBS) $(EXTRA_DSP_LIBS) $(EXTRA_UI_LIBS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# Static

static: $(static)

ifeq ($(HAVE_DGL),true)
$(static): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_STATIC.cpp.o $(BUILD_DIR)/DistrhoUIMain_STATIC.cpp.o
else
$(static): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_STATIC.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating static library for $(NAME)"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------
# macOS files

$(TARGET_DIR)/%.app/Contents/Info.plist: $(DPF_PATH)/utils/plugin.app/Contents/Info.plist
	-@mkdir -p $(shell dirname $@)
	$(SILENT)sed -e "s/@INFO_PLIST_PROJECT_NAME@/$(NAME)/" $< > $@

$(TARGET_DIR)/%.vst/Contents/Info.plist: $(DPF_PATH)/utils/plugin.bundle/Contents/Info.plist
	-@mkdir -p $(shell dirname $@)
	$(SILENT)sed -e "s/@INFO_PLIST_PROJECT_NAME@/$(NAME)/" $< > $@

$(TARGET_DIR)/%.vst3/Contents/Info.plist: $(DPF_PATH)/utils/plugin.bundle/Contents/Info.plist
	-@mkdir -p $(shell dirname $@)
	$(SILENT)sed -e "s/@INFO_PLIST_PROJECT_NAME@/$(NAME)/" $< > $@

$(TARGET_DIR)/%.clap/Contents/Info.plist: $(DPF_PATH)/utils/plugin.bundle/Contents/Info.plist
	-@mkdir -p $(shell dirname $@)
	$(SILENT)sed -e "s/@INFO_PLIST_PROJECT_NAME@/$(NAME)/" $< > $@

$(TARGET_DIR)/%/Contents/PkgInfo: $(DPF_PATH)/utils/plugin.bundle/Contents/PkgInfo
	-@mkdir -p $(shell dirname $@)
	$(SILENT)cp $< $@

$(TARGET_DIR)/%/Resources/empty.lproj: $(DPF_PATH)/utils/plugin.bundle/Contents/Resources/empty.lproj
	-@mkdir -p $(shell dirname $@)
	$(SILENT)cp $< $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_DSP:%.o=%.d)
ifneq ($(UI_TYPE),)
-include $(OBJS_UI:%.o=%.d)
endif

-include $(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_LADSPA.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_DSSI.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_VST2.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_VST3.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_CLAP.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_SHARED.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_STATIC.cpp.d

-include $(BUILD_DIR)/DistrhoUIMain_JACK.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_VST2.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_VST3.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_CLAP.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_SHARED.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_STATIC.cpp.d

# ---------------------------------------------------------------------------------------------------------------------
