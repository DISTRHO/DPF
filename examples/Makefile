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
TARGETS = app.exe color.exe images.exe nekobi-ui.exe
else
ifeq ($(MACOS),true)
TARGETS = app color images nekobi-ui
else
TARGETS = app cairo color images nekobi-ui
endif
endif

# --------------------------------------------------------------

all: ../libdgl.a $(TARGETS)

clean:
	$(MAKE) -C ../dgl clean
	$(RM) *.exe app cairo color images nekobi-ui

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

app: app.cpp ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

cairo: cairo.cpp ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(shell pkg-config --cflags --libs cairo) $(LINK_FLAGS) -o $@

color: color.cpp ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

images: images.cpp images_src/* ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

nekobi-ui: nekobi-ui.cpp nekobi-ui_src/* ../dgl/*
	$(CXX) $< $(BUILD_CXX_FLAGS) $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

.FORCE:
.PHONY: .FORCE

# --------------------------------------------------------------
