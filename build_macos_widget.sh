#!/usr/bin/env bash

###
# Build script for PokerTH Widget Client (Qt Widgets / classic GUI).
# you need to install latest xcode and xcode commandline tools before running this script

set -euo pipefail

BUILD_TARGET="pokerth_client"
BUILD_DIR_NAME="build_macos"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Load shared setup (Homebrew, vcpkg, Qt, helper functions)
# shellcheck source=build_macos_common.sh
source "$SCRIPT_DIR/build_macos_common.sh"

########################################
# Build PokerTH Widget Client
########################################

BUILD_DIR="$SCRIPT_DIR/$BUILD_DIR_NAME"

log "Configuring CMake build (Widget Client)…"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
  -DCMAKE_PREFIX_PATH="$QT_DIR" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
  -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
  -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET"

log "Building ${BUILD_TARGET}…"
ninja -C "$BUILD_DIR" "$BUILD_TARGET"

########################################
# Create App Bundle & DMG
########################################

# USE_QML=0 → no -qmldir flag passed to macdeployqt
build_bundle_and_dmg "$BUILD_DIR" "$BUILD_TARGET" "0" "$SCRIPT_DIR"
