#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_midiKeyboard

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	MidiKeyboardExamplePlugin.cpp

FILES_UI = \
	resources/MidiKeyboardResources.cpp \
	MidiKeyboardExampleUI.cpp


# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
#  Additional flags

BASE_FLAGS += -I../../dgl/src/nanovg 

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(LINUX),true)
TARGETS += jack
endif

TARGETS += lv2_dsp
TARGETS += vst

all: $(TARGETS)

# --------------------------------------------------------------
