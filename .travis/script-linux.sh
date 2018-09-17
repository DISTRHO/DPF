#!/bin/bash

set -e

# Preparation
_FLAGS="-Werror"
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"

# Start clean
make clean >/dev/null

# Build now
make
