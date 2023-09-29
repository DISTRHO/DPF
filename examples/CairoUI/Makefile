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

TARGETS += clap
TARGETS += dssi
TARGETS += jack
TARGETS += lv2_sep
TARGETS += vst2

all: $(TARGETS)

# --------------------------------------------------------------
