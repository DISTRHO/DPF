#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the root folder"
  exit
fi

DPF_DIR=$(dirname $0)/..
PLUGINS=$(ls | grep vst.dylib)

rm -rf *.vst/

for i in $PLUGINS; do
  BUNDLE=$(echo ${i} | awk 'sub("-vst.dylib","")')
  cp -r ${DPF_DIR}/utils/plugin.vst/ ${BUNDLE}.vst
  mv ${i} ${BUNDLE}.vst/Contents/MacOS/${BUNDLE}
  rm -f ${BUNDLE}.vst/Contents/MacOS/deleteme
  sed -i -e "s/X-PROJECTNAME-X/${BUNDLE}/" ${BUNDLE}.vst/Contents/Info.plist
  rm -f ${BUNDLE}.vst/Contents/Info.plist-e
done

cd ..
