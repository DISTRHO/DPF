#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_metronome

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	ExamplePluginMetronome.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += jack
TARGETS += lv2_dsp
TARGETS += vst2
TARGETS += vst3

all: $(TARGETS)

# --------------------------------------------------------------
