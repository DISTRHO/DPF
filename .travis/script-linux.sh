#!/bin/bash

set -e

# Preparation
_FLAGS="-Werror"
export CFLAGS="${_FLAGS}"
export CXXFLAGS="${_FLAGS}"

echo "==============> Normal build"
make clean >/dev/null
make

echo "==============> No namespace build"
make clean >/dev/null
make CXXFLAGS="${_FLAGS} -DDONT_SET_USING_DISTRHO_NAMESPACE -DDONT_SET_USING_DGL_NAMESPACE"

echo "==============> Custom namespace build"
make clean >/dev/null
make CXXFLAGS="${_FLAGS} -DDISTRHO_NAMESPACE=WubbWubb -DDGL_NAMESPACE=DabDab"
