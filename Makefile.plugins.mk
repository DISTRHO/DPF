#!/usr/bin/make -f
# Makefile for DPF Example Plugins #
# -------------------------------- #
# Created by falkTX
#

# NOTE: NAME, FILES_DSP and FILES_UI must have been defined before including this file!


ifeq ($(DPF_PATH),)
ifeq (,$(wildcard ../../Makefile.base.mk))
DPF_PATH=../../dpf
else
DPF_PATH=../..
endif
endif

include $(DPF_PATH)/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Basic setup

ifeq ($(DPF_TARGET_DIR),)
TARGET_DIR = ../../bin
else
TARGET_DIR = $(DPF_TARGET_DIR)
endif
ifeq ($(DPF_BUILD_DIR),)
BUILD_DIR = ../../build/$(NAME)
else
BUILD_DIR = $(DPF_BUILD_DIR)
endif

BUILD_C_FLAGS   += -I.
BUILD_CXX_FLAGS += -I. -I$(DPF_PATH)/distrho -I$(DPF_PATH)/dgl

ifeq ($(HAVE_ALSA),true)
BASE_FLAGS += -DHAVE_ALSA
endif

ifeq ($(HAVE_LIBLO),true)
BASE_FLAGS += -DHAVE_LIBLO
endif

ifeq ($(HAVE_PULSEAUDIO),true)
BASE_FLAGS += -DHAVE_PULSEAUDIO
endif

ifeq ($(MACOS),true)
JACK_LIBS  += -framework CoreAudio -framework CoreFoundation
else ifeq ($(WINDOWS),true)
JACK_LIBS  += -lksuser -lmfplat -lmfuuid -lole32 -lwinmm -lwmcodecdspuuid
else ifneq ($(HAIKU),true)
JACK_LIBS   = -ldl
ifeq ($(HAVE_ALSA),true)
JACK_FLAGS += $(ALSA_FLAGS)
JACK_LIBS  += $(ALSA_LIBS)
endif
ifeq ($(HAVE_PULSEAUDIO),true)
JACK_FLAGS += $(PULSEAUDIO_FLAGS)
JACK_LIBS  += $(PULSEAUDIO_LIBS)
endif
ifeq ($(HAVE_RTAUDIO),true)
JACK_LIBS  += -lpthread
endif # !HAIKU
endif

# backwards compat
BASE_FLAGS += -DHAVE_JACK

# ---------------------------------------------------------------------------------------------------------------------
# Set VST3 filename, see https://vst3sdk-doc.diatonic.jp/doc/vstinterfaces/vst3loc.html

ifeq ($(LINUX),true)
VST3_FILENAME = $(TARGET_PROCESSOR)-linux/$(NAME).so
endif
ifeq ($(MACOS),true)
ifneq ($(MACOS_OLD),true)
VST3_FILENAME = MacOS/$(NAME)
endif
endif
ifeq ($(WINDOWS),true)
VST3_FILENAME = $(TARGET_PROCESSOR)-win/$(NAME).vst3
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set files to build

OBJS_DSP = $(FILES_DSP:%=$(BUILD_DIR)/%.o)
OBJS_UI  = $(FILES_UI:%=$(BUILD_DIR)/%.o)

ifeq ($(MACOS),true)
OBJS_UI += $(BUILD_DIR)/DistrhoUI_macOS_$(NAME).mm.o
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set plugin binary file targets

jack       = $(TARGET_DIR)/$(NAME)$(APP_EXT)
ladspa_dsp = $(TARGET_DIR)/$(NAME)-ladspa$(LIB_EXT)
dssi_dsp   = $(TARGET_DIR)/$(NAME)-dssi$(LIB_EXT)
dssi_ui    = $(TARGET_DIR)/$(NAME)-dssi/$(NAME)_ui$(APP_EXT)
lv2        = $(TARGET_DIR)/$(NAME).lv2/$(NAME)$(LIB_EXT)
lv2_dsp    = $(TARGET_DIR)/$(NAME).lv2/$(NAME)_dsp$(LIB_EXT)
lv2_ui     = $(TARGET_DIR)/$(NAME).lv2/$(NAME)_ui$(LIB_EXT)
vst2       = $(TARGET_DIR)/$(NAME)-vst$(LIB_EXT)
ifneq ($(VST3_FILENAME),)
vst3       = $(TARGET_DIR)/$(NAME).vst3/Contents/$(VST3_FILENAME)
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
else
SYMBOLS_LADSPA = -Wl,--version-script=$(DPF_PATH)/utils/symbols/ladspa.version
SYMBOLS_DSSI   = -Wl,--version-script=$(DPF_PATH)/utils/symbols/dssi.version
SYMBOLS_LV2DSP = -Wl,--version-script=$(DPF_PATH)/utils/symbols/lv2-dsp.version
SYMBOLS_LV2UI  = -Wl,--version-script=$(DPF_PATH)/utils/symbols/lv2-ui.version
SYMBOLS_LV2    = -Wl,--version-script=$(DPF_PATH)/utils/symbols/lv2.version
SYMBOLS_VST2   = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst2.version
SYMBOLS_VST3   = -Wl,--version-script=$(DPF_PATH)/utils/symbols/vst3.version
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
DGL_LIB    = $(DPF_PATH)/build/libdgl-cairo.a
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
DGL_LIB    = $(DPF_PATH)/build/libdgl-opengl.a
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
DGL_LIB    = $(DPF_PATH)/build/libdgl-vulkan.a
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
DGL_LIB    = $(DPF_PATH)/build/libdgl-stub.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

