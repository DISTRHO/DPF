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

ifneq ($(HAIKU),true)
LINK_FLAGS += -ldl
endif

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_JACK),true)
TARGETS += jack
endif

TARGETS += dssi
TARGETS += lv2_sep

all: $(TARGETS)

# --------------------------------------------------------------
