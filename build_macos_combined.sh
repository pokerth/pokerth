#!/usr/bin/env bash

###
# Combined build script for PokerTH on macOS.
#
# Builds BOTH clients and packs them into a SINGLE DMG — the macOS pendant of
# the Windows combined installer (docker/windows/build_windows_combined_docker.sh):
#
#   PokerTH.app          → Qt Quick / QML client   (modern, primary)
#   PokerTH Classic.app  → Qt Widgets / classic client
#
# Both bundles are self-contained (own Qt frameworks via macdeployqt) and land
# in one PokerTH-Combined.dmg, so the user can drag either or both to
# /Applications from the same disk image.
#
# you need to install latest xcode and xcode commandline tools before running this script

set -euo pipefail

BUILD_DIR_NAME="build_macos_combined"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Load shared setup (Homebrew, vcpkg, Qt, helper functions)
# shellcheck source=build_macos_common.sh
source "$SCRIPT_DIR/build_macos_common.sh"

########################################
# Build both PokerTH clients
########################################

BUILD_DIR="$SCRIPT_DIR/$BUILD_DIR_NAME"

log "Configuring CMake build (Widget + QML)…"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# One configure step, both GUI targets share the compiled engine/net libraries.
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
  -DCMAKE_PREFIX_PATH="$QT_DIR" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$MACOSX_DEPLOYMENT_TARGET" \
  -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHITECTURES" \
  -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET"

log "Building pokerth_client (Widget) and pokerth_qml-client (QML)…"
ninja -C "$BUILD_DIR" pokerth_client pokerth_qml-client

########################################
# Create both App Bundles & one combined DMG
########################################

build_combined_bundles_and_dmg "$BUILD_DIR" "$SCRIPT_DIR"
