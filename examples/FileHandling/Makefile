#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_files

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	FileHandlingPlugin.cpp

FILES_UI = \
	FileHandlingUI.cpp \
	NanoButton.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif

TARGETS += vst2
TARGETS += vst3

all: $(TARGETS)

# --------------------------------------------------------------
