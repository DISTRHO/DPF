#!/usr/bin/make -f
# Makefile for DGL #
# ---------------- #
# Created by falkTX
#

include ../Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I. -Isrc
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I. -Isrc -DDONT_SET_USING_DGL_NAMESPACE -Wno-unused-parameter
LINK_FLAGS      += $(DGL_LIBS)

# ifneq ($(MACOS_OLD),true)
# needed by sofd right now, fix later
# BUILD_CXX_FLAGS += -Wno-type-limits -fpermissive
# endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_common = \
	../build/dgl/Application.cpp.o \
	../build/dgl/Color.cpp.o \
	../build/dgl/Geometry.cpp.o \
	../build/dgl/ImageBase.cpp.o \
	../build/dgl/Resources.cpp.o \
	../build/dgl/Widget.cpp.o

# TODO: ImageWidgets.cpp

# ---------------------------------------------------------------------------------------------------------------------

OBJS_cairo = $(OBJS_common) \
	../build/dgl/Cairo.cpp.cairo.o \
	../build/dgl/WidgetPrivateData.cpp.cairo.o

ifeq ($(MACOS),true)
OBJS_cairo += ../build/dgl/Window.mm.cairo.o
else
OBJS_cairo += ../build/dgl/Window.cpp.cairo.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_opengl = $(OBJS_common) \
	../build/dgl/OpenGL.cpp.opengl.o \
	../build/dgl/Image.cpp.opengl.o \
	../build/dgl/ImageWidgets.cpp.opengl.o \
	../build/dgl/NanoVG.cpp.opengl.o \
	../build/dgl/WidgetPrivateData.cpp.opengl.o

ifeq ($(MACOS),true)
OBJS_opengl += ../build/dgl/Window.mm.opengl.o
else
OBJS_opengl += ../build/dgl/Window.cpp.opengl.o
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_CAIRO),true)
TARGETS += ../build/libdgl-cairo.a
endif

ifeq ($(HAVE_OPENGL),true)
TARGETS += ../build/libdgl-opengl.a
# Compat name, to be removed soon
TARGETS += ../build/libdgl.a
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

../build/libdgl-cairo.a: $(OBJS_cairo)
	-@mkdir -p ../build
	@echo "Creating libdgl-cairo.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

../build/libdgl-opengl.a: $(OBJS_opengl)
	-@mkdir -p ../build
	@echo "Creating libdgl-opengl.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# Compat name, to be removed soon
../build/libdgl.a: ../build/libdgl-opengl.a
	@echo "Symlinking libdgl.a"
	$(SILENT)ln -sf $< $@

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.c.o: src/%.c
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

../build/dgl/%.cpp.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.cpp.cairo.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (Cairo variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -o $@

../build/dgl/Window.cpp.cairo.o: src/Window.cpp src/sofd/* src/pugl/*
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (Cairo variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -o $@

../build/dgl/Window.mm.cairo.o: src/Window.cpp src/sofd/* src/pugl/*
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (Cairo variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -ObjC++ -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

../build/dgl/%.cpp.opengl.o: src/%.cpp
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (OpenGL variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -o $@

../build/dgl/Window.cpp.opengl.o: src/Window.cpp src/sofd/* src/pugl/*
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (OpenGL variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -o $@

../build/dgl/Window.mm.opengl.o: src/Window.cpp src/sofd/* src/pugl/*
	-@mkdir -p ../build/dgl
	@echo "Compiling $< (OpenGL variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -ObjC++ -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf ../build/dgl ../build/libdgl*.*

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_common:%.o=%.d)
-include $(OBJS_cairo:%.o=%.d)
-include $(OBJS_opengl:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
