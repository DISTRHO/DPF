#!/bin/bash

# the realpath function is not available on some systems
if ! which realpath &>/dev/null; then
    function realpath() {
        [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
    }
fi

set -e

DPF_UTILS_DIR="$(dirname $(realpath ${0}))"

# can be overridden by environment variables
WELCOME_TXT=${WELCOME_TXT:=${DPF_UTILS_DIR}/plugin.pkg/welcome.txt.in}

if [ -d bin ]; then
  cd bin
elif [ -d build/bin ]; then
  cd build/bin
else
  echo "Please run this script from the root folder"
  exit
fi

NAME="$(basename $(git rev-parse --show-toplevel))"
SNAME="$(echo ${NAME} | tr -d ' ' | tr '/' '-')"

SKIP_START="<!--"
SKIP_END="-->"

if [ -n "${MACOS_INSTALLER_DEV_ID}" ]; then
    PKG_SIGN_ARGS=(--sign "${MACOS_INSTALLER_DEV_ID}")
fi

rm -rf pkg
mkdir pkg

ENABLE_AU=$(find . -maxdepth 1 -name '*.component' -print -quit | grep -q '.component' && echo 1 || echo)
if [ -n "${ENABLE_AU}" ]; then
  mkdir pkg/au
  cp -RL *.component pkg/au/
  [ -n "${MACOS_APP_DEV_ID}" ] && codesign -s "${MACOS_APP_DEV_ID}" --deep --force --verbose --option=runtime pkg/au/*.component
  pkgbuild \
    --identifier "studio.kx.distrho.plugins.${SNAME}.components" \
    --install-location "/Library/Audio/Plug-Ins/Components/" \
    --root "${PWD}/pkg/au/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${SNAME}-components.pkg
else
  SKIP_AU_START="${SKIP_START}"
  SKIP_AU_END="${SKIP_END}"
fi

ENABLE_CLAP=$(find . -maxdepth 1 -name '*.clap' -print -quit | grep -q '.clap' && echo 1 || echo)
if [ -n "${ENABLE_CLAP}" ]; then
  mkdir pkg/clap   
  cp -RL *.clap pkg/clap/
  [ -n "${MACOS_APP_DEV_ID}" ] && codesign -s "${MACOS_APP_DEV_ID}" --deep --force --verbose --option=runtime pkg/clap/*.clap
  pkgbuild \
    --identifier "studio.kx.distrho.plugins.${SNAME}.clapbundles" \
    --install-location "/Library/Audio/Plug-Ins/CLAP/" \
    --root "${PWD}/pkg/clap/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${SNAME}-clapbundles.pkg
else
  SKIP_CLAP_START="${SKIP_START}"
  SKIP_CLAP_END="${SKIP_END}"
fi

ENABLE_LV2=$(find . -maxdepth 1 -name '*.lv2' -print -quit | grep -q '.lv2' && echo 1 || echo)
if [ -n "${ENABLE_LV2}" ]; then
  mkdir pkg/lv2
  cp -RL *.lv2 pkg/lv2/
  [ -n "${MACOS_APP_DEV_ID}" ] && codesign -s "${MACOS_APP_DEV_ID}" --force --verbose --option=runtime pkg/lv2/*.lv2/*.dylib
  pkgbuild \
    --identifier "studio.kx.distrho.plugins.${SNAME}.lv2bundles" \
    --install-location "/Library/Audio/Plug-Ins/LV2/" \
    --root "${PWD}/pkg/lv2/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${SNAME}-lv2bundles.pkg
else
  SKIP_LV2_START="${SKIP_START}"
  SKIP_LV2_END="${SKIP_END}"
fi

ENABLE_VST2=$(find . -maxdepth 1 -name '*.vst' -print -quit | grep -q '.vst' && echo 1 || echo)
if [ -n "${ENABLE_VST2}" ]; then
  mkdir pkg/vst2
  cp -RL *.vst pkg/vst2/
  [ -n "${MACOS_APP_DEV_ID}" ] && codesign -s "${MACOS_APP_DEV_ID}" --deep --force --verbose --option=runtime pkg/vst2/*.vst
  pkgbuild \
    --identifier "studio.kx.distrho.plugins.${SNAME}.vst2bundles" \
    --install-location "/Library/Audio/Plug-Ins/VST/" \
    --root "${PWD}/pkg/vst2/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${SNAME}-vst2bundles.pkg
else
  SKIP_VST2_START="${SKIP_START}"
  SKIP_VST2_END="${SKIP_END}"
fi

ENABLE_VST3=$(find . -maxdepth 1 -name '*.vst3' -print -quit | grep -q '.vst3' && echo 1 || echo)
if [ -n "${ENABLE_VST3}" ]; then
  mkdir pkg/vst3
  cp -RL *.vst3 pkg/vst3/
  [ -n "${MACOS_APP_DEV_ID}" ] && codesign -s "${MACOS_APP_DEV_ID}" --deep --force --verbose --option=runtime pkg/vst3/*.vst3
  pkgbuild \
    --identifier "studio.kx.distrho.plugins.${SNAME}.vst3bundles" \
    --install-location "/Library/Audio/Plug-Ins/VST3/" \
    --root "${PWD}/pkg/vst3/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${SNAME}-vst3bundles.pkg
else
  SKIP_VST3_START="${SKIP_START}"
  SKIP_VST3_END="${SKIP_END}"
fi

cd ..

mkdir -p build
sed -e "s|@name@|${NAME}|" "${WELCOME_TXT}" > build/welcome.txt
sed -e "s|@builddir@|${PWD}/build|" \
    -e "s|@skip_au_start@|${SKIP_AU_START}|" \
    -e "s|@skip_clap_start@|${SKIP_CLAP_START}|" \
    -e "s|@skip_lv2_start@|${SKIP_LV2_START}|" \
    -e "s|@skip_vst2_start@|${SKIP_VST2_START}|" \
    -e "s|@skip_vst3_start@|${SKIP_VST3_START}|" \
    -e "s|@skip_au_end@|${SKIP_AU_END}|" \
    -e "s|@skip_clap_end@|${SKIP_CLAP_END}|" \
    -e "s|@skip_lv2_end@|${SKIP_LV2_END}|" \
    -e "s|@skip_vst2_end@|${SKIP_VST2_END}|" \
    -e "s|@skip_vst3_end@|${SKIP_VST3_END}|" \
    -e "s|@aubundleref@|dpf-${SNAME}-components.pkg|" \
    -e "s|@clapbundleref@|dpf-${SNAME}-clapbundles.pkg|" \
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
  "${PKG_SIGN_ARGS[@]}" \
  ${SNAME}-macOS.pkg

# xcrun notarytool submit build/*-macOS.pkg --keychain-profile "build-notary" --wait
# xcrun notarytool log --keychain-profile "build-notary" 00000000-0000-0000-0000-000000000000
