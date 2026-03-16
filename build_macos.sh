#!/usr/bin/env bash

###
# you need to install latest xcode and xcode commandline tools before running this script

set -euo pipefail

########################################
# Configuration
########################################

# Build target selection (can be overridden via environment variable)
# Options: pokerth_client, pokerth_qml-client
BUILD_TARGET="${BUILD_TARGET:-pokerth_client}"

BREW_PREFIX_DEFAULT="/opt/homebrew"   # Apple Silicon
VCPKG_DIR="$HOME/vcpkg"
PYTHON_USER_BASE="$HOME/.local"
AQT_BIN="$PYTHON_USER_BASE/bin/aqt"
MACOSX_DEPLOYMENT_TARGET=12.0


########################################
# Helper functions
########################################

log() {
  echo "▶ $1"
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

########################################
# 1. Homebrew
########################################

if ! command_exists brew; then
  log "Installing Homebrew…"
  NONINTERACTIVE=1 /bin/bash -c \
    "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

  # shellenv (Apple Silicon)
  if [ -d "$BREW_PREFIX_DEFAULT" ]; then
    eval "$($BREW_PREFIX_DEFAULT/bin/brew shellenv)"
  fi
else
  log "Homebrew already installed"
fi

########################################
# 2. Base packages
########################################

log "Installing base packages via Homebrew…"
brew install cmake python pkg-config ninja || true
brew pin cmake python pkg-config ninja || true
# Git nur installieren, falls nicht vorhanden, und sofort pinnen
if ! brew list git &>/dev/null; then
  brew install --ignore-dependencies git || true
  brew pin git || true
else
  brew pin git || true
fi

########################################
# 3. pipx
########################################

if ! command_exists pipx; then
  log "Installing pipx…"
  brew install pipx
  pipx ensurepath
else
  log "pipx already installed"
fi

########################################
# 4. aqtinstall (via pipx)
########################################

if ! command_exists aqt; then
  log "Installing aqtinstall via pipx…"
  pipx install aqtinstall
else
  log "aqtinstall already installed"
fi

export PATH="$HOME/.local/bin:$PATH"

########################################
# 5. vcpkg
########################################

if [ ! -d "$VCPKG_DIR" ]; then
  log "Cloning vcpkg…"
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
else
  log "vcpkg directory already exists"
fi

log "Bootstrapping vcpkg…"
"$VCPKG_DIR/bootstrap-vcpkg.sh"


########################################
# vcpkg dependencies
########################################

brew install \
brew install cmake python pkg-config ninja autoconf autoconf-archive automake libtool || true
brew pin cmake python pkg-config ninja autoconf autoconf-archive automake libtool || true
# Git nur installieren, falls nicht vorhanden, und sofort pinnen
if ! brew list git &>/dev/null; then
  brew install --ignore-dependencies git || true
  brew pin git || true
else
  brew pin git || true
fi

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
  boost-system
  boost-thread
  boost-serialization
  boost-smart-ptr
  protobuf
)
# Note: Main 'boost' package removed - it would include all submodules including boost-cobalt
# Only essential modules are installed instead
# Note: curl removed - using Qt Network instead


# Architektur bestimmen
if [[ "$(uname -m)" == "arm64" ]]; then
  VCPKG_TRIPLET="arm64-osx"
else
  VCPKG_TRIPLET="x64-osx"
fi

log "Installing vcpkg dependencies (${VCPKG_TRIPLET})…"
"$HOME/vcpkg/vcpkg" install \
  --triplet="$VCPKG_TRIPLET" \
  "${VCPKG_PORTS[@]}"

########################################
# Qt installation (aqtinstall)
########################################

QT_VERSION="6.9.2"
QT_OUTPUT_DIR="$HOME/Qt"
QT_DIR="$QT_OUTPUT_DIR/$QT_VERSION/macos"

