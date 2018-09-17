#!/usr/bin/make -f
# Makefile for DPF #
# ---------------- #
# Created by falkTX
#

include Makefile.mk

all: dgl examples gen

# --------------------------------------------------------------

dgl:
ifeq ($(HAVE_DGL),true)
	$(MAKE) -C dgl
endif

examples: dgl
	$(MAKE) all -C examples/Info
	$(MAKE) all -C examples/Latency
	$(MAKE) all -C examples/Meters
	$(MAKE) all -C examples/MidiThrough
	$(MAKE) all -C examples/Parameters
	$(MAKE) all -C examples/States

ifneq ($(CROSS_COMPILING),true)
gen: examples utils/lv2_ttl_generator
	@$(CURDIR)/utils/generate-ttl.sh
ifeq ($(MACOS),true)
	@$(CURDIR)/utils/generate-vst-bundles.sh
endif

utils/lv2_ttl_generator:
	$(MAKE) -C utils/lv2-ttl-generator
else
gen:
endif

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C dgl
	$(MAKE) clean -C examples/Info
	$(MAKE) clean -C examples/Latency
	$(MAKE) clean -C examples/Meters
	$(MAKE) clean -C examples/MidiThrough
	$(MAKE) clean -C examples/Parameters
	$(MAKE) clean -C examples/States
	$(MAKE) clean -C utils/lv2-ttl-generator
	rm -rf bin build

# --------------------------------------------------------------

.PHONY: dgl examples
