#!/bin/bash

set -e

sudo add-apt-repository -y ppa:kxstudio-debian/kxstudio
sudo add-apt-repository -y ppa:kxstudio-debian/mingw
sudo add-apt-repository -y ppa:kxstudio-debian/toolchain
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test

sudo apt-get update -qq
sudo apt-get install kxstudio-repos
sudo apt-get update -qq