# Check if Qt is already installed
if [ -d "$QT_DIR" ] && [ -f "$QT_DIR/bin/qmake" ] && [ -f "$QT_DIR/bin/macdeployqt" ]; then
  log "Qt ${QT_VERSION} already installed at: $QT_DIR"
else
  log "Installing Qt ${QT_VERSION} for macOS (clang_64) with modules…"

  QT_MODULES=(
    qt3d
    qt5compat
    qtcharts
    qtconnectivity
    qtdatavis3d
    qtgraphs
    qtgrpc
    qthttpserver
    qtimageformats
    qtlocation
    qtlottie
    qtmultimedia
    qtnetworkauth
    qtpositioning
    qtquick3d
    qtquick3dphysics
    qtquicktimeline
    qtremoteobjects
    qtscxml
    qtsensors
    qtserialbus
    qtserialport
    qtshadertools
    qtspeech
    qtvirtualkeyboard
    qtwebchannel
    qtwebsockets
    qtwebview
  )

  aqt install-qt mac desktop "$QT_VERSION" clang_64 \
    --outputdir "$QT_OUTPUT_DIR" \
    --modules "${QT_MODULES[@]}"

  log "Qt installed at: $QT_DIR"
fi

########################################
# Build PokerTH
########################################

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build_macos"

log "Configuring CMake build…"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
  -DCMAKE_PREFIX_PATH="$QT_DIR" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0

log "Building ${BUILD_TARGET}…"
ninja -C "$BUILD_DIR" "$BUILD_TARGET"

########################################
# Create macOS App Bundle
########################################

APP_NAME="PokerTH"
APP_BUNDLE="$BUILD_DIR/${APP_NAME}.app"
APP_CONTENTS="$APP_BUNDLE/Contents"
APP_MACOS="$APP_CONTENTS/MacOS"
APP_RESOURCES="$APP_CONTENTS/Resources"

log "Creating app bundle structure…"
mkdir -p "$APP_MACOS"
mkdir -p "$APP_RESOURCES"

log "Copying binary and resources…"
# Convert build target to binary name (remove hyphens for binary name)
BINARY_NAME="${BUILD_TARGET//-/_}"
if [ ! -f "$BUILD_DIR/bin/$BINARY_NAME" ]; then
    # Try with hyphens
    BINARY_NAME="$BUILD_TARGET"
fi
cp "$BUILD_DIR/bin/$BINARY_NAME" "$APP_MACOS/$APP_NAME"
cp -r "$SCRIPT_DIR/data" "$APP_RESOURCES/"

# Create app icon from PNG (preferred for transparency) or SVG
ICON_SOURCE=""
if [ -f "$SCRIPT_DIR/pokerth.png" ]; then
    ICON_SOURCE="$SCRIPT_DIR/pokerth.png"
    log "Converting PNG to .icns…"
elif [ -f "$SCRIPT_DIR/pokerth.svg" ]; then
    ICON_SOURCE="$SCRIPT_DIR/pokerth.svg"
    log "Converting SVG to .icns…"
fi

