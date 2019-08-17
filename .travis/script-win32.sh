#!/bin/bash

set -e

# Preparation
_FLAGS="-DPTW32_STATIC_LIB -Werror"
_PREFIX=i686-w64-mingw32
export AR=${_PREFIX}-ar
export CC=${_PREFIX}-gcc
export CXX=${_PREFIX}-g++
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"
export PATH=/opt/mingw32/${_PREFIX}/bin:/opt/mingw32/bin:${PATH}
export CROSS_COMPILING=true

# Start clean
make clean >/dev/null

# Build now
make HAVE_CAIRO=false HAVE_JACK=false
