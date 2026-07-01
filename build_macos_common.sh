#!/usr/bin/env bash

###
# Shared setup for PokerTH macOS builds.
# Source this file from build_macos_client.sh or build_macos_qml.sh.
# Do NOT execute directly.

# Guard against double-sourcing
[[ -n "${_POKERTH_BUILD_COMMON_LOADED:-}" ]] && return 0
_POKERTH_BUILD_COMMON_LOADED=1

########################################
# Configuration
########################################

BREW_PREFIX_DEFAULT="/opt/homebrew"   # Apple Silicon
VCPKG_DIR="$HOME/vcpkg"
PYTHON_USER_BASE="$HOME/.local"
AQT_BIN="$PYTHON_USER_BASE/bin/aqt"
MACOSX_DEPLOYMENT_TARGET=12.0

QT_VERSION="6.9.2"
QT_OUTPUT_DIR="$HOME/Qt"
QT_DIR="$QT_OUTPUT_DIR/$QT_VERSION/macos"

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
brew install cmake python pkg-config ninja autoconf autoconf-archive automake libtool || true
brew pin cmake python pkg-config ninja autoconf autoconf-archive automake libtool || true
# Install git only if missing, then pin it
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
# 6. vcpkg dependencies
########################################

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
  boost-uuid
  protobuf
)
# Note: Main 'boost' package removed - it would include all submodules including boost-cobalt
# Only essential modules are installed instead
# Note: curl removed - using Qt Network instead

# Determine architecture
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
# 7. Qt installation (aqtinstall)
########################################

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
# Shared bundle/DMG helpers
########################################

