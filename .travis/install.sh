#!/bin/bash

set -e

# Special macOS native handling
if [ "${TARGET}" = "macos" ] || [ "${TARGET}" = "macos-universal" ]; then
    HOMEBREW_NO_AUTO_UPDATE=1 brew install cairo jack2 liblo
    exit 0
fi

# Special handling for caching deb archives
if [ "$(ls ${HOME}/debs | wc -l)" -ne 0 ]; then
    sudo cp ${HOME}/debs/*.deb /var/cache/apt/archives/
fi

# common
sudo apt-get install -y build-essential pkg-config

# specific
if [ "${TARGET}" = "win32" ]; then
    sudo apt-get install -y binutils-mingw-w64-i686 g++-mingw-w64-i686 mingw-w64 winehq-stable
elif [ "${TARGET}" = "win64" ]; then
    sudo apt-get install -y binutils-mingw-w64-x86-64 g++-mingw-w64-x86-64 mingw-w64 winehq-stable
else
    sudo apt-get install -y libcairo2-dev libgl1-mesa-dev libjack-jackd2-dev liblo-dev libx11-dev
fi

# Special handling for caching deb archives
sudo mv /var/cache/apt/archives/*.deb ${HOME}/debs/