if [ -n "$ICON_SOURCE" ]; then
    ICONSET_DIR="$BUILD_DIR/pokerth.iconset"
    mkdir -p "$ICONSET_DIR"

    # Use PNG directly with sips (works for both SVG and PNG sources)
    # For best results with transparency, use PNG source
    BASE_PNG="$ICON_SOURCE"

    # If SVG, convert to PNG first using qlmanage (best we have without librsvg)
    if [[ "$ICON_SOURCE" == *.svg ]]; then
        # Try to use PNG instead if available
        if [ -f "$SCRIPT_DIR/pokerth.png" ]; then
            BASE_PNG="$SCRIPT_DIR/pokerth.png"
            log "Using PNG source for better transparency support"
        else
            qlmanage -t -s 1024 -o "$BUILD_DIR" "$ICON_SOURCE" >/dev/null 2>&1
            BASE_PNG="$BUILD_DIR/$(basename "$ICON_SOURCE").png"
        fi
    fi

    # Generate different sizes from the base PNG
    sips -z 16 16     "$BASE_PNG" --out "$ICONSET_DIR/icon_16x16.png" >/dev/null 2>&1
    sips -z 32 32     "$BASE_PNG" --out "$ICONSET_DIR/icon_16x16@2x.png" >/dev/null 2>&1
    sips -z 32 32     "$BASE_PNG" --out "$ICONSET_DIR/icon_32x32.png" >/dev/null 2>&1
    sips -z 64 64     "$BASE_PNG" --out "$ICONSET_DIR/icon_32x32@2x.png" >/dev/null 2>&1
    sips -z 128 128   "$BASE_PNG" --out "$ICONSET_DIR/icon_128x128.png" >/dev/null 2>&1
    sips -z 256 256   "$BASE_PNG" --out "$ICONSET_DIR/icon_128x128@2x.png" >/dev/null 2>&1
    sips -z 256 256   "$BASE_PNG" --out "$ICONSET_DIR/icon_256x256.png" >/dev/null 2>&1
    sips -z 512 512   "$BASE_PNG" --out "$ICONSET_DIR/icon_256x256@2x.png" >/dev/null 2>&1
    sips -z 512 512   "$BASE_PNG" --out "$ICONSET_DIR/icon_512x512.png" >/dev/null 2>&1
    sips -z 1024 1024 "$BASE_PNG" --out "$ICONSET_DIR/icon_512x512@2x.png" >/dev/null 2>&1

    if [[ "$BASE_PNG" == *"qlmanage"* ]] || [[ "$BASE_PNG" == *".svg.png" ]]; then
        rm -f "$BASE_PNG"
    fi

    # Create icns from iconset    # Convert to icns
    iconutil -c icns "$ICONSET_DIR" -o "$APP_RESOURCES/pokerth.icns"
    rm -rf "$ICONSET_DIR"
fi

log "Creating Info.plist…"
cat > "$APP_CONTENTS/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>net.pokerth.PokerTH</string>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundleDisplayName</key>
    <string>$APP_NAME</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>2.0.6</string>
    <key>CFBundleVersion</key>
    <string>2.0.6</string>
    <key>CFBundleIconFile</key>
    <string>pokerth.icns</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
EOF

log "Deploying Qt frameworks with macdeployqt…"
# For QML apps, specify the QML source directory
if [[ "$BUILD_TARGET" == *"qml"* ]]; then
    QML_DIR="$SCRIPT_DIR/src/gui/qt6-qml"
    "$QT_DIR/bin/macdeployqt" "$APP_BUNDLE" -qmldir="$QML_DIR" -verbose=1
else
    "$QT_DIR/bin/macdeployqt" "$APP_BUNDLE" -verbose=1
fi

########################################
# Code Signing (Optional)
########################################

# Check if CODESIGN_IDENTITY environment variable is set
# Usage: export CODESIGN_IDENTITY="Developer ID Application: Your Name (TEAM_ID)"
if [ -n "${CODESIGN_IDENTITY:-}" ]; then
    log "Code signing with identity: $CODESIGN_IDENTITY"

    # Sign all frameworks and dylibs first
    find "$APP_BUNDLE/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "Qt*" \) -exec codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime {} \;

    # Sign the main executable
    codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime "$APP_BUNDLE/Contents/MacOS/PokerTH"

    # Sign the app bundle
    codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime --entitlements /dev/null "$APP_BUNDLE"

    # Verify signature
    codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"

    log "Code signing complete!"
else
    log "Ad-hoc signing app bundle (no Developer ID identity)"
    codesign --force --deep --sign - "$APP_BUNDLE"
    xattr -dr com.apple.quarantine "$APP_BUNDLE" || true
    codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"
    log "Ad-hoc signing complete."
    echo "  To enable Developer ID signing, set: export CODESIGN_IDENTITY=\"Developer ID Application: Your Name (TEAM_ID)\""
