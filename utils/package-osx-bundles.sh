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

if [ -n "${MACOS_APP_CERTIFICATE}" ] && [ -n "${MACOS_INSTALLER_CERTIFICATE}" ] && [ -n "${MACOS_CERTIFICATE_PASSWORD}" ]; then
    security create-keychain -p "" $(pwd)/keychain.db
    security unlock-keychain -p "" $(pwd)/keychain.db
    echo -n "${MACOS_APP_CERTIFICATE}" | base64 --decode -o cert.p12
    security import cert.p12 -P "${MACOS_CERTIFICATE_PASSWORD}" -A -t cert -f pkcs12 -k $(pwd)/keychain.db
    echo -n "${MACOS_INSTALLER_CERTIFICATE}" | base64 --decode -o cert.p12
    security import cert.p12 -P "${MACOS_CERTIFICATE_PASSWORD}" -A -t cert -f pkcs12 -k $(pwd)/keychain.db
    rm cert.p12
    # security set-key-partition-list -S apple-tool:,apple: -k "" $(pwd)/keychain.db
    security list-keychain -d user -s $(pwd)/keychain.db
    export MACOS_APP_DEV_ID="$(security find-identity -v $(pwd)/keychain.db | grep 'Developer ID Application:' | head -n 1 | cut -d' ' -f 5-99 | sed 's/\"//g')"
    export MACOS_INSTALLER_DEV_ID="$(security find-identity -v $(pwd)/keychain.db | grep 'Developer ID Installer:' | head -n 1 | cut -d' ' -f 5-99 | sed 's/\"//g')"
fi

# can be overridden by environment variables
MACOS_PKG_LICENSE_FILE=${MACOS_PKG_LICENSE_FILE:=""}
MACOS_PKG_NAME=${MACOS_PKG_NAME:="$(basename $(git rev-parse --show-toplevel))"}
MACOS_PKG_SNAME=${MACOS_PKG_SNAME:="$(echo ${MACOS_PKG_NAME} | tr -d ' ' | tr '/' '-')"}
MACOS_PKG_SYMBOL=${MACOS_PKG_SYMBOL:="studio.kx.distrho.plugins.${MACOS_PKG_SNAME}"}
MACOS_PKG_WELCOME_TXT=${MACOS_PKG_WELCOME_TXT:=${DPF_UTILS_DIR}/plugin.pkg/welcome.txt.in}

# backwards compat
if [ -n "${WELCOME_TXT}" ]; then
    MACOS_PKG_WELCOME_TXT="${WELCOME_TXT}"
fi

SKIP_START="<!--"
SKIP_END="-->"

if [ -n "${MACOS_INSTALLER_DEV_ID}" ]; then
    PKG_SIGN_ARGS=(--sign "${MACOS_INSTALLER_DEV_ID}")
fi

rm -rf pkg
mkdir pkg

if [ -z "${MACOS_PKG_LICENSE_FILE}" ]; then
  SKIP_LICENSE_START="${SKIP_START}"
  SKIP_LICENSE_END="${SKIP_END}"
fi