# build_bundle <BUILD_DIR> <BUILD_TARGET> <BINARY_NAME> <USE_QML 0|1>
# Creates the .app bundle, code-signs it, and produces a DMG.
build_bundle_and_dmg() {
  local BUILD_DIR="$1"
  local BUILD_TARGET="$2"
  local USE_QML="$3"      # 1 = QML client, 0 = widget client
  local SCRIPT_DIR="$4"

  local APP_NAME="PokerTH"
  local APP_BUNDLE="$BUILD_DIR/${APP_NAME}.app"
  local APP_CONTENTS="$APP_BUNDLE/Contents"
  local APP_MACOS="$APP_CONTENTS/MacOS"
  local APP_RESOURCES="$APP_CONTENTS/Resources"

  log "Creating app bundle structure…"
  mkdir -p "$APP_MACOS"
  mkdir -p "$APP_RESOURCES"

  log "Copying binary and resources…"
  # Convert build target to binary name (replace hyphens with underscores)
  local BINARY_NAME="${BUILD_TARGET//-/_}"
  if [ ! -f "$BUILD_DIR/bin/$BINARY_NAME" ]; then
      BINARY_NAME="$BUILD_TARGET"
  fi
  cp "$BUILD_DIR/bin/$BINARY_NAME" "$APP_MACOS/$APP_NAME"
  cp -r "$SCRIPT_DIR/data" "$APP_RESOURCES/"

  # Create app icon from PNG (preferred for transparency) or SVG
  local ICON_SOURCE=""
  if [ -f "$SCRIPT_DIR/pokerth.png" ]; then
      ICON_SOURCE="$SCRIPT_DIR/pokerth.png"
      log "Converting PNG to .icns…"
  elif [ -f "$SCRIPT_DIR/pokerth.svg" ]; then
      ICON_SOURCE="$SCRIPT_DIR/pokerth.svg"
      log "Converting SVG to .icns…"
  fi

  if [ -n "$ICON_SOURCE" ]; then
      local ICONSET_DIR="$BUILD_DIR/pokerth.iconset"
      mkdir -p "$ICONSET_DIR"

      local BASE_PNG="$ICON_SOURCE"

      if [[ "$ICON_SOURCE" == *.svg ]]; then
          if [ -f "$SCRIPT_DIR/pokerth.png" ]; then
              BASE_PNG="$SCRIPT_DIR/pokerth.png"
              log "Using PNG source for better transparency support"
          else
              qlmanage -t -s 1024 -o "$BUILD_DIR" "$ICON_SOURCE" >/dev/null 2>&1
              BASE_PNG="$BUILD_DIR/$(basename "$ICON_SOURCE").png"
          fi
      fi

      sips -z 16 16     "$BASE_PNG" --out "$ICONSET_DIR/icon_16x16.png"    >/dev/null 2>&1
      sips -z 32 32     "$BASE_PNG" --out "$ICONSET_DIR/icon_16x16@2x.png" >/dev/null 2>&1
      sips -z 32 32     "$BASE_PNG" --out "$ICONSET_DIR/icon_32x32.png"    >/dev/null 2>&1
      sips -z 64 64     "$BASE_PNG" --out "$ICONSET_DIR/icon_32x32@2x.png" >/dev/null 2>&1
      sips -z 128 128   "$BASE_PNG" --out "$ICONSET_DIR/icon_128x128.png"  >/dev/null 2>&1
      sips -z 256 256   "$BASE_PNG" --out "$ICONSET_DIR/icon_128x128@2x.png" >/dev/null 2>&1
      sips -z 256 256   "$BASE_PNG" --out "$ICONSET_DIR/icon_256x256.png"  >/dev/null 2>&1
      sips -z 512 512   "$BASE_PNG" --out "$ICONSET_DIR/icon_256x256@2x.png" >/dev/null 2>&1
      sips -z 512 512   "$BASE_PNG" --out "$ICONSET_DIR/icon_512x512.png"  >/dev/null 2>&1
      sips -z 1024 1024 "$BASE_PNG" --out "$ICONSET_DIR/icon_512x512@2x.png" >/dev/null 2>&1

      if [[ "$BASE_PNG" == *"qlmanage"* ]] || [[ "$BASE_PNG" == *".svg.png" ]]; then
          rm -f "$BASE_PNG"
      fi

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
    <string>2.1.0</string>
    <key>CFBundleVersion</key>
    <string>2.1.0</string>
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
  if [[ "$USE_QML" == "1" ]]; then
      "$QT_DIR/bin/macdeployqt" "$APP_BUNDLE" \
          -qmldir="$SCRIPT_DIR/src/gui/qt6-qml" -verbose=1
  else
      "$QT_DIR/bin/macdeployqt" "$APP_BUNDLE" -verbose=1
  fi

  ########################################
  # Code Signing
  ########################################

  if [ -n "${CODESIGN_IDENTITY:-}" ]; then
      log "Code signing with identity: $CODESIGN_IDENTITY"
      find "$APP_BUNDLE/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "Qt*" \) \
          -exec codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime {} \;
      codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime \
          "$APP_BUNDLE/Contents/MacOS/PokerTH"
      codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime \
          --entitlements /dev/null "$APP_BUNDLE"
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
  # DMG with visual layout
  ########################################

  local DMG_NAME="${APP_NAME}.dmg"
  local DMG_PATH="$BUILD_DIR/$DMG_NAME"
  local DMG_TEMP_DIR="$BUILD_DIR/dmg_temp"
  local DMG_BACKGROUND_DIR="$DMG_TEMP_DIR/.background"

  log "Creating DMG installer with visual layout…"
  rm -f "$DMG_PATH"
  rm -rf "$DMG_TEMP_DIR"
  mkdir -p "$DMG_TEMP_DIR"
  mkdir -p "$DMG_BACKGROUND_DIR"

  cp -R "$APP_BUNDLE" "$DMG_TEMP_DIR/"
  ln -s /Applications "$DMG_TEMP_DIR/Applications"

  log "Creating DMG background image…"
  local ARROW_SVG="$BUILD_DIR/dmg_background.svg"
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

  qlmanage -t -s 500 -o "$BUILD_DIR" "$ARROW_SVG" >/dev/null 2>&1
  mv "$BUILD_DIR/dmg_background.svg.png" "$DMG_BACKGROUND_DIR/background.png" 2>/dev/null || {
      log "Warning: Could not create background image, continuing without..."
  }
  rm -f "$ARROW_SVG"

  local TMP_DMG="$BUILD_DIR/temp.dmg"
  hdiutil create -srcfolder "$DMG_TEMP_DIR" -volname "$APP_NAME" -fs HFS+ \
        -fsargs "-c c=64,a=16,e=16" -format UDRW -size 500m "$TMP_DMG"

  local DEVICE
  DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "$TMP_DMG" | \
           grep '^/dev/' | sed 1q | awk '{print $1}')

  sleep 2

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

  sync
  hdiutil detach "$DEVICE"

  rm -f "$DMG_PATH"
  hdiutil convert "$TMP_DMG" -format UDZO -imagekey zlib-level=9 -o "$DMG_PATH"
  rm -f "$TMP_DMG"
  rm -rf "$DMG_TEMP_DIR"

  log "Build complete!"
  echo ""
  echo "✓ App Bundle: $APP_BUNDLE"
  echo "✓ DMG Installer: $DMG_PATH"
  echo ""
  echo "To run: open $APP_BUNDLE"
  echo "To install: open $DMG_PATH"
}
