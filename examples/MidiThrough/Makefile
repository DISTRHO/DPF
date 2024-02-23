#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_midiThrough

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	MidiThroughExamplePlugin.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += jack
TARGETS += lv2_dsp
TARGETS += vst2
TARGETS += vst3
TARGETS += clap
TARGETS += au

all: $(TARGETS)

# --------------------------------------------------------------