ENABLE_AU=$(find . -maxdepth 1 -name '*.component' -print -quit | grep -q '.component' && echo 1 || echo)
if [ -n "${ENABLE_AU}" ]; then
  mkdir pkg/au
  cp -RL *.component pkg/au/
  [ -n "${MACOS_APP_DEV_ID}" ] && codesign -s "${MACOS_APP_DEV_ID}" --deep --force --verbose --option=runtime pkg/au/*.component
  pkgbuild \
    --identifier "${MACOS_PKG_SYMBOL}-components" \
    --install-location "/Library/Audio/Plug-Ins/Components/" \
    --root "${PWD}/pkg/au/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${MACOS_PKG_SNAME}-components.pkg
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
    --identifier "${MACOS_PKG_SYMBOL}-clapbundles" \
    --install-location "/Library/Audio/Plug-Ins/CLAP/" \
    --root "${PWD}/pkg/clap/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${MACOS_PKG_SNAME}-clapbundles.pkg
else
  SKIP_CLAP_START="${SKIP_START}"
  SKIP_CLAP_END="${SKIP_END}"
fi

ENABLE_LV2=$(find . -maxdepth 1 -name '*.lv2' -print -quit | grep -q '.lv2' && echo 1 || echo)
if [ -n "${ENABLE_LV2}" ]; then
  mkdir pkg/lv2
  cp -RL *.lv2 pkg/lv2/
  [ -n "${MACOS_APP_DEV_ID}" ] && codesign -s "${MACOS_APP_DEV_ID}" --force --verbose --option=runtime $(find pkg/lv2/*.lv2/* -type f | grep -v "^.*\.\(html\|ttl\)$")
  pkgbuild \
    --identifier "${MACOS_PKG_SYMBOL}-lv2bundles" \
    --install-location "/Library/Audio/Plug-Ins/LV2/" \
    --root "${PWD}/pkg/lv2/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${MACOS_PKG_SNAME}-lv2bundles.pkg
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
    --identifier "${MACOS_PKG_SYMBOL}-vst2bundles" \
    --install-location "/Library/Audio/Plug-Ins/VST/" \
    --root "${PWD}/pkg/vst2/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${MACOS_PKG_SNAME}-vst2bundles.pkg
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
    --identifier "${MACOS_PKG_SYMBOL}-vst3bundles" \
    --install-location "/Library/Audio/Plug-Ins/VST3/" \
    --root "${PWD}/pkg/vst3/" \
    "${PKG_SIGN_ARGS[@]}" \
    ../dpf-${MACOS_PKG_SNAME}-vst3bundles.pkg
else
  SKIP_VST3_START="${SKIP_START}"
  SKIP_VST3_END="${SKIP_END}"
fi

cd ..

mkdir -p build
sed -e "s|@name@|${MACOS_PKG_NAME}|" "${MACOS_PKG_WELCOME_TXT}" > build/welcome.txt
sed -e "s|@builddir@|${PWD}/build|" \
    -e "s|@skip_license_start@|${SKIP_LICENSE_START}|" \
    -e "s|@skip_au_start@|${SKIP_AU_START}|" \
    -e "s|@skip_clap_start@|${SKIP_CLAP_START}|" \
    -e "s|@skip_lv2_start@|${SKIP_LV2_START}|" \
    -e "s|@skip_vst2_start@|${SKIP_VST2_START}|" \
    -e "s|@skip_vst3_start@|${SKIP_VST3_START}|" \
    -e "s|@skip_license_end@|${SKIP_LICENSE_END}|" \
    -e "s|@skip_au_end@|${SKIP_AU_END}|" \
    -e "s|@skip_clap_end@|${SKIP_CLAP_END}|" \
    -e "s|@skip_lv2_end@|${SKIP_LV2_END}|" \
    -e "s|@skip_vst2_end@|${SKIP_VST2_END}|" \
    -e "s|@skip_vst3_end@|${SKIP_VST3_END}|" \
    -e "s|@license_file@|${MACOS_PKG_LICENSE_FILE}|" \
    -e "s|@name@|${MACOS_PKG_NAME}|g" \
    -e "s|@sname@|${MACOS_PKG_SNAME}|g" \
    -e "s|@symbol@|${MACOS_PKG_SYMBOL}|g" \
    ${DPF_UTILS_DIR}/plugin.pkg/package.xml.in > build/package.xml

productbuild \
  --distribution build/package.xml \
  --identifier "${MACOS_PKG_SYMBOL}" \
  --package-path "${PWD}" \
  --version 0 \
  "${PKG_SIGN_ARGS[@]}" \
  ${MACOS_PKG_SNAME}-macOS.pkg

if [ -n "${MACOS_NOTARIZATION_USER}" ] && [ -n "${MACOS_NOTARIZATION_PASS}" ] && [ -n "${MACOS_NOTARIZATION_TEAM}" ]; then
  # Notarize using credentials
  xcrun notarytool submit ${MACOS_PKG_SNAME}-macOS.pkg \
    --apple-id ${MACOS_NOTARIZATION_USER} \
    --password ${MACOS_NOTARIZATION_PASS} \
    --team-id ${MACOS_NOTARIZATION_TEAM} \
    --wait
  xcrun stapler staple ${MACOS_PKG_SNAME}-macOS.pkg
elif [ -n "${MACOS_KEYCHAIN_PROFILE}" ]; then
  # Notarize using keychain profile
  xcrun notarytool submit ${MACOS_PKG_SNAME}-macOS.pkg \
    --keychain-profile ${MACOS_KEYCHAIN_PROFILE} \
    --wait
  xcrun stapler staple ${MACOS_PKG_SNAME}-macOS.pkg
fi

# To get logs of your notarization note the notarization id (of the form `00000000-0000-0000-0000-000000000000`) 
# and use either your credentials or keychain profile:
# xcrun notarytool log --keychain-profile ${MACOS_KEYCHAIN_PROFILE} ${NOTARIZATION_ID}
