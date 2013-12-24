#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

AR  ?= ar
RM  ?= rm -f

CC  ?= gcc
CXX ?= g++

# --------------------------------------------------------------
# Fallback to Linux if no other OS defined

ifneq ($(MACOS),true)
ifneq ($(WIN32),true)
LINUX=true
endif
endif

# --------------------------------------------------------------
# Common build and link flags

BASE_FLAGS = -Wall -Wextra -fPIC -DPIC -pipe -DREAL_BUILD
BASE_OPTS  = -O3 -ffast-math -mtune=generic -msse -msse2 -mfpmath=sse

ifeq ($(RASPPI),true)
# Raspberry-Pi optimization flags
BASE_OPTS  = -O3 -ffast-math -march=armv6 -mfpu=vfp -mfloat-abi=hard
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -DDEBUG -O0 -g
else
BASE_FLAGS += -DNDEBUG $(BASE_OPTS) -fvisibility=hidden
CXXFLAGS   += -fvisibility-inlines-hidden
LINK_OPTS  += -Wl,--strip-all
endif

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=gnu++0x $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) -Wl,--no-undefined $(LDFLAGS)

ifeq ($(MACOS),true)
# Get rid of most options for old gcc4.2
BUILD_CXX_FLAGS = $(BASE_FLAGS) $(CXXFLAGS)
LINK_FLAGS      = $(LDFLAGS)
endif

# --------------------------------------------------------------
# Check for required libs

ifeq ($(LINUX),true)
ifneq ($(shell pkg-config --exists gl && echo true),true)
$(error OpenGL missing, cannot continue)
endif
ifneq ($(shell pkg-config --exists x11 && echo true),true)
$(error X11 missing, cannot continue)
endif
endif

# --------------------------------------------------------------
# Set libs stuff

ifeq ($(LINUX),true)
DGL_FLAGS = $(shell pkg-config --cflags gl x11)
DGL_LIBS  = $(shell pkg-config --libs gl x11)
endif

ifeq ($(MACOS),true)
DGL_LIBS  = -framework OpenGL -framework Cocoa
endif

ifeq ($(WIN32),true)
DGL_LIBS  = -lopengl32 -lgdi32
endif

# --------------------------------------------------------------
