#!/usr/bin/make -f
# Makefile for DPT examples #
# ------------------------- #
# Created by falkTX
#

include ../dgl/Makefile.mk

# --------------------------------------------------------------

BUILD_CXX_FLAGS += -I../dgl
LINK_FLAGS      += -L.. -ldgl $(DGL_LIBS)

# --------------------------------------------------------------

ifeq ($(WIN32),true)
TARGETS = dgl-app.exe dgl-color.exe dgl-images.exe
else
TARGETS = dgl-app     dgl-color     dgl-images
endif

# --------------------------------------------------------------

all: ../libdgl.a $(TARGETS)

clean:
	$(MAKE) -C ../dgl clean
	$(RM) $(TARGETS)

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------

%.exe: %
	rm -f $*.exe
	mv $* $*.exe
	ln -sf $*.exe $*

../libdgl.a: .FORCE
	$(MAKE) -C ../dgl

# --------------------------------------------------------------

dgl-app: dgl-app.cpp ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

dgl-color: dgl-color.cpp ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

dgl-images: dgl-images.cpp dgl-images_src/* ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

dgl-cairo: dgl-cairo.cpp ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(shell pkg-config --cflags --libs cairo) $(LINK_FLAGS) -o $@

dgl-text: dgl-text.cpp ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

# --------------------------------------------------------------
