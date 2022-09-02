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

TARGETS += jack
TARGETS += ladspa

ifeq ($(HAVE_LIBLO),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += dssi
endif # HAVE_OPENGL
endif # HAVE_LIBLO

TARGETS += lv2_sep
TARGETS += vst
TARGETS += vst3
TARGETS += clap

all: $(TARGETS)

# --------------------------------------------------------------
