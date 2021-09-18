#!/usr/bin/makefile -f

include ../../Makefile.base.mk

all: build

ifeq ($(WINDOWS),true)

build: ../lv2_ttl_generator.exe

../lv2_ttl_generator.exe: lv2_ttl_generator.c
	$(CC) $< $(BUILD_C_FLAGS) -o $@ $(LINK_FLAGS) -static
	touch ../lv2_ttl_generator

else # WINDOWS

ifneq ($(HAIKU_OR_MACOS_OR_WINDOWS),true)
LINK_FLAGS += -ldl
endif

build: ../lv2_ttl_generator

../lv2_ttl_generator: lv2_ttl_generator.c
	$(CC) $< $(BUILD_C_FLAGS) -o $@ $(LINK_FLAGS)

endif # WINDOWS

clean:
	rm -f ../lv2_ttl_generator ../lv2_ttl_generator.exe
