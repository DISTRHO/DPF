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
rm -rf vst3

mkdir lv2 vst2 vst3
cp -RL *.lv2 lv2/
cp -RL *.vst vst2/
cp -RL *.vst3 vst3/
rm -rf *.lv2 *.vst *.vst3

pkgbuild \
  --identifier "studio.kx.distrho.plugins.${SNAME}.lv2bundles" \
  --install-location "/Library/Audio/Plug-Ins/LV2/" \
  --root "${PWD}/lv2/" \
  ../dpf-${SNAME}-lv2bundles.pkg

pkgbuild \
  --identifier "studio.kx.distrho.plugins.${SNAME}.vst2bundles" \
  --install-location "/Library/Audio/Plug-Ins/VST/" \
  --root "${PWD}/vst2/" \
  ../dpf-${SNAME}-vst2bundles.pkg

pkgbuild \
  --identifier "studio.kx.distrho.plugins.${SNAME}.vst3bundles" \
  --install-location "/Library/Audio/Plug-Ins/VST3/" \
  --root "${PWD}/vst3/" \
  ../dpf-${SNAME}-vst3bundles.pkg

cd ..

DPF_UTILS_DIR=$(dirname ${0})

sed -e "s|@name@|${NAME}|" ${DPF_UTILS_DIR}/plugin.pkg/welcome.txt.in > build/welcome.txt
sed -e "s|@builddir@|${PWD}/build|" \
    -e "s|@lv2bundleref@|dpf-${SNAME}-lv2bundles.pkg|" \
    -e "s|@vst2bundleref@|dpf-${SNAME}-vst2bundles.pkg|" \
    -e "s|@vst3bundleref@|dpf-${SNAME}-vst3bundles.pkg|" \
    -e "s|@name@|${NAME}|g" \
    -e "s|@sname@|${SNAME}|g" \
    ${DPF_UTILS_DIR}/plugin.pkg/package.xml.in > build/package.xml

productbuild \
  --distribution build/package.xml \
  --identifier "studio.kx.distrho.${SNAME}" \
  --package-path "${PWD}" \
  --version 0 \
  ${SNAME}-macOS.pkg
