#!/bin/bash

set -e

# --------------------------------------------------------------------------------------------------------------------------------
# extract debs and pack them

function compressFolderAsZip() {
rm -f "$1.zip"
zip -X -r "$1" "$1"
rm -r "$1"
}

# --------------------------------------------------------------------------------------------------------------------------------

if [ "$1" == "" ]; then
echo Missing argument
exit
fi

# --------------------------------------------------------------------------------------------------------------------------------

cd bin

mkdir -p tmp
rm -rf tmp/*

NAME="$1"

_mingw32-build make -C .. clean
_mingw32-build make -C ..
for i in `ls *-vst.dll`; do mv $i `echo $i | awk 'sub("-vst","")'`; done
rm -rf *ladspa* *dssi* *vst*
mkdir -p "$NAME-win32bit"
mv *.dll *.lv2/ "$NAME-win32bit"
cp "../dpf/utils/README-DPF-Windows.txt" "$NAME-win32bit/README.txt"
compressFolderAsZip "$NAME-win32bit"
rm -rf tmp/*

_mingw64-build make -C .. clean
_mingw64-build make -C ..
for i in `ls *-vst.dll`; do mv $i `echo $i | awk 'sub("-vst","")'`; done
rm -rf *ladspa* *dssi* *vst*
mkdir -p "$NAME-win64bit"
mv *.dll *.lv2/ "$NAME-win64bit"
cp "../dpf/utils/README-DPF-Windows.txt" "$NAME-win64bit/README.txt"
compressFolderAsZip "$NAME-win64bit"
rm -rf tmp/*

_mingw64-build make -C .. clean

cd ..

# --------------------------------------------------------------------------------------------------------------------------------
