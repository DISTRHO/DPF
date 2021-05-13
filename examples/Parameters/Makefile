#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_parameters

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	ExamplePluginParameters.cpp

FILES_UI  = \
	ExampleUIParameters.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
endif # HAVE_OPENGL

ifneq ($(MACOS_OR_WINDOWS),true)
TARGETS += ladspa
ifeq ($(HAVE_LIBLO),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += dssi
endif # HAVE_OPENGL
endif # HAVE_LIBLO
endif # MACOS_OR_WINDOWS

ifeq ($(HAVE_OPENGL),true)
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif

TARGETS += vst
TARGETS += vst3

all: $(TARGETS)

# --------------------------------------------------------------
