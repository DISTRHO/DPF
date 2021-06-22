#!/bin/bash

set -e

# Special macOS native handling
if [ "${TARGET}" = "macos" ] || [ "${TARGET}" = "macos-universal" ]; then
    exit 0
fi

if [ "${TARGET}" = "win32" ] || [ "${TARGET}" = "win64" ]; then
    wget -qO- https://dl.winehq.org/wine-builds/winehq.key | sudo apt-key add -
    sudo apt-add-repository -y 'deb https://dl.winehq.org/wine-builds/ubuntu/ focal main'
    sudo dpkg --add-architecture i386
fi

sudo apt-get update -qq
sudo apt-get install -y -o APT::Immediate-Configure=false libc6 libc6:i386 libgcc-s1:i386
sudo apt-get install -y -f
