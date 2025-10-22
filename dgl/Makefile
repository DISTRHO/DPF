#!/usr/bin/make -f
# Makefile for DGL #
# ---------------- #
# Created by falkTX
#

include ../Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I. -Isrc
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I. -Isrc -DDONT_SET_USING_DGL_NAMESPACE -Wno-unused-parameter
BUILD_CXX_FLAGS += -Isrc/pugl-upstream/include
LINK_FLAGS      += $(DGL_LIBS)

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -DDISTRHO_MACOS_NAMESPACE_TIME=$(shell date +%s)
BUILD_CXX_FLAGS += -Wno-deprecated-declarations
else
PUGL_EXTRA_FLAGS = -Wno-extra -Wmissing-field-initializers
endif

# ifneq ($(MACOS_OLD),true)
# needed by sofd right now, fix later
# BUILD_CXX_FLAGS += -Wno-type-limits -fpermissive
# endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(MODGUI_BUILD),true)
BUILD_DIR_SUFFIX = -modgui
endif

BUILD_DIR = ../build$(BUILD_DIR_SUFFIX)

# ---------------------------------------------------------------------------------------------------------------------

OBJS_common = \
	$(BUILD_DIR)/dgl/Application.cpp.o \
	$(BUILD_DIR)/dgl/ApplicationPrivateData.cpp.o \
	$(BUILD_DIR)/dgl/Color.cpp.o \
	$(BUILD_DIR)/dgl/EventHandlers.cpp.o \
	$(BUILD_DIR)/dgl/Geometry.cpp.o \
	$(BUILD_DIR)/dgl/ImageBase.cpp.o \
	$(BUILD_DIR)/dgl/ImageBaseWidgets.cpp.o \
	$(BUILD_DIR)/dgl/Layout.cpp.o \
	$(BUILD_DIR)/dgl/Resources.cpp.o \
	$(BUILD_DIR)/dgl/SubWidget.cpp.o \
	$(BUILD_DIR)/dgl/SubWidgetPrivateData.cpp.o \
	$(BUILD_DIR)/dgl/TopLevelWidget.cpp.o \
	$(BUILD_DIR)/dgl/TopLevelWidgetPrivateData.cpp.o \
	$(BUILD_DIR)/dgl/Widget.cpp.o \
	$(BUILD_DIR)/dgl/WidgetPrivateData.cpp.o \
	$(BUILD_DIR)/dgl/Window.cpp.o \
	$(BUILD_DIR)/dgl/WindowPrivateData.cpp.o

ifeq ($(WINDOWS)$(USE_WEB_VIEW),truetrue)
OBJS_common += $(BUILD_DIR)/dgl/WebViewWin32.cpp.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_cairo = $(OBJS_common) \
	$(BUILD_DIR)/dgl/Cairo.cpp.cairo.o

ifeq ($(MACOS),true)
OBJS_cairo += $(BUILD_DIR)/dgl/pugl.mm.cairo.o
else
OBJS_cairo += $(BUILD_DIR)/dgl/pugl.cpp.cairo.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_gles2 = $(OBJS_common) \
	$(BUILD_DIR)/dgl/OpenGL.cpp.gles2.o \
	$(BUILD_DIR)/dgl/OpenGL3.cpp.gles2.o \
	$(BUILD_DIR)/dgl/NanoVG.cpp.gles2.o

ifeq ($(MACOS),true)
OBJS_gles2 += $(BUILD_DIR)/dgl/pugl.mm.gles2.o
else
OBJS_gles2 += $(BUILD_DIR)/dgl/pugl.cpp.gles2.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_gles3 = $(OBJS_common) \
	$(BUILD_DIR)/dgl/OpenGL.cpp.gles3.o \
	$(BUILD_DIR)/dgl/OpenGL3.cpp.gles3.o \
	$(BUILD_DIR)/dgl/NanoVG.cpp.gles3.o

