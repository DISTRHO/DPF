#!/usr/bin/make -f
# Makefile for DPF Example Plugins #
# -------------------------------- #
# Created by falkTX
#

# NAME, FILES_DSP and FILES_UI have been defined before

include ../../Makefile.mk

ifeq ($(OBJS_UI),)
HAVE_DGL = false
endif

# ---------------------------------------------------------------------------------------------------------------------
# Basic setup

TARGET_DIR = ../../bin
BUILD_DIR = ../../build/$(NAME)

BUILD_C_FLAGS   += -I.
BUILD_CXX_FLAGS += -I. -I../../distrho -I../../dgl

ifeq ($(HAVE_DGL),true)
BASE_FLAGS += -DHAVE_DGL
endif

ifeq ($(HAVE_JACK),true)
BASE_FLAGS += -DHAVE_JACK
endif

ifeq ($(HAVE_LIBLO),true)
BASE_FLAGS += -DHAVE_LIBLO
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set files to build

OBJS_DSP = $(FILES_DSP:%=$(BUILD_DIR)/%.o)
OBJS_UI  = $(FILES_UI:%=$(BUILD_DIR)/%.o)

# ---------------------------------------------------------------------------------------------------------------------
# Set plugin binary file targets

jack       = $(TARGET_DIR)/$(NAME)$(APP_EXT)
ladspa_dsp = $(TARGET_DIR)/$(NAME)-ladspa$(LIB_EXT)
dssi_dsp   = $(TARGET_DIR)/$(NAME)-dssi$(LIB_EXT)
dssi_ui    = $(TARGET_DIR)/$(NAME)-dssi/$(NAME)_ui$(APP_EXT)
lv2        = $(TARGET_DIR)/$(NAME).lv2/$(NAME)$(LIB_EXT)
lv2_dsp    = $(TARGET_DIR)/$(NAME).lv2/$(NAME)_dsp$(LIB_EXT)
lv2_ui     = $(TARGET_DIR)/$(NAME).lv2/$(NAME)_ui$(LIB_EXT)
vst        = $(TARGET_DIR)/$(NAME)-vst$(LIB_EXT)

# ---------------------------------------------------------------------------------------------------------------------
# Set distrho code files

DISTRHO_PLUGIN_FILES = ../../distrho/DistrhoPluginMain.cpp

ifeq ($(HAVE_DGL),true)
DISTRHO_UI_FILES     = ../../distrho/DistrhoUIMain.cpp ../../build/libdgl.a
endif

# ---------------------------------------------------------------------------------------------------------------------
# Handle plugins without UI

ifneq ($(HAVE_DGL),true)
dssi_ui =
lv2_ui =
DISTRHO_UI_FILES =
DGL_LIBS =
OBJS_UI =
endif

# ---------------------------------------------------------------------------------------------------------------------
# all needs to be first

all:

# ---------------------------------------------------------------------------------------------------------------------
# Common

$(BUILD_DIR)/%.c.o: %.c
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TARGET_DIR)/$(NAME) $(TARGET_DIR)/$(NAME)-* $(TARGET_DIR)/$(NAME).lv2

# ---------------------------------------------------------------------------------------------------------------------
# JACK

jack: $(jack)

$(jack): $(OBJS_DSP) $(OBJS_UI) $(DISTRHO_PLUGIN_FILES) $(DISTRHO_UI_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating JACK standalone for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(shell pkg-config --cflags --libs jack) -DDISTRHO_PLUGIN_TARGET_JACK -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LADSPA

ladspa: $(ladspa_dsp)

$(ladspa_dsp): $(OBJS_DSP) $(DISTRHO_PLUGIN_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LADSPA plugin for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) -DDISTRHO_PLUGIN_TARGET_LADSPA -o $@

# ---------------------------------------------------------------------------------------------------------------------
# DSSI

dssi:     $(dssi_dsp) $(dssi_ui)
dssi_dsp: $(dssi_dsp)
dssi_ui:  $(dssi_ui)

$(dssi_dsp): $(OBJS_DSP) $(DISTRHO_PLUGIN_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI plugin library for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) -DDISTRHO_PLUGIN_TARGET_DSSI -o $@

$(dssi_ui): $(OBJS_UI) $(DISTRHO_UI_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI UI for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(shell pkg-config --cflags --libs liblo) -DDISTRHO_PLUGIN_TARGET_DSSI -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LV2

lv2_one: $(lv2)
lv2_dsp: $(lv2_dsp)
lv2_sep: $(lv2_dsp) $(lv2_ui)

$(lv2): $(OBJS_DSP) $(OBJS_UI) $(DISTRHO_PLUGIN_FILES) $(DISTRHO_UI_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) -DDISTRHO_PLUGIN_TARGET_LV2 -o $@

$(lv2_dsp): $(OBJS_DSP) $(DISTRHO_PLUGIN_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin library for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) -DDISTRHO_PLUGIN_TARGET_LV2 -o $@

$(lv2_ui): $(OBJS_UI) $(DISTRHO_UI_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin UI for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) -DDISTRHO_PLUGIN_TARGET_LV2 -o $@

# ---------------------------------------------------------------------------------------------------------------------
# VST

vst: $(vst)

$(vst): $(OBJS_DSP) $(OBJS_UI) $(DISTRHO_PLUGIN_FILES) $(DISTRHO_UI_FILES)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating VST plugin for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) -DDISTRHO_PLUGIN_TARGET_VST -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_DSP:%.o=%.d)
ifeq ($(HAVE_DGL),true)
-include $(OBJS_UI:%.o=%.d)
endif

# ---------------------------------------------------------------------------------------------------------------------
