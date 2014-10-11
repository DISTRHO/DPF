#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the root folder"
  exit
fi

PWD=`pwd`

if [ ! -d /System/Library ]; then
  echo "This doesn't seem to be OSX, please stop!"
  exit 0
fi

rm -rf *.vst/

PLUGINS=`ls | grep vst.dylib`

for i in $PLUGINS; do
  FILE=`echo $i | awk 'sub("-vst.dylib","")'`
  cp -r ../dpf/utils/plugin.vst/ $FILE.vst
  mv $i $FILE.vst/Contents/MacOS/$FILE
  rm -f $FILE.vst/Contents/MacOS/deleteme
  sed -i -e "s/X-PROJECTNAME-X/$FILE/" $FILE.vst/Contents/Info.plist
  rm -f $FILE.vst/Contents/Info.plist-e
  SetFile -a B $FILE.vst
done

cd ..