ifeq ($(MACOS),true)
OBJS_gles3 += $(BUILD_DIR)/dgl/pugl.mm.gles3.o
else
OBJS_gles3 += $(BUILD_DIR)/dgl/pugl.cpp.gles3.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_opengl = $(OBJS_common) \
	$(BUILD_DIR)/dgl/OpenGL.cpp.opengl.o \
	$(BUILD_DIR)/dgl/OpenGL2.cpp.opengl.o \
	$(BUILD_DIR)/dgl/NanoVG.cpp.opengl.o

ifeq ($(MACOS),true)
OBJS_opengl += $(BUILD_DIR)/dgl/pugl.mm.opengl.o
else
OBJS_opengl += $(BUILD_DIR)/dgl/pugl.cpp.opengl.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_opengl3 = $(OBJS_common) \
	$(BUILD_DIR)/dgl/OpenGL.cpp.opengl3.o \
	$(BUILD_DIR)/dgl/OpenGL3.cpp.opengl3.o \
	$(BUILD_DIR)/dgl/NanoVG.cpp.opengl3.o

ifeq ($(MACOS),true)
OBJS_opengl3 += $(BUILD_DIR)/dgl/pugl.mm.opengl3.o
else
OBJS_opengl3 += $(BUILD_DIR)/dgl/pugl.cpp.opengl3.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_stub = $(OBJS_common) \
	$(BUILD_DIR)/dgl/Stub.cpp.o

ifeq ($(MACOS),true)
OBJS_stub += $(BUILD_DIR)/dgl/pugl.mm.o
else
OBJS_stub += $(BUILD_DIR)/dgl/pugl.cpp.o
endif

# ---------------------------------------------------------------------------------------------------------------------

OBJS_vulkan = $(OBJS_common) \
	$(BUILD_DIR)/dgl/Vulkan.cpp.vulkan.o

ifeq ($(MACOS),true)
OBJS_vulkan += $(BUILD_DIR)/dgl/pugl.mm.vulkan.o
else
OBJS_vulkan += $(BUILD_DIR)/dgl/pugl.cpp.vulkan.o
endif

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(HAVE_CAIRO),true)
TARGETS += $(BUILD_DIR)/libdgl-cairo.a
endif

ifeq ($(HAVE_OPENGL),true)
TARGETS += $(BUILD_DIR)/libdgl-gles2.a
TARGETS += $(BUILD_DIR)/libdgl-gles3.a
TARGETS += $(BUILD_DIR)/libdgl-opengl.a
TARGETS += $(BUILD_DIR)/libdgl-opengl3.a
endif

ifeq ($(HAVE_STUB),true)
TARGETS += $(BUILD_DIR)/libdgl-stub.a
endif

ifeq ($(HAVE_VULKAN),true)
TARGETS += $(BUILD_DIR)/libdgl-vulkan.a
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)

cairo:   $(BUILD_DIR)/libdgl-cairo.a
gles2:   $(BUILD_DIR)/libdgl-gles2.a
gles3:   $(BUILD_DIR)/libdgl-gles3.a
ifeq ($(WASM),true)
opengl:  gles2
opengl3: gles3
else
opengl:  $(BUILD_DIR)/libdgl-opengl.a
opengl3: $(BUILD_DIR)/libdgl-opengl3.a
endif
stub:    $(BUILD_DIR)/libdgl-stub.a
vulkan:  $(BUILD_DIR)/libdgl-vulkan.a
web:     $(BUILD_DIR)/libdgl-web.a

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/libdgl-cairo.a: $(OBJS_cairo)
	-@mkdir -p $(BUILD_DIR)
	@echo "Creating libdgl-cairo.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(BUILD_DIR)/libdgl-gles2.a: $(OBJS_gles2)
	-@mkdir -p $(BUILD_DIR)
	@echo "Creating libdgl-gles2.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(BUILD_DIR)/libdgl-gles3.a: $(OBJS_gles3)
	-@mkdir -p $(BUILD_DIR)
	@echo "Creating libdgl-gles3.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(BUILD_DIR)/libdgl-opengl.a: $(OBJS_opengl)
	-@mkdir -p $(BUILD_DIR)
	@echo "Creating libdgl-opengl.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(BUILD_DIR)/libdgl-opengl3.a: $(OBJS_opengl3)
	-@mkdir -p $(BUILD_DIR)
	@echo "Creating libdgl-opengl3.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(BUILD_DIR)/libdgl-stub.a: $(OBJS_stub)
	-@mkdir -p $(BUILD_DIR)
	@echo "Creating libdgl-stub.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

