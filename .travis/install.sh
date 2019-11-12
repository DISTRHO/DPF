#!/bin/bash

set -e

sudo apt-get install -y \
    g++ \
    pkg-config \
    libcairo2-dev \
    libjack-jackd2-dev \
    liblo-dev \
    libgl1-mesa-dev \
    libx11-dev \
    apple-x86-setup \
    mingw32-x-gcc \
    mingw32-x-pkgconfig \
    mingw64-x-gcc \
    mingw64-x-pkgconfig
