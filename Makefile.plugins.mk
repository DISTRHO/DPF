#!/usr/bin/make -f
# Makefile for DPF Example Plugins #
# -------------------------------- #
# Created by falkTX
#

# NOTE: NAME, FILES_DSP and FILES_UI must have been defined before including this file!


ifeq ($(DPF_CUSTOM_PATH),)
ifeq (,$(wildcard ../../Makefile.base.mk))
DPF_PATH=../../dpf
else
DPF_PATH=../..
endif
else
DPF_PATH = $(DPF_CUSTOM_PATH)
endif

include $(DPF_PATH)/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Basic setup

ifeq ($(DPF_CUSTOM_PATH),)
TARGET_DIR = ../../bin
BUILD_DIR = ../../build/$(NAME)
else
ifeq ($(DPF_CUSTOM_TARGET_DIR),)
$(error DPF_CUSTOM_TARGET_DIR is not set)
else
TARGET_DIR = $(DPF_CUSTOM_TARGET_DIR)
endif
ifeq ($(DPF_CUSTOM_BUILD_DIR),)
$(error DPF_CUSTOM_BUILD_DIR is not set)
else
BUILD_DIR = $(DPF_CUSTOM_BUILD_DIR)
endif
endif

BUILD_C_FLAGS   += -I.
BUILD_CXX_FLAGS += -I. -I$(DPF_PATH)/distrho -I$(DPF_PATH)/dgl

ifeq ($(HAVE_LIBLO),true)
BASE_FLAGS += -DHAVE_LIBLO
endif

ifneq ($(HAIKU_OR_MACOS_OR_WINDOWS),true)
JACK_LIBS = -ldl
endif

# backwards compat
BASE_FLAGS += -DHAVE_JACK

# ---------------------------------------------------------------------------------------------------------------------
# Set VST3 filename, see https://vst3sdk-doc.diatonic.jp/doc/vstinterfaces/vst3loc.html

ifeq ($(LINUX),true)
VST3_FILENAME = $(TARGET_PROCESSOR)-linux/$(NAME).so
endif
ifeq ($(MACOS),true)
VST3_FILENAME = MacOS/$(NAME)
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
SYMBOLS_LADSPA = -Wl,-exported_symbol,_ladspa_descriptor
SYMBOLS_DSSI   = -Wl,-exported_symbol,_ladspa_descriptor -Wl,-exported_symbol,_dssi_descriptor
SYMBOLS_LV2    = -Wl,-exported_symbol,_lv2_descriptor -Wl,-exported_symbol,_lv2_generate_ttl
SYMBOLS_LV2UI  = -Wl,-exported_symbol,_lv2ui_descriptor
SYMBOLS_VST2   = -Wl,-exported_symbol,_VSTPluginMain
SYMBOLS_VST3   = -Wl,-exported_symbol,_GetPluginFactory -Wl,-exported_symbol,_bundleEntry -Wl,-exported_symbol,_bundleExit
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

ifeq ($(UI_TYPE),cairo)
ifeq ($(HAVE_CAIRO),true)
DGL_FLAGS += -DDGL_CAIRO
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
DGL_FLAGS += -DDGL_OPENGL
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
DGL_FLAGS += -DDGL_VULKAN
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
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_JACK -c -o $@

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

$(lv2): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) $(SYMBOLS_LV2) $(SYMBOLS_LV2UI)  -o $@

$(lv2_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin library for $(NAME)"
	$(SILENT)$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) $(SYMBOLS_LV2) -o $@

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
-include $(BUILD_DIR)/DistrhoPluginMain_VST.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_VST3.cpp.d

-include $(BUILD_DIR)/DistrhoUIMain_JACK.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_VST.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_VST3.cpp.d

# ---------------------------------------------------------------------------------------------------------------------