$(BUILD_DIR)/libdgl-vulkan.a: $(OBJS_vulkan)
	-@mkdir -p $(BUILD_DIR)
	@echo "Creating libdgl-vulkan.a"
	$(SILENT)rm -f $@
	$(SILENT)$(AR) crs $@ $^

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/%.c.o: src/%.c
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

$(BUILD_DIR)/dgl/%.cpp.o: src/%.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

$(BUILD_DIR)/dgl/%.mm.o: src/%.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/pugl.cpp.o: src/pugl.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) -c -o $@

$(BUILD_DIR)/dgl/pugl.mm.o: src/pugl.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) -c -ObjC++ -o $@

$(BUILD_DIR)/dgl/WebViewWin32.cpp.o: src/WebViewWin32.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -std=gnu++17 -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/%.cpp.cairo.o: src/%.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (Cairo variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -o $@

$(BUILD_DIR)/dgl/%.mm.cairo.o: src/%.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (Cairo variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/%.cpp.gles2.o: src/%.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (GLESv2 variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -DDGL_USE_OPENGL3 -DDGL_USE_GLES -DDGL_USE_GLES2 -UDGL_USE_GLES3 -c -o $@

$(BUILD_DIR)/dgl/%.mm.gles2.o: src/%.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (GLESv2 variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -DDGL_USE_OPENGL3 -DDGL_USE_GLES -DDGL_USE_GLES2 -UDGL_USE_GLES3 -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/%.cpp.gles3.o: src/%.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (GLESv3 variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -DDGL_USE_OPENGL3 -DDGL_USE_GLES -DDGL_USE_GLES3 -UDGL_USE_GLES2 -c -o $@

$(BUILD_DIR)/dgl/%.mm.gles3.o: src/%.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (GLESv3 variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -DDGL_USE_OPENGL3 -DDGL_USE_GLES -DDGL_USE_GLES3 -UDGL_USE_GLES2 -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/%.cpp.opengl.o: src/%.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (OpenGL variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -o $@

$(BUILD_DIR)/dgl/%.mm.opengl.o: src/%.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (OpenGL variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/%.cpp.opengl3.o: src/%.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (OpenGL3 variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -DDGL_USE_OPENGL3 -c -o $@

$(BUILD_DIR)/dgl/%.mm.opengl3.o: src/%.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (OpenGL3 variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -DDGL_USE_OPENGL3 -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

$(BUILD_DIR)/dgl/%.cpp.vulkan.o: src/%.cpp
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (Vulkan variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(VULKAN_FLAGS) -DDGL_VULKAN -c -o $@

$(BUILD_DIR)/dgl/%.mm.vulkan.o: src/%.mm
	-@mkdir -p $(BUILD_DIR)/dgl
	@echo "Compiling $< (Vulkan variant)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(PUGL_EXTRA_FLAGS) $(VULKAN_FLAGS) -DDGL_VULKAN -c -ObjC++ -o $@

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR)/dgl $(BUILD_DIR)/libdgl*.*

debug:
	$(MAKE) DEBUG=true

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS_common:%.o=%.d)
-include $(OBJS_cairo:%.o=%.d)
-include $(OBJS_gles2:%.o=%.d)
-include $(OBJS_gles3:%.o=%.d)
-include $(OBJS_opengl:%.o=%.d)
-include $(OBJS_opengl3:%.o=%.d)
-include $(OBJS_stub:%.o=%.d)
-include $(OBJS_vulkan:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
