#!/usr/bin/makefile -f

all: build

build: ../lv2_ttl_generator
mingw: ../lv2_ttl_generator.exe

../lv2_ttl_generator: lv2_ttl_generator.c
	$(CXX) lv2_ttl_generator.c -o ../lv2_ttl_generator -ldl

../lv2_ttl_generator.exe: lv2_ttl_generator.c
	$(CXX) lv2_ttl_generator.c -o ../lv2_ttl_generator.exe -static
	touch ../lv2_ttl_generator

clean:
	rm -f ../lv2_ttl_generator ../lv2_ttl_generator.exe
