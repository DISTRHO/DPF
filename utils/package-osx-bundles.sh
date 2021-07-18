#!/bin/bash

set -e

if [ -d bin ]; then
  cd bin
else
  echo "Please run this script from the root folder"
  exit
fi

NAME="$(basename $(git rev-parse --show-toplevel))"
SNAME="$(echo ${NAME} | tr -d ' ' | tr '/' '-')"

rm -rf lv2
rm -rf vst2

mkdir lv2 vst2
mv *.lv2 lv2/
mv *.vst vst2/

pkgbuild \
  --identifier "studio.kx.distrho.plugins.${SNAME}.lv2bundles" \
  --install-location "/Library/Audio/Plug-Ins/LV2/" \
  --root "${PWD}/lv2/" \
  ../dpf-${sname}-lv2bundles.pkg

pkgbuild \
  --identifier "studio.kx.distrho.plugins.${SNAME}.vst2bundles" \
  --install-location "/Library/Audio/Plug-Ins/VST/" \
  --root "${PWD}/vst2/" \
  ../dpf-${sname}-vst2bundles.pkg

cd ..

sed -e "s|@name@|${NAME}|" utils/plugin.pkg/welcome.txt.in > build/welcome.txt
sed -e "s|@builddir@|${PWD}/build|" \
    -e "s|@lv2bundleref@|dpf-${sname}-lv2bundles.pkg|" \
    -e "s|@vst2bundleref@|dpf-${sname}-vst2bundles.pkg|" \
    -e "s|@sname@|${SNAME}|g" \
    utils/plugin.pkg/package.xml.in > build/package.xml

productbuild \
  --distribution build/package.xml \
  --identifier "studio.kx.distrho.plugins.${SNAME}" \
  --package-path "${PWD}" \
  ${SNAME}-macOS.pkg
