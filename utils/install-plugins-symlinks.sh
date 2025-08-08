#!/bin/bash

# the realpath function is not available on some systems
if ! which realpath &>/dev/null; then
    function realpath() {
        [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
    }
fi

set -e

DPF_UTILS_DIR="$(dirname $(realpath ${0}))"

if [ -d bin ]; then
  cd bin
elif [ -d build/bin ]; then
  cd build/bin
else
  echo "Please run this script from the root folder"
  exit
fi

if [ "$(uname -s)" = "Darwin" ]; then
    CLAP_PATH=~/Library/Audio/Plug-Ins/CLAP
    LV2_PATH=~/Library/Audio/Plug-Ins/LV2
    VST2_PATH=~/Library/Audio/Plug-Ins/VST
    VST3_PATH=~/Library/Audio/Plug-Ins/VST3
else
    CLAP_PATH=~/.clap
    LV2_PATH=~/.lv2
    VST2_PATH=~/.vst
    VST3_PATH=~/.vst3
fi

export IFS=$'\n'

# NOTE macOS ignores AU plugins installed in ~/Library/Audio/Plug-Ins/Components/
# we **MUST** install AU plugins system-wide, so we need sudo/root here
# they cannot be symlinks either, so we do a full copy
for p in $(find . -maxdepth 1 -name '*.component' -print | grep '.component'); do
    basename=$(basename ${p})
    if [ ! -L /Library/Audio/Plug-Ins/Components/"${basename}" ]; then
        sudo cp -r $(pwd)/"${basename}" /Library/Audio/Plug-Ins/Components/
    fi
done

for p in $(find . -maxdepth 1 -name '*.clap' -print); do
    basename=$(basename ${p})
    mkdir -p ${CLAP_PATH}
    if [ ! -L ${CLAP_PATH}/"${basename}" ]; then
        ln -s $(pwd)/"${basename}" ${CLAP_PATH}/"${basename}"
    fi
done

for p in $(find . -maxdepth 1 -name '*.lv2' -print); do
    basename=$(basename ${p})
    mkdir -p ${LV2_PATH}
    if [ ! -L ${LV2_PATH}/"${basename}" ]; then
        ln -s $(pwd)/"${basename}" ${LV2_PATH}/"${basename}"
    fi
done

for p in $(find . -maxdepth 1 -name '*.vst' -print); do
    basename=$(basename ${p})
    mkdir -p ${VST2_PATH}
    if [ ! -L ${VST2_PATH}/"${basename}" ]; then
        ln -s $(pwd)/"${basename}" ${VST2_PATH}/"${basename}"
    fi
done

for p in $(find . -maxdepth 1 -name '*.vst3' -print); do
    basename=$(basename ${p})
    mkdir -p ${VST3_PATH}
    if [ ! -L ${VST3_PATH}/"${basename}" ]; then
        ln -s $(pwd)/"${basename}" ${VST3_PATH}/"${basename}"
    fi
done