DGL_LIBS += $(DGL_SYSTEM_LIBS) -lm

ifneq ($(HAVE_DGL),true)
dssi_ui =
lv2_ui =
DGL_LIBS =
OBJS_UI =
endif

ifneq ($(HAVE_LIBLO),true)
dssi_ui =
endif

# TODO split dsp and ui object build flags
BASE_FLAGS += $(DGL_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------
# all needs to be first

all:

# ---------------------------------------------------------------------------------------------------------------------
# Common

$(BUILD_DIR)/%.S.o: %.S
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/%.c.o: %.c
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/%.cc.o: %.cc
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TARGET_DIR)/$(NAME) $(TARGET_DIR)/$(NAME)-* $(TARGET_DIR)/$(NAME).lv2

# ---------------------------------------------------------------------------------------------------------------------
# DGL

$(DPF_PATH)/build/libdgl-cairo.a:
	$(MAKE) -C $(DPF_PATH)/dgl cairo

$(DPF_PATH)/build/libdgl-opengl.a:
	$(MAKE) -C $(DPF_PATH)/dgl opengl

$(DPF_PATH)/build/libdgl-stub.a:
	$(MAKE) -C $(DPF_PATH)/dgl stub

$(DPF_PATH)/build/libdgl-vulkan.a:
	$(MAKE) -C $(DPF_PATH)/dgl vulkan

# ---------------------------------------------------------------------------------------------------------------------

AS_PUGL_NAMESPACE = $(subst -,_,$(1))

$(BUILD_DIR)/DistrhoPluginMain_%.cpp.o: $(DPF_PATH)/distrho/DistrhoPluginMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginMain.cpp ($*)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_$* -c -o $@

$(BUILD_DIR)/DistrhoUIMain_%.cpp.o: $(DPF_PATH)/distrho/DistrhoUIMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUIMain.cpp ($*)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_$* -c -o $@

$(BUILD_DIR)/DistrhoUI_macOS_%.mm.o: $(DPF_PATH)/distrho/DistrhoUI_macOS.mm
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUI_macOS.mm ($*)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DPUGL_NAMESPACE=$(call AS_PUGL_NAMESPACE,$*) -DGL_SILENCE_DEPRECATION -Wno-deprecated-declarations -I$(DPF_PATH)/dgl/src -I$(DPF_PATH)/dgl/src/pugl-upstream/include -ObjC++ -c -o $@

$(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.o: $(DPF_PATH)/distrho/DistrhoPluginMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginMain.cpp (JACK)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_JACK $(JACK_FLAGS) -c -o $@

$(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.o: $(DPF_PATH)/distrho/DistrhoUIMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUIMain.cpp (DSSI)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(LIBLO_FLAGS) -DDISTRHO_PLUGIN_TARGET_DSSI -c -o $@

# ---------------------------------------------------------------------------------------------------------------------
# JACK

jack: $(jack)

ifeq ($(HAVE_DGL),true)
$(jack): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.o $(BUILD_DIR)/DistrhoUIMain_JACK.cpp.o $(DGL_LIB)
else
$(jack): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating JACK standalone for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(JACK_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LADSPA

ladspa: $(ladspa_dsp)

$(ladspa_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_LADSPA.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LADSPA plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) $(SYMBOLS_LADSPA) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# DSSI

dssi:     $(dssi_dsp) $(dssi_ui)
dssi_dsp: $(dssi_dsp)
dssi_ui:  $(dssi_ui)

$(dssi_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_DSSI.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI plugin library for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) $(SYMBOLS_DSSI) -o $@

$(dssi_ui): $(OBJS_UI) $(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI UI for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(LIBLO_LIBS) -o $@

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
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_LV2) -o $@

$(lv2_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin library for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) $(SYMBOLS_LV2DSP) -o $@

$(lv2_ui): $(OBJS_UI) $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin UI for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_LV2UI) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# VST2

vst2 vst: $(vst2)

ifeq ($(HAVE_DGL),true)
$(vst2): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_VST2.cpp.o $(BUILD_DIR)/DistrhoUIMain_VST2.cpp.o $(DGL_LIB)
else
$(vst2): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_VST2.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating VST2 plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_VST2) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# VST3

vst3: $(vst3)

$(vst3): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_VST3.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating VST3 plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_VST3) -o $@

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

-include $(BUILD_DIR)/DistrhoUIMain_JACK.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_VST2.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_VST3.cpp.d

# ---------------------------------------------------------------------------------------------------------------------
