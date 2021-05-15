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

ifeq ($(HAVE_JACK),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
endif
endif

ifeq ($(HAVE_OPENGL),true)
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif

TARGETS += vst

all: $(TARGETS)

# --------------------------------------------------------------
