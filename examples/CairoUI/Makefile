#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_cairoui

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	CairoExamplePlugin.cpp

FILES_UI  = \
	Artwork.cpp \
	DemoWidgetBanner.cpp \
	CairoExampleUI.cpp

# --------------------------------------------------------------
# Do some magic

UI_TYPE = cairo
include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_CAIRO),true)

TARGETS += jack

ifneq ($(MACOS_OR_WINDOWS),true)
ifeq ($(HAVE_LIBLO),true)
TARGETS += dssi
endif # HAVE_LIBLO
endif # MACOS_OR_WINDOWS

TARGETS += lv2_sep
TARGETS += vst2
TARGETS += vst3

endif # HAVE_CAIRO

all: $(TARGETS)

# --------------------------------------------------------------
