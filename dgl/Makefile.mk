#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

CC  ?= gcc
CXX ?= g++

# --------------------------------------------------------------
# Fallback to Linux if no other OS defined

ifneq ($(HAIKU),true)
ifneq ($(MACOS),true)
ifneq ($(WIN32),true)
LINUX=true
endif
endif
endif

# --------------------------------------------------------------
# Common build and link flags

BASE_FLAGS = -Wall -Wextra -pipe
BASE_OPTS  = -O2 -ffast-math -fdata-sections -ffunction-sections
ifneq ($(NOOPT),true)
BASE_OPTS  += -mtune=generic -msse -msse2 -mfpmath=sse
endif
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,-O1 -Wl,--as-needed -Wl,--gc-sections -Wl,--strip-all

ifeq ($(MACOS),true)
# MacOS linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,-dead_strip -Wl,-dead_strip_dylibs
endif

ifeq ($(RASPPI),true)
# Raspberry-Pi flags
BASE_OPTS  = -O2 -ffast-math
ifneq ($(NOOPT),true)
BASE_OPTS += -march=armv6 -mfpu=vfp -mfloat-abi=hard
endif
LINK_OPTS  = -Wl,-O1 -Wl,--as-needed -Wl,--strip-all
endif

ifneq ($(WIN32),true)
# not needed for Windows
BASE_FLAGS += -fPIC -DPIC
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -DDEBUG -O0 -g
LINK_OPTS   =
else
BASE_FLAGS += -DNDEBUG $(BASE_OPTS) -fvisibility=hidden
CXXFLAGS   += -fvisibility-inlines-hidden
endif

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=c99 -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=c++0x -std=gnu++0x $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) -Wl,--no-undefined $(LDFLAGS)

ifeq ($(MACOS),true)
# No C++11 support
BUILD_CXX_FLAGS = $(BASE_FLAGS) $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) $(LDFLAGS)
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
