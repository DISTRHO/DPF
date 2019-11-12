#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_latency

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	LatencyExamplePlugin.cpp

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifneq ($(MACOS_OR_WINDOWS),true)
TARGETS += ladspa
TARGETS += dssi
endif

TARGETS += lv2_dsp
TARGETS += vst

all: $(TARGETS)

# --------------------------------------------------------------
