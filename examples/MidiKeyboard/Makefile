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
	widgets/PianoKey.cpp \
	widgets/KeyboardWidget.cpp \
	MidiKeyboardExampleUI.cpp


# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
#  Additional flags

BASE_FLAGS += -I./widgets/

# --------------------------------------------------------------
# Enable all possible plugin types

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
