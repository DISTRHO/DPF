#!/usr/bin/make -f
# Makefile for DPF Example Plugins #
# -------------------------------- #
# Created by falkTX
#

# NOTE: NAME, FILES_DSP and FILES_UI must have been defined before including this file!


ifeq (,$(wildcard ../../Makefile.base.mk))
DPF_PATH=../../dpf
else
DPF_PATH=../..
endif

include $(DPF_PATH)/Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------
# Basic setup

TARGET_DIR = ../../bin
BUILD_DIR = ../../build/$(NAME)

BUILD_C_FLAGS   += -I.
BUILD_CXX_FLAGS += -I. -I$(DPF_PATH)/distrho -I$(DPF_PATH)/dgl

ifeq ($(HAVE_CAIRO),true)
DGL_FLAGS += -DHAVE_CAIRO
endif

ifeq ($(HAVE_OPENGL),true)
DGL_FLAGS += -DHAVE_OPENGL
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
au_plugin  = $(TARGET_DIR)/$(NAME).component/Contents/MacOS/$(NAME)
au_pkginfo = $(TARGET_DIR)/$(NAME).component/Contents/PkgInfo
au_plist   = $(TARGET_DIR)/$(NAME).component/Contents/Info.plist
au_rsrc    = $(TARGET_DIR)/$(NAME).component/Contents/Resources/$(NAME).rsrc

# ---------------------------------------------------------------------------------------------------------------------
# Set stuff needed for AU

