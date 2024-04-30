#!/bin/bash

# the realpath function is not available on some systems
if ! which realpath &>/dev/null; then
    function realpath() {
        [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
    }
fi

set -e

BIN_DIR=${1}

if [ -z "${BIN_DIR}" ]; then
  BIN_DIR=bin
fi

if [ ! -d ${BIN_DIR} ]; then
  echo "Please run this script from the source root folder"
  exit
fi

PWD="$(dirname "${0}")"

if [ -f "${PWD}/lv2_ttl_generator.exe" ]; then
  GEN="$(realpath ${PWD}/lv2_ttl_generator.exe)"
  EXT=dll
else
  GEN="$(realpath ${PWD}/lv2_ttl_generator)"
  if [ -d /Library/Audio ]; then
    EXT=dylib
  else
    EXT=so
  fi
fi

cd ${BIN_DIR}
FOLDERS=`find . -type d -name \*.lv2`

for i in ${FOLDERS}; do
  cd ${i}
  FILE="$(ls *.${EXT} 2>/dev/null | sort | head -n 1)"
  if [ -n "${FILE}" ]; then
    ${EXE_WRAPPER} "${GEN}" "./${FILE}"
  fi
  cd ..
done
