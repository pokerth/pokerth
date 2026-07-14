#!/usr/bin/env bash

###
# Build script for the PokerTH QML client for iOS (arm64 device) and packaging
# of an UNSIGNED .ipa for side-loading (Sideloadly / AltStore / SideStore re-sign
# the app with your own Apple ID on install).
#
# Runs on a Mac with Xcode + command line tools installed; the same script is used
# by .github/workflows/ios.yml on a macOS runner. Every install step is idempotent,
# so a cached ~/Qt and ~/vcpkg turn the setup into a no-op.
#
#   bash build_ios_qml.sh
#
# Environment overrides:
#   QT_VERSION      Qt version to use                     (default 6.9.2)
#   QT_OUTPUT_DIR   Qt install prefix                     (default $HOME/Qt)
#   VCPKG_DIR       vcpkg checkout                        (default $HOME/vcpkg)
#   BUILD_TYPE      Release | Debug                       (default Release)
#   BUILD_DIR       CMake build dir                       (default ./build_ios)

set -euo pipefail

BUILD_TARGET="pokerth_qml-client"
APP_NAME="PokerTH"                     # OUTPUT_NAME of the target -> PokerTH.app

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

QT_VERSION="${QT_VERSION:-6.9.2}"
QT_OUTPUT_DIR="${QT_OUTPUT_DIR:-$HOME/Qt}"
QT_IOS_DIR="$QT_OUTPUT_DIR/$QT_VERSION/ios"
QT_HOST_DIR="$QT_OUTPUT_DIR/$QT_VERSION/macos"   # host tools: moc/rcc/lupdate/qmlimportscanner

VCPKG_DIR="${VCPKG_DIR:-$HOME/vcpkg}"
VCPKG_TRIPLET="arm64-ios"
if [[ "$(uname -m)" == "arm64" ]]; then
  VCPKG_HOST_TRIPLET="arm64-osx"
else
  VCPKG_HOST_TRIPLET="x64-osx"
fi

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build_ios}"

