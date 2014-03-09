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
	src/Base.cpp.o \
	src/Image.cpp.o \
	src/ImageAboutWindow.cpp.o \
	src/ImageButton.cpp.o \
	src/ImageKnob.cpp.o \
	src/ImageSlider.cpp.o \
	src/Geometry.cpp.o \
	src/Widget.cpp.o \
	src/Window.cpp.o

ifeq ($(MACOS),true)
OBJS += src/pugl/pugl_osx_extended.m.o
endif

TARGET = ../libdgl.a

# --------------------------------------------------------------

all: $(TARGET)

# --------------------------------------------------------------

../libdgl.a: $(OBJS)
	$(RM) $@
	$(AR) crs $@ $^

../libdgl.dll: $(OBJS)
	# -Wl,--output-def,$@.def,--out-implib,$@.a
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

../libdgl.dylib: $(OBJS)
	$(CXX) $^ -dynamiclib $(LINK_FLAGS) -o $@

../libdgl.so: $(OBJS)
	$(CXX) $^ -shared $(LINK_FLAGS) -o $@

# --------------------------------------------------------------

%.cpp.o: %.cpp
	$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

%.m.o: %.m
	$(CC) $< $(BUILD_C_FLAGS) -ObjC -c -o $@

# --------------------------------------------------------------

clean:
	$(RM) src/*.o src/pugl/*.o ../libdgl.*

debug:
	$(MAKE) DEBUG=true

# --------------------------------------------------------------
