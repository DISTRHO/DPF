#!/usr/bin/make -f
# Makefile for DGL #
# ---------------- #
# Created by falkTX
#

# debug mode by default
DEBUG=true

include ../Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I..
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I.. -I../dgl/src/pugl-upstream/include -DDONT_SET_USING_DGL_NAMESPACE
LINK_FLAGS      += -lpthread

# TODO fix within pugl
BUILD_CXX_FLAGS += -Wno-missing-field-initializers -Wno-extra

# ---------------------------------------------------------------------------------------------------------------------

TESTS    = Application Color Point
ifeq ($(HAVE_CAIRO),true)
WTESTS   = Window.cairo
endif
ifeq ($(HAVE_OPENGL),true)
WTESTS   = Window.opengl
endif
ifeq ($(HAVE_VULKAN),true)
WTESTS   = Window.vulkan
endif
TARGETS  = $(TESTS:%=../build/tests/%)
TARGETS += $(WTESTS:Window.%=../build/tests/Window.%)
OBJS     = $(TARGETS:%=%.cpp.o)

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_CAIRO),true)
endif

ifeq ($(HAVE_OPENGL),true)
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

../build/tests/%: ../build/tests/%.cpp.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) -o $@
	@echo "Running test for $*"
	$(SILENT)$@
# 	valgrind --leak-check=full $@

../build/tests/%.cairo: ../build/tests/%.cpp.cairo.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(CAIRO_LIBS) -o $@
	@echo "Running test for $*"
	$(SILENT)$@

../build/tests/%.opengl: ../build/tests/%.cpp.opengl.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@
	@echo "Running test for $*"
	$(SILENT)
	gdb -ex run $@

../build/tests/%.vulkan: ../build/tests/%.cpp.vulkan.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(VULKAN_LIBS) -o $@
	@echo "Running test for $*"
	$(SILENT)$@

# ---------------------------------------------------------------------------------------------------------------------

../build/tests/%.c.o: %.c
	-@mkdir -p ../build/tests
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

../build/tests/%.cpp.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../build/tests/%.cpp.cairo.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (Cairo)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -o $@

../build/tests/%.cpp.opengl.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (OpenGL)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -o $@

../build/tests/%.cpp.vulkan.o: Window.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (Vulkan)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_VULKAN -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf ../build/tests

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
