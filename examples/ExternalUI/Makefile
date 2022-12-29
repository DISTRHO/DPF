#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_extui

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	ExternalExamplePlugin.cpp

FILES_UI  = \
	ExternalExampleUI.cpp

# --------------------------------------------------------------
# Do some magic

UI_TYPE = external
include ../../Makefile.plugins.mk

ifneq ($(MACOS)$(HAIKU),true)
LINK_FLAGS += -ldl
endif

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += jack
TARGETS += dssi
TARGETS += lv2_sep
TARGETS += vst2

all: $(TARGETS)

# --------------------------------------------------------------
