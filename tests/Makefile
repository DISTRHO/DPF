#!/usr/bin/make -f
# Makefile for DGL #
# ---------------- #
# Created by falkTX
#

include ../Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I..
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I.. -I../dgl/src/pugl-upstream/include -DDONT_SET_USING_DGL_NAMESPACE
LINK_FLAGS      += -lpthread

# TODO fix within pugl
BUILD_CXX_FLAGS += -Wno-extra -Wno-missing-field-initializers

# ---------------------------------------------------------------------------------------------------------------------

TESTS    = Application Color Point
ifeq ($(HAVE_CAIRO),true)
TESTS   += Demo.cairo
WTESTS  += Window.cairo
endif
ifeq ($(HAVE_OPENGL),true)
TESTS   += Demo.opengl
WTESTS  += Window.opengl
endif
ifeq ($(HAVE_VULKAN),true)
TESTS   += Demo.vulkan
WTESTS   = Window.vulkan
endif

TARGETS  = $(TESTS:%=../build/tests/%$(APP_EXT))
TARGETS += $(WTESTS:Window.%=../build/tests/Window.%$(APP_EXT))

OBJS     = $(TESTS:%=../build/tests/%.cpp.o)
OBJS    += $(WTESTS:Window.%=../build/tests/Window.cpp.%.o)

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_CAIRO),true)
endif

ifeq ($(HAVE_OPENGL),true)
endif

ifeq ($(HAVE_VULKAN),true)
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

# ---------------------------------------------------------------------------------------------------------------------

define RUN_TEST

	${1}
endef

# 	valgrind --leak-check=full $@

run: $(TARGETS)
	$(foreach TEST,$(TARGETS),$(call RUN_TEST,$(TEST)))

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf ../build/tests

# ---------------------------------------------------------------------------------------------------------------------
# building steps

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

../build/tests/%.cpp.vulkan.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (Vulkan)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_VULKAN -c -o $@

# ---------------------------------------------------------------------------------------------------------------------
# linking steps

../build/tests/%$(APP_EXT): ../build/tests/%.cpp.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) -o $@

../build/tests/%.cairo$(APP_EXT): ../build/tests/%.cpp.cairo.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(CAIRO_LIBS) -o $@

../build/tests/%.opengl$(APP_EXT): ../build/tests/%.cpp.opengl.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/%.vulkan$(APP_EXT): ../build/tests/%.cpp.vulkan.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(VULKAN_LIBS) -o $@

../build/tests/Demo.cairo$(APP_EXT): ../build/tests/Demo.cpp.cairo.o ../build/libdgl-cairo.a
	@echo "Linking Demo (Cairo)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(CAIRO_LIBS) -o $@

../build/tests/Demo.opengl$(APP_EXT): ../build/tests/Demo.cpp.opengl.o ../build/libdgl-opengl.a
	@echo "Linking Demo (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/Demo.vulkan$(APP_EXT): ../build/tests/Demo.cpp.vulkan.o ../build/libdgl-vulkan.a
	@echo "Linking Demo (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(VULKAN_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)
-include ../build/tests/Demo.cpp.cairo.d
-include ../build/tests/Demo.cpp.opengl.d
-include ../build/tests/Demo.cpp.vulkan.d

# ---------------------------------------------------------------------------------------------------------------------