AU_BUILD_FLAGS = \
	-I$(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/AUBase \
	-I$(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/Utility \
	-I$(DPF_PATH)/distrho/src/CoreAudio106/PublicUtility\
    -Wno-deprecated-declarations \
    -Wno-four-char-constants \
    -Wno-overloaded-virtual \
    -Wno-unused-parameter \
    -DTARGET_OS_MAC

AU_LINK_FLAGS = \
     -bundle \
	 -framework AudioToolbox \
	 -framework AudioUnit \
	 -framework CoreAudio \
	 -framework CoreServices \
     -exported_symbols_list $(DPF_PATH)/distrho/src/DistrhoPluginAU.exp

# not needed yet
#	 -I$(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/AUCarbonViewBase
#  	 -I$(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/AUInstrumentBase
#	 -I$(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/AUViewBase
#	 -I$(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/OtherBases


# ---------------------------------------------------------------------------------------------------------------------
# Handle UI stuff, disable UI support automatically

ifeq ($(FILES_UI),)
UI_TYPE = none
endif

ifeq ($(UI_TYPE),)
UI_TYPE = opengl
endif

ifeq ($(UI_TYPE),cairo)
ifeq ($(HAVE_CAIRO),true)
DGL_FLAGS += $(CAIRO_FLAGS) -DDGL_CAIRO
DGL_LIBS  += $(CAIRO_LIBS)
DGL_LIB    = $(DPF_PATH)/build/libdgl-cairo.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

ifeq ($(UI_TYPE),opengl)
ifeq ($(HAVE_OPENGL),true)
DGL_FLAGS += $(OPENGL_FLAGS) -DDGL_OPENGL
DGL_LIBS  += $(OPENGL_LIBS)
DGL_LIB    = $(DPF_PATH)/build/libdgl-opengl.a
HAVE_DGL   = true
else
HAVE_DGL   = false
endif
endif

DGL_LIBS += $(DGL_SYSTEM_LIBS)

ifneq ($(HAVE_DGL),true)
dssi_ui =
lv2_ui =
DGL_LIBS =
OBJS_UI =
endif

# TODO split dsp and ui object build flags
BASE_FLAGS += $(DGL_FLAGS)

# ---------------------------------------------------------------------------------------------------------------------
# all needs to be first

all:

# ---------------------------------------------------------------------------------------------------------------------
# Common

$(BUILD_DIR)/%.c.o: %.c
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/%.cc.o: %.cc
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(TARGET_DIR)/$(NAME) $(TARGET_DIR)/$(NAME)-* $(TARGET_DIR)/$(NAME).lv2

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/DistrhoPluginMain_%.cpp.o: $(DPF_PATH)/distrho/DistrhoPluginMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginMain.cpp ($*)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_$* -c -o $@

$(BUILD_DIR)/DistrhoUIMain_%.cpp.o: $(DPF_PATH)/distrho/DistrhoUIMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUIMain.cpp ($*)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -DDISTRHO_PLUGIN_TARGET_$* -c -o $@

$(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.o: $(DPF_PATH)/distrho/DistrhoPluginMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginMain.cpp (JACK)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(shell $(PKG_CONFIG) --cflags jack) -DDISTRHO_PLUGIN_TARGET_JACK -c -o $@

$(BUILD_DIR)/DistrhoPluginMain_AU.cpp.o: $(DPF_PATH)/distrho/DistrhoPluginMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginMain.cpp (AU)"
	$(CXX) $< $(BUILD_CXX_FLAGS) $(AU_BUILD_FLAGS) -DDISTRHO_PLUGIN_TARGET_AU -c -o $@

$(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.o: $(DPF_PATH)/distrho/DistrhoUIMain.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoUIMain.cpp (DSSI)"
	@$(CXX) $< $(BUILD_CXX_FLAGS) $(shell $(PKG_CONFIG) --cflags liblo) -DDISTRHO_PLUGIN_TARGET_DSSI -c -o $@

$(BUILD_DIR)/DistrhoPluginAUexport.cpp.o: $(DPF_PATH)/distrho/src/DistrhoPluginAUexport.cpp
	-@mkdir -p $(BUILD_DIR)
	@echo "Compiling DistrhoPluginAUexport.cpp"
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

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
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(shell $(PKG_CONFIG) --libs jack) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LADSPA

ladspa: $(ladspa_dsp)

$(ladspa_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_LADSPA.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LADSPA plugin for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# DSSI

dssi:     $(dssi_dsp) $(dssi_ui)
dssi_dsp: $(dssi_dsp)
dssi_ui:  $(dssi_ui)

$(dssi_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_DSSI.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI plugin library for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) -o $@

$(dssi_ui): $(OBJS_UI) $(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DSSI UI for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(shell $(PKG_CONFIG) --libs liblo) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# LV2

lv2: $(lv2)
lv2_dsp: $(lv2_dsp)
lv2_sep: $(lv2_dsp) $(lv2_ui)

$(lv2): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) -o $@

$(lv2_dsp): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin library for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(SHARED) -o $@

$(lv2_ui): $(OBJS_UI) $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.o $(DGL_LIB)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating LV2 plugin UI for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# VST

vst: $(vst)

ifeq ($(HAVE_DGL),true)
$(vst): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_VST.cpp.o $(BUILD_DIR)/DistrhoUIMain_VST.cpp.o $(DGL_LIB)
else
$(vst): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_VST.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating VST plugin for $(NAME)"
	@$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(DGL_LIBS) $(SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# AU

au: $(au_plugin) $(au_pkginfo) $(au_plist) $(au_rsrc)

ifeq ($(HAVE_DGL),true)
$(au_plugin): $(OBJS_DSP) $(OBJS_UI) $(BUILD_DIR)/DistrhoPluginMain_AU.cpp.o $(BUILD_DIR)/DistrhoUIMain_AU.cpp.o $(DGL_LIB)
else
$(au_plugin): $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginMain_AU.cpp.o
endif
	-@mkdir -p $(shell dirname $@)
	@echo "Creating AU plugin for $(NAME)"
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) $(AU_LINK_FLAGS) $(DGL_LIBS) -o $@

$(au_pkginfo):
	-@mkdir -p $(shell dirname $@)
	@echo "Creating AU PkgInfo for $(NAME)"
	cp $(DPF_PATH)/distrho/src/CoreAudio106/PkgInfo $@

$(au_plist):
	-@mkdir -p $(shell dirname $@)
	@echo "Creating AU Info.plist for $(NAME)"
	sed -e "s/X-DPF-EXECUTABLE-DPF-X/$(NAME)/g" $(DPF_PATH)/distrho/src/CoreAudio106/Info.plist > $@

$(au_rsrc): $(BUILD_DIR)/step2.rsrc
	-@mkdir -p $(shell dirname $@)
	@echo "Creating AU rsrc for $(NAME) (part 3/3)"
	ResMerger $< -dstIs DF -o $@

$(BUILD_DIR)/step2.rsrc: $(BUILD_DIR)/step1.rsrc
	-@mkdir -p $(shell dirname $@)
	@echo "Creating AU rsrc for $(NAME) (part 2/3)"
	ResMerger -dstIs DF $< -o $@

$(BUILD_DIR)/step1.rsrc: $(DPF_PATH)/distrho/src/DistrhoPluginAU.r $(BUILD_DIR)/DistrhoPluginInfo.r $(OBJS_DSP)
	-@mkdir -p $(shell dirname $@)
	@echo "Creating AU rsrc for $(NAME) (part 1/3)"
	Rez $< \
		-d SystemSevenOrLater=1 \
		-useDF -script Roman \
        -d x86_64_YES -d i386_YES -arch x86_64 -arch i386 \
		-i . \
		-i $(BUILD_DIR) \
		-i $(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic \
        -i $(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/AUBase \
        -i $(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/OtherBases \
        -i $(DPF_PATH)/distrho/src/CoreAudio106/AudioUnits/AUPublic/Utility \
        -i $(DPF_PATH)/distrho/src/CoreAudio106/PublicUtility \
        -I /System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/A/Headers \
		-o $@

$(BUILD_DIR)/DistrhoPluginInfo.r: $(OBJS_DSP) $(BUILD_DIR)/DistrhoPluginAUexport.cpp.o
	-@mkdir -p $(shell dirname $@)
	@echo "Creating DistrhoPluginInfo.r for $(NAME)"
	$(CXX) $^ $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $(BUILD_DIR)/DistrhoPluginInfoGenerator && \
		$(BUILD_DIR)/DistrhoPluginInfoGenerator "$(BUILD_DIR)" && \
		rm $(BUILD_DIR)/DistrhoPluginInfoGenerator

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_DSP:%.o=%.d)
ifeq ($(HAVE_DGL),true)
-include $(OBJS_UI:%.o=%.d)
endif

-include $(BUILD_DIR)/DistrhoPluginMain_JACK.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_LADSPA.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_DSSI.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_LV2.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_VST.cpp.d
-include $(BUILD_DIR)/DistrhoPluginMain_AU.cpp.d

-include $(BUILD_DIR)/DistrhoUIMain_JACK.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_DSSI.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_LV2.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_VST.cpp.d
-include $(BUILD_DIR)/DistrhoUIMain_AU.cpp.d

# ---------------------------------------------------------------------------------------------------------------------
