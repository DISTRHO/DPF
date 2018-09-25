#!/usr/bin/make -f
# Makefile for DGL #
# ---------------- #
# Created by falkTX
#

include ../Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I. -Isrc
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I. -Isrc
LINK_FLAGS      += $(DGL_LIBS)

# ifneq ($(MACOS_OLD),true)
# needed by sofd right now, fix later
# BUILD_CXX_FLAGS += -Wno-type-limits -fpermissive
# endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS = \
	../build/dgl/Application.cpp.o \
	../build/dgl/Color.cpp.o \
	../build/dgl/Geometry.cpp.o \
	../build/dgl/Image.cpp.o \
	../build/dgl/ImageWidgets.cpp.o \
	../build/dgl/NanoVG.cpp.o \
	../build/dgl/Resources.cpp.o \
	../build/dgl/Widget.cpp.o

ifeq ($(MACOS),true)
OBJS += ../build/dgl/Window.mm.o
else
OBJS += ../build/dgl/Window.cpp.o
endif

TARGET = ../build/libdgl.a

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGET)

# ---------------------------------------------------------------------------------------------------------------------

../build/libdgl.a: $(OBJS)
	-@mkdir -p ../build
	@echo "Creating libdgl.a"
	@rm -f $@
	@$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.c.o: src/%.c
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	@$(CC) $< $(BUILD_C_FLAGS) -c -o $@

../build/dgl/%.cpp.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../build/dgl/Window.cpp.o: src/Window.cpp src/sofd/* src/pugl/*
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../build/dgl/Window.mm.o: src/Window.cpp src/sofd/* src/pugl/*
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf ../build/dgl ../build/libdgl.*

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