log() {
  echo "▶ $1"
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

########################################
# 1. Base tools
########################################

if ! command_exists xcodebuild; then
  echo "ERROR: Xcode not found. Install Xcode and run 'sudo xcode-select -s /Applications/Xcode.app'." >&2
  exit 1
fi

if ! command_exists brew; then
  log "Installing Homebrew…"
  NONINTERACTIVE=1 /bin/bash -c \
    "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  [ -d /opt/homebrew ] && eval "$(/opt/homebrew/bin/brew shellenv)"
fi

log "Installing base packages via Homebrew…"
brew install cmake pkg-config autoconf autoconf-archive automake libtool || true

if ! command_exists aqt; then
  log "Installing aqtinstall…"
  brew install pipx || true
  pipx install aqtinstall
fi
export PATH="$HOME/.local/bin:$PATH"

########################################
# 2. Qt for iOS (+ matching host Qt)
########################################

# Modules mirror the Android/macOS builds. Everything else the client needs
# (Core, Gui, Widgets, Quick, QuickControls2, Qml, Sql, Xml, Svg, Network,
# LinguistTools) ships with the Qt base package.
QT_MODULES=(
  qt5compat
  qtimageformats
  qtmultimedia
  qtshadertools
  qtwebsockets
)

if [ -f "$QT_IOS_DIR/lib/cmake/Qt6/qt.toolchain.cmake" ] && [ -x "$QT_HOST_DIR/bin/qmlimportscanner" ]; then
  log "Qt ${QT_VERSION} for iOS already installed at: $QT_IOS_DIR"
else
  log "Installing Qt ${QT_VERSION} for iOS (+ host Qt via --autodesktop)…"
  # --autodesktop pulls the matching macOS desktop Qt; Qt's iOS toolchain needs it
  # as QT_HOST_PATH for the code generators that must run on the build machine.
  aqt install-qt mac ios "$QT_VERSION" ios \
    --outputdir "$QT_OUTPUT_DIR" \
    --autodesktop \
    --modules "${QT_MODULES[@]}"
fi

########################################
# 3. vcpkg dependencies (cross-compiled for iOS)
########################################

# Boost, OpenSSL and protobuf are not shipped by Qt and Homebrew only builds for
# macOS — vcpkg's arm64-ios triplet cross-compiles them against the iPhoneOS SDK.
# protoc itself is built for the host triplet automatically (vcpkg host dependency).
declare -a VCPKG_PORTS=(
  boost-any
  boost-asio
  boost-atomic
  boost-chrono
  boost-container
  boost-date-time
  boost-filesystem
  boost-foreach
  boost-interprocess
  boost-iostreams
  boost-lambda
  boost-program-options
  boost-random
  boost-serialization
  boost-smart-ptr
  boost-system
  boost-thread
  boost-uuid
  openssl
  protobuf
)

if [ ! -d "$VCPKG_DIR" ]; then
  log "Cloning vcpkg…"
  git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi
if [ ! -x "$VCPKG_DIR/vcpkg" ]; then
  log "Bootstrapping vcpkg…"
  "$VCPKG_DIR/bootstrap-vcpkg.sh" -disableMetrics
fi

log "Installing vcpkg dependencies (${VCPKG_TRIPLET}, host ${VCPKG_HOST_TRIPLET})…"
# --clean-after-build drops buildtrees/packages of each port once it is installed:
# without it the vcpkg tree grows to several GB (boost) and no longer fits into a
# GitHub Actions cache entry.
"$VCPKG_DIR/vcpkg" install \
  --triplet="$VCPKG_TRIPLET" \
  --host-triplet="$VCPKG_HOST_TRIPLET" \
  --clean-after-build \
  "${VCPKG_PORTS[@]}"

########################################
# 4. Configure
########################################

log "Configuring CMake build (iOS / Xcode generator)…"
rm -rf "$BUILD_DIR"

# The Xcode generator is required for iOS: only it produces a real .app bundle,
# compiles the asset catalog (app icon) and the launch storyboard.
# Toolchain layering: vcpkg's toolchain is the entry point and chain-loads Qt's,
# which sets CMAKE_SYSTEM_NAME=iOS, the iPhoneOS sysroot and arm64 — the same
# pattern the Android build uses (vcpkg + NDK toolchain).
# Code signing is switched off; the .ipa is signed on side-load.
"$QT_IOS_DIR/bin/qt-cmake" -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$QT_IOS_DIR/lib/cmake/Qt6/qt.toolchain.cmake" \
  -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET" \
  -DVCPKG_HOST_TRIPLET="$VCPKG_HOST_TRIPLET" \
  -DQT_HOST_PATH="$QT_HOST_DIR" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED=NO \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="" \
  -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO

########################################
# 5. Build
########################################

log "Building ${BUILD_TARGET} (${BUILD_TYPE})…"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" --target "$BUILD_TARGET" \
  -- -parallelizeTargets -jobs "$(sysctl -n hw.ncpu)"

########################################
# 6. Package .ipa
########################################

APP_BUNDLE="$BUILD_DIR/bin/${BUILD_TYPE}-iphoneos/${APP_NAME}.app"
if [ ! -d "$APP_BUNDLE" ]; then
  # Fallback in case CMAKE_RUNTIME_OUTPUT_DIRECTORY ever changes.
  APP_BUNDLE="$(find "$BUILD_DIR" -type d -name "${APP_NAME}.app" -print -quit)"
fi
if [ ! -d "$APP_BUNDLE" ]; then
  echo "ERROR: ${APP_NAME}.app not found below $BUILD_DIR" >&2
  exit 1
fi
log "App bundle: $APP_BUNDLE"

# An .ipa is just a ZIP with the .app inside a Payload/ directory.
IPA_DIR="$BUILD_DIR/ipa"
IPA_FILE="${IPA_OUT:-$BUILD_DIR/${APP_NAME}-ios-arm64-unsigned.ipa}"
# zip runs from inside IPA_DIR, so the target must be absolute.
case "$IPA_FILE" in /*) ;; *) IPA_FILE="$PWD/$IPA_FILE" ;; esac
rm -rf "$IPA_DIR" "$IPA_FILE"
mkdir -p "$IPA_DIR/Payload"
cp -R "$APP_BUNDLE" "$IPA_DIR/Payload/"
(cd "$IPA_DIR" && zip -qry "$IPA_FILE" Payload)

echo ""
echo "======================================"
echo "Unsigned IPA created:"
ls -lh "$IPA_FILE"
echo ""
echo "Side-load it with Sideloadly, AltStore or SideStore — those re-sign the app"
echo "with your own Apple ID. A free Apple ID gives a 7-day signature."
echo "======================================"
