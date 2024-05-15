#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_web

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	ExamplePluginWebMeters.cpp

FILES_UI  = \
	ExampleUIWebMeters.cpp

# --------------------------------------------------------------
# Do some magic

# MACOS_NO_DEAD_STRIP = true
UI_TYPE = webview
include ../../Makefile.plugins.mk

# BUILD_CXX_FLAGS += $(shell pkg-config --cflags gtk+-3.0 webkit2gtk-4.0)
BUILD_CXX_FLAGS += -Wno-unused-parameter -Wno-unused-result
BUILD_CXX_FLAGS += -Wno-deprecated-declarations
# LINK_FLAGS += $(shell pkg-config --libs gtk+-3.0 webkit2gtk-4.0)

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_OPENGL),true)

TARGETS += jack

ifneq ($(MACOS_OR_WINDOWS),true)
ifeq ($(HAVE_LIBLO),true)
TARGETS += dssi
endif # HAVE_LIBLO
endif # MACOS_OR_WINDOWS

TARGETS += lv2_sep
TARGETS += vst2
TARGETS += vst3
TARGETS += clap
TARGETS += au

endif # HAVE_OPENGL

ifeq ($(MACOS_APP_BUNDLE),true)
jackfiles += $(TARGET_DIR)/$(NAME).app/Contents/Resources/index.html
else
jackfiles += $(TARGET_DIR)/resources/index.html
endif

vst3files += $(TARGET_DIR)/$(NAME).vst3/Contents/Resources/index.html

all: $(TARGETS) $(jackfiles) $(vst3files)

%/index.html: index.html
	-$(SILENT)$(shell mkdir -p "$(shell dirname $(abspath $@))")
	install -m 644 $< $(abspath $@)

# --------------------------------------------------------------
