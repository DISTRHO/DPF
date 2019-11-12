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
	DemoWidgetBanner.cpp \
	DemoWidgetClickable.cpp \
	CairoExampleUI.cpp

# --------------------------------------------------------------
# Do some magic

UI_TYPE = cairo
include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_JACK),true)
ifeq ($(HAVE_CAIRO),true)
TARGETS += jack
endif
endif

ifneq ($(MACOS_OR_WINDOWS),true)
ifeq ($(HAVE_CAIRO),true)
ifeq ($(HAVE_LIBLO),true)
TARGETS += dssi
endif
endif
endif

ifeq ($(HAVE_CAIRO),true)
TARGETS += lv2_sep
else
TARGETS += lv2_dsp
endif

TARGETS += vst

all: $(TARGETS)

# --------------------------------------------------------------
