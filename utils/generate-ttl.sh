#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the source root folder"
  exit
fi

PWD="$(dirname "$0")"

if [ -f "$PWD/lv2_ttl_generator.exe" ]; then
  GEN="$PWD/lv2_ttl_generator.exe"
  EXT=dll
else
  GEN="$PWD/lv2_ttl_generator"
  if [ -d /Library/Audio ]; then
    EXT=dylib
  else
    EXT=so
  fi
fi

FOLDERS=`find . -type d -name \*.lv2`

for i in $FOLDERS; do
  cd $i
  FILE="$(ls *.$EXT | sort | head -n 1)"
  "$GEN" "./$FILE"
  cd ..
done
