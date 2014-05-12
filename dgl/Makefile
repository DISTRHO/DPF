#!/usr/bin/make -f
# Makefile for dgl #
# ---------------- #
# Created by falkTX
#

include Makefile.mk

# --------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I.
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I.
LINK_FLAGS      += $(DGL_LIBS)

# --------------------------------------------------------------

OBJS = \
	src/App.cpp.o \
	src/Geometry.cpp.o \
	src/Image.cpp.o \
	src/ImageAboutWindow.cpp.o \
	src/ImageButton.cpp.o \
	src/ImageKnob.cpp.o \
	src/ImageSlider.cpp.o \
	src/Widget.cpp.o \
	src/Window.cpp.o

# freetype-gl
# OBJS += \
# 	src/freetype-gl/font-manager.c.o \
# 	src/freetype-gl/mat4.c.o \
# 	src/freetype-gl/shader.c.o \
# 	src/freetype-gl/text-buffer.c.o \
# 	src/freetype-gl/texture-atlas.c.o \
# 	src/freetype-gl/texture-font.c.o \
# 	src/freetype-gl/vector.c.o \
# 	src/freetype-gl/vertex-attribute.c.o \
# 	src/freetype-gl/vertex-buffer.c.o

ifeq ($(MACOS),true)
OBJS += src/pugl/pugl_osx_extended.m.o
endif

TARGET = ../libdgl.a

# --------------------------------------------------------------

all: $(TARGET)

# --------------------------------------------------------------

../libdgl.a: $(OBJS)
	rm -f $@
	ar crs $@ $^

../libdgl.dll: $(OBJS)
	# -Wl,--output-def,$@.def,--out-implib,$@.a
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libdgl.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libdgl.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.c.o: %.c
	$(CC) $< $(BUILD_C_FLAGS) -c -o $@

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.m.o: %.m
	$(CC) $< $(BUILD_C_FLAGS) -ObjC -c -o $@

# --------------------------------------------------------------

clean:
	rm -f src/*.o src/pugl/*.o src/freetype-gl/*.o ../libdgl.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
