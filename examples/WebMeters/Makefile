#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_webmeters

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	ExamplePluginWebMeters.cpp

FILES_UI  = \
	ExampleUIWebMeters.cpp

# --------------------------------------------------------------
# Do some magic

CLAP_BUNDLE = true
VST2_BUNDLE = true
UI_TYPE = webview
include ../../Makefile.plugins.mk

# BUILD_CXX_FLAGS += $(shell pkg-config --cflags gtk+-3.0 webkit2gtk-4.0)
# LINK_FLAGS += $(shell pkg-config --libs gtk+-3.0 webkit2gtk-4.0)

BUILD_CXX_FLAGS += -Wno-unused-parameter -Wno-unused-result
BUILD_CXX_FLAGS += -Wno-deprecated-declarations

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += jack
TARGETS += lv2_sep
TARGETS += vst2
TARGETS += vst3
TARGETS += clap
TARGETS += au

ifeq ($(MACOS_APP_BUNDLE),true)
aufiles   += $(TARGET_DIR)/$(NAME).component/Contents/Resources/index.html
clapfiles += $(TARGET_DIR)/$(NAME).clap/Contents/Resources/index.html
jackfiles += $(TARGET_DIR)/$(NAME).app/Contents/Resources/index.html
vst2files += $(TARGET_DIR)/$(NAME).vst/Contents/Resources/index.html
else
clapfiles += $(TARGET_DIR)/$(NAME).clap/resources/index.html
jackfiles += $(TARGET_DIR)/resources/index.html
lv2files += $(TARGET_DIR)/$(NAME).lv2/resources/index.html
vst2files += $(TARGET_DIR)/$(NAME).vst/resources/index.html
endif

vst3files += $(TARGET_DIR)/$(NAME).vst3/Contents/Resources/index.html

all: $(TARGETS) $(jackfiles) $(clapfiles) $(vst2files) $(vst3files)

%/index.html: index.html
	-$(SILENT)$(shell mkdir -p "$(shell dirname $(abspath $@))")
	install -m 644 $< $(abspath $@)

# --------------------------------------------------------------
