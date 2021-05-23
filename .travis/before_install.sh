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

sudo add-apt-repository -y ppa:kxstudio-debian/kxstudio
sudo add-apt-repository -y ppa:kxstudio-debian/mingw
sudo add-apt-repository -y ppa:kxstudio-debian/toolchain
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

sudo apt-get update -qq
sudo apt-get install kxstudio-repos
sudo apt-get update -qq
