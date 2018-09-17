#!/bin/bash

set -e

# Preparation
_FLAGS="-DDISTRHO_NO_WARNINGS -Werror"
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"
export MACOS_OLD=true
export CROSS_COMPILING=true
. /usr/bin/apple-cross-setup.env

# Start clean
make clean >/dev/null

# Build now
make
