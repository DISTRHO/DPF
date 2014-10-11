#!/usr/bin/makefile -f

all: build

ifeq ($(WIN32),true)
build: ../lv2_ttl_generator.exe
else
build: ../lv2_ttl_generator
endif

../lv2_ttl_generator: lv2_ttl_generator.c
	$(CC) lv2_ttl_generator.c -o ../lv2_ttl_generator -ldl

../lv2_ttl_generator.exe: lv2_ttl_generator.c
	$(CC) lv2_ttl_generator.c -o ../lv2_ttl_generator.exe -static
	touch ../lv2_ttl_generator

clean:
	rm -f ../lv2_ttl_generator ../lv2_ttl_generator.exe
