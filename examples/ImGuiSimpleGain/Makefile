#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX, Christopher Arndt, and Patrick Desaulniers
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_ImGuiSimpleGain

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	PluginSimpleGain.cpp

FILES_UI = \
	UISimpleGain.cpp \
	../../../DPF-Widgets/opengl/DearImGui.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# path to DPF-Widgets
BUILD_CXX_FLAGS += -I../../../DPF-Widgets/opengl/

# --------------------------------------------------------------
# Enable all selected plugin types

TARGETS += jack lv2_sep vst2 vst3

ifeq ($(HAVE_LIBLO),true)
TARGETS += dssi
endif # HAVE_LIBLO

all: $(TARGETS)

# --------------------------------------------------------------