fi

########################################
# Create DMG with visual layout
########################################

DMG_NAME="${APP_NAME}.dmg"
DMG_PATH="$BUILD_DIR/$DMG_NAME"
DMG_TEMP_DIR="$BUILD_DIR/dmg_temp"
DMG_BACKGROUND_DIR="$DMG_TEMP_DIR/.background"

log "Creating DMG installer with visual layout…"
rm -f "$DMG_PATH"
rm -rf "$DMG_TEMP_DIR"
mkdir -p "$DMG_TEMP_DIR"
mkdir -p "$DMG_BACKGROUND_DIR"

# Copy app bundle to temp DMG directory
cp -R "$APP_BUNDLE" "$DMG_TEMP_DIR/"

# Create symlink to Applications
ln -s /Applications "$DMG_TEMP_DIR/Applications"

# Create background image with arrow
log "Creating DMG background image…"
ARROW_SVG="$BUILD_DIR/dmg_background.svg"
cat > "$ARROW_SVG" <<'ARROW_EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="500" height="300" xmlns="http://www.w3.org/2000/svg">
  <!-- Light background -->
  <rect width="500" height="300" fill="#f5f5f5"/>

  <!-- Arrow from app icon area to Applications area -->
  <defs>
    <marker id="arrowhead" markerWidth="8" markerHeight="8"
            refX="7" refY="4" orient="auto">
      <path d="M 0 0 L 8 4 L 0 8 z" fill="#999"/>
    </marker>
  </defs>

  <!-- Arrow line (from right of app ~180 to left of Applications ~340) -->
  <line x1="200" y1="150" x2="300" y2="150"
        stroke="#999" stroke-width="2"
        marker-end="url(#arrowhead)"/>
</svg>
ARROW_EOF

# Convert SVG to PNG using qlmanage (built-in macOS tool)
qlmanage -t -s 500 -o "$BUILD_DIR" "$ARROW_SVG" >/dev/null 2>&1
mv "$BUILD_DIR/dmg_background.svg.png" "$DMG_BACKGROUND_DIR/background.png" 2>/dev/null || {
    log "Warning: Could not create background image, continuing without..."
}
rm -f "$ARROW_SVG"


# Create temporary RW DMG
TMP_DMG="$BUILD_DIR/temp.dmg"
hdiutil create -srcfolder "$DMG_TEMP_DIR" -volname "$APP_NAME" -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" -format UDRW -size 500m "$TMP_DMG"

# Mount it
DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "$TMP_DMG" | \
         grep '^/dev/' | sed 1q | awk '{print $1}')

sleep 2

# Set window appearance with AppleScript
osascript <<DMG_SCRIPT
   tell application "Finder"
     tell disk "$APP_NAME"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {100, 100, 600, 450}
           set viewOptions to the icon view options of container window
           set arrangement of viewOptions to not arranged
           set icon size of viewOptions to 128
           set background picture of viewOptions to file ".background:background.png"
           delay 1
           set position of item "$APP_NAME.app" of container window to {120, 150}
           set position of item "Applications" of container window to {380, 150}
           close
           open
           update without registering applications
           delay 2
     end tell
   end tell
DMG_SCRIPT

# Sync and unmount
sync
hdiutil detach "$DEVICE"

# Convert to compressed final DMG
rm -f "$DMG_PATH"
hdiutil convert "$TMP_DMG" -format UDZO -imagekey zlib-level=9 -o "$DMG_PATH"
rm -f "$TMP_DMG"

# Clean up temp directory
rm -rf "$DMG_TEMP_DIR"

########################################
# Summary
########################################

log "Build complete!"
echo ""
echo "✓ App Bundle: $APP_BUNDLE"
echo "✓ DMG Installer: $DMG_PATH"
echo ""
echo "To run: open $APP_BUNDLE"
echo "To install: open $DMG_PATH"
########################################


