#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = d_embedextui

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	EmbedExternalExamplePlugin.cpp

FILES_UI  = \
	EmbedExternalExampleUI.cpp

# --------------------------------------------------------------
# Do some magic

UI_TYPE = external
include ../../Makefile.plugins.mk

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS  += -ObjC++
endif

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += jack
TARGETS += dssi
TARGETS += lv2_sep
TARGETS += vst2
TARGETS += vst3

all: $(TARGETS)

# --------------------------------------------------------------
