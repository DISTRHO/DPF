#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_states

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	ExamplePluginStates.cpp

FILES_UI  = \
	ExampleUIStates.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_JACK),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
endif # HAVE_OPENGL
endif # HAVE_JACK

ifneq ($(MACOS_OR_WINDOWS),true)
TARGETS += dssi
endif # MACOS_OR_WINDOWS

ifeq ($(HAVE_OPENGL),true)
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif

TARGETS += vst

all: $(TARGETS)

# --------------------------------------------------------------
