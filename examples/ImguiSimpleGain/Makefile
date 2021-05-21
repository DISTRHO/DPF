#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX, Christopher Arndt, and Patrick Desaulniers
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_ImguiSimpleGain

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	PluginSimpleGain.cpp

FILES_UI = \
	UISimpleGain.cpp \
	ImGuiUI.cpp \
	ImGuiSrc.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

BUILD_CXX_FLAGS += -I../../../imgui -I../../../imgui/backends
BUILD_CXX_FLAGS += $(shell $(PKG_CONFIG) glew --cflags)
LINK_FLAGS += $(shell $(PKG_CONFIG) glew --libs)

# --------------------------------------------------------------
# Enable all selected plugin types

ifeq ($(HAVE_OPENGL),true)

ifeq ($(HAVE_JACK),true)
TARGETS += jack
endif # HAVE_JACK

ifneq ($(MACOS_OR_WINDOWS),true)
ifeq ($(HAVE_LIBLO),true)
TARGETS += dssi
endif # HAVE_LIBLO
endif # MACOS_OR_WINDOWS

TARGETS += lv2_sep
TARGETS += vst

endif # HAVE_OPENGL

all: $(TARGETS)

# --------------------------------------------------------------
