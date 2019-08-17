#!/usr/bin/makefile -f

include ../../Makefile.base.mk

all: build

ifeq ($(WINDOWS),true)

build: ../lv2_ttl_generator.exe

../lv2_ttl_generator.exe: lv2_ttl_generator.c
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS) -static
	touch ../lv2_ttl_generator

else # WINDOWS

ifneq ($(HAIKU),true)
LDFLAGS += -ldl
endif

build: ../lv2_ttl_generator

../lv2_ttl_generator: lv2_ttl_generator.c
	$(CC) $< $(CFLAGS) -o $@ $(LDFLAGS)

endif # WINDOWS

clean:
	rm -f ../lv2_ttl_generator ../lv2_ttl_generator.exe
