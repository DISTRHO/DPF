#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_info

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	InfoExamplePlugin.cpp

FILES_UI = \
	InfoExampleUI.cpp

# require for modgui builds
MODGUI_CLASS_NAME = distrho_examples_info

# --------------------------------------------------------------
# Do some magic

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
endif # HAVE_OPENGL

TARGETS += lv2_sep
TARGETS += vst2
TARGETS += vst3
TARGETS += clap

all: $(TARGETS)

# --------------------------------------------------------------
