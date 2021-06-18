#!/bin/bash

# function not available on some systems
if ! which realpath &>/dev/null; then
    function realpath() {
        [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
    }
fi

set -e

if [ ! -d bin ]; then
  echo "Please run this script from the source root folder"
  exit
fi

PWD="$(dirname "${0}")"

if [ -f "${PWD}/lv2_ttl_generator.exe" ]; then
  GEN="${PWD}/lv2_ttl_generator.exe"
  EXT=dll
else
  GEN="$(realpath ${PWD}/lv2_ttl_generator)"
  if [ -d /Library/Audio ]; then
    EXT=dylib
  else
    EXT=so
  fi
fi

cd bin
FOLDERS=`find . -type d -name \*.lv2`

for i in ${FOLDERS}; do
  cd ${i}
  FILE="$(ls *.${EXT} | sort | head -n 1)"
  ${EXE_WRAPPER} "${GEN}" "./${FILE}"
  cd ..
done
