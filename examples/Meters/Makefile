#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_meters

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	ExamplePluginMeters.cpp

FILES_UI  = \
	ExampleUIMeters.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_JACK),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
endif
endif

ifneq ($(MACOS_OR_WINDOWS),true)
ifeq ($(HAVE_LIBLO),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += dssi
endif
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
