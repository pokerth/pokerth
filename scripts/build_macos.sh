#!/usr/bin/env bash

###
# Build script for PokerTH on macOS
# Run make setup-macos or scripts/setup_macos.sh first to install dependencies
# You need to install latest xcode and xcode commandline tools before running this script

set -euo pipefail

# Source shared build environment (sets REPO_ROOT when in scripts/)
source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"

########################################
# macOS-specific Configuration
########################################

# Build target selection (can be overridden via environment variable)
# Options: pokerth_client, pokerth_qml-client
BUILD_TARGET="${BUILD_TARGET:-pokerth_client}"

# Reuse existing build directory by default; set to "yes" for a clean rebuild
CLEAN="${CLEAN:-no}"

# Set to "yes" to create DMG installer after building the app bundle
CREATE_INSTALLER="${CREATE_INSTALLER:-no}"

QT_DIR="$QT_OUTPUT_DIR/$QT_VERSION/macos"
MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-12.0}"

if [ $# -gt 0 ]; then
  echo "Usage: make macos or ./scripts/build_macos.sh"
  echo "  No arguments. Set environment variables to change behavior."
  echo ""
  echo "Environment: BUILD_TARGET, CLEAN, CREATE_INSTALLER"
  echo ""
  echo "Examples:"
  echo "  make macos"
  echo "  CLEAN=yes make macos"
  echo "  make macos-installer"
  echo ""
  echo "Note: Run make setup-macos or scripts/setup_macos.sh first if dependencies are missing."
  exit 0
fi

########################################
# Script-local functions (plan section 3)
########################################

create_macos_app_bundle() {
  APP_NAME="PokerTH"
  APP_BUNDLE="$BUILD_DIR/${APP_NAME}.app"
  APP_CONTENTS="$APP_BUNDLE/Contents"
  APP_MACOS="$APP_CONTENTS/MacOS"
  APP_RESOURCES="$APP_CONTENTS/Resources"

  log "Creating app bundle structure…"
  mkdir -p "$APP_MACOS"
  mkdir -p "$APP_RESOURCES"

  log "Copying binary and resources…"
  BINARY_NAME="${BUILD_TARGET//-/_}"
  [ ! -f "$BUILD_DIR/bin/$BINARY_NAME" ] && BINARY_NAME="$BUILD_TARGET"
  cp "$BUILD_DIR/bin/$BINARY_NAME" "$APP_MACOS/$APP_NAME"
  cp -r "$REPO_ROOT/data" "$APP_RESOURCES/"

  ICON_SOURCE=""
  [ -f "$REPO_ROOT/pokerth.png" ] && { ICON_SOURCE="$REPO_ROOT/pokerth.png"; log "Converting PNG to .icns…"; }
  [ -f "$REPO_ROOT/pokerth.svg" ] && [ -z "$ICON_SOURCE" ] && { ICON_SOURCE="$REPO_ROOT/pokerth.svg"; log "Converting SVG to .icns…"; }

  if [ -n "$ICON_SOURCE" ]; then
    ICONSET_DIR="$BUILD_DIR/pokerth.iconset"
    mkdir -p "$ICONSET_DIR"
    BASE_PNG="$ICON_SOURCE"
    if [[ "$ICON_SOURCE" == *.svg ]]; then
      if [ -f "$REPO_ROOT/pokerth.png" ]; then
        BASE_PNG="$REPO_ROOT/pokerth.png"
        log "Using PNG source for better transparency support"
      else
        qlmanage -t -s 1024 -o "$BUILD_DIR" "$ICON_SOURCE" >/dev/null 2>&1
        BASE_PNG="$BUILD_DIR/$(basename "$ICON_SOURCE").png"
      fi
    fi
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
    [[ "$BASE_PNG" == *"qlmanage"* ]] || [[ "$BASE_PNG" == *".svg.png" ]] && rm -f "$BASE_PNG"
    if ! iconutil -c icns "$ICONSET_DIR" -o "$APP_RESOURCES/pokerth.icns" 2>/dev/null; then
      # Fallback: ImageMagick convert works on Linux/headless (e.g. Cursor sandbox); creates single-res .icns
      if command -v convert >/dev/null 2>&1; then
        IMG_SRC=""
        [ -f "$REPO_ROOT/pokerth.png" ] && IMG_SRC="$REPO_ROOT/pokerth.png"
        [ -z "$IMG_SRC" ] && [ -f "$REPO_ROOT/pokerth.svg" ] && IMG_SRC="$REPO_ROOT/pokerth.svg"
        if [ -n "$IMG_SRC" ] && convert "$IMG_SRC" -resize 1024x1024 "$APP_RESOURCES/pokerth.icns" 2>/dev/null; then
          log "Icon: used ImageMagick (iconutil unavailable or failed)"
        elif [ -f "$REPO_ROOT/pokerth.icns" ]; then
          log "Icon: using repo pokerth.icns"
          cp "$REPO_ROOT/pokerth.icns" "$APP_RESOURCES/pokerth.icns"
        else
          log "Warning: icon conversion failed and no repo pokerth.icns; app will use default icon"
        fi
      elif [ -f "$REPO_ROOT/pokerth.icns" ]; then
        log "Icon: using repo pokerth.icns"
        cp "$REPO_ROOT/pokerth.icns" "$APP_RESOURCES/pokerth.icns"
      else
        log "Warning: icon conversion failed and no repo pokerth.icns; app will use default icon"
      fi
    fi
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
  rm -f "$APP_RESOURCES/qt.conf"
  if [[ "$BUILD_TARGET" == *"qml"* ]]; then
    QML_DIR="$REPO_ROOT/src/gui/qt6-qml"
    "$QT_DIR/bin/macdeployqt" "$APP_BUNDLE" -qmldir="$QML_DIR" -verbose=1
  else
    "$QT_DIR/bin/macdeployqt" "$APP_BUNDLE" -verbose=1
  fi

  # Ensure qt.conf is correct so plugins load from Contents/PlugIns (avoids macdeployqt warning).
  cat > "$APP_RESOURCES/qt.conf" <<'QTCONF'
[Paths]
Plugins = PlugIns
QTCONF

  if [ -n "${CODESIGN_IDENTITY:-}" ]; then
    log "Code signing with identity: $CODESIGN_IDENTITY"
    find "$APP_BUNDLE/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "Qt*" \) -exec codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime {} \;
    codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime "$APP_BUNDLE/Contents/MacOS/PokerTH"
    codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime --entitlements /dev/null "$APP_BUNDLE"
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
}

create_macos_dmg() {
  DMG_NAME="${APP_NAME}.dmg"
  DMG_PATH="$BUILD_DIR/$DMG_NAME"
  DMG_TEMP_DIR="$BUILD_DIR/dmg_temp"
  DMG_BACKGROUND_DIR="$DMG_TEMP_DIR/.background"
  TMP_DMG="$BUILD_DIR/temp.dmg"

  log "Creating DMG installer with visual layout…"
  rm -f "$DMG_PATH"
  rm -f "$TMP_DMG"
  rm -rf "$DMG_TEMP_DIR"
  mkdir -p "$DMG_TEMP_DIR"
  mkdir -p "$DMG_BACKGROUND_DIR"
  cp -R "$APP_BUNDLE" "$DMG_TEMP_DIR/"
  ln -s /Applications "$DMG_TEMP_DIR/Applications"

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
  qlmanage -t -s 500 -o "$BUILD_DIR" "$ARROW_SVG" >/dev/null 2>&1
  mv "$BUILD_DIR/dmg_background.svg.png" "$DMG_BACKGROUND_DIR/background.png" 2>/dev/null || log "Warning: Could not create background image, continuing without..."
  rm -f "$ARROW_SVG"

  hdiutil create -srcfolder "$DMG_TEMP_DIR" -volname "$APP_NAME" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -size 500m "$TMP_DMG"
  DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "$TMP_DMG" | grep '^/dev/' | sed 1q | awk '{print $1}')
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
}

print_build_summary_macos() {
  log "Build complete!"
  echo ""
  echo "✓ App Bundle: $APP_BUNDLE"
  if is_yes "${CREATE_INSTALLER:-no}"; then
    echo "✓ DMG Installer: $DMG_PATH"
    echo ""
    echo "To run: open $APP_BUNDLE"
    echo "To install: open $DMG_PATH"
  else
    echo ""
    echo "To run: open $APP_BUNDLE"
    echo "To create DMG: make macos-installer"
  fi
  echo ""
  echo "Note: Run make setup-macos or scripts/setup_macos.sh first if dependencies are missing."
  echo ""
}

########################################
# 1. Check dependencies
########################################

log "Checking build dependencies..."

check_dependency cmake "make setup-macos or scripts/setup_macos.sh"
check_dependency ninja "make setup-macos or scripts/setup_macos.sh"
check_dependency git "make setup-macos or scripts/setup_macos.sh"

# Check Qt installation
if [ ! -d "$QT_DIR" ] || [ ! -f "$QT_DIR/bin/qmake" ] || [ ! -f "$QT_DIR/bin/macdeployqt" ]; then
  error "Qt not found at $QT_DIR. Please run make setup-macos or scripts/setup_macos.sh first."
fi
log "✓ Qt: $QT_DIR"

# Check vcpkg
check_vcpkg_deps "make setup-macos or scripts/setup_macos.sh"

log "✓ All dependencies found"

########################################
# 2. Build PokerTH
########################################

BUILD_DIR="$REPO_ROOT/build_macos"

if is_yes "$CLEAN"; then
  log "Cleaning and reconfiguring build…"
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
else
  log "Reusing existing build directory…"
  mkdir -p "$BUILD_DIR"
fi
log "Configuring CMake build…"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
  -DCMAKE_PREFIX_PATH="$QT_DIR" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0

# So Cursor/clangd see the same compile flags and includes as the build
if [ -f "$BUILD_DIR/compile_commands.json" ]; then
  ln -sf "$BUILD_DIR/compile_commands.json" "$REPO_ROOT/compile_commands.json"
  log "  ✓ compile_commands.json linked for clangd/Cursor"
fi

log "Building ${BUILD_TARGET}…"
ninja -C "$BUILD_DIR" "$BUILD_TARGET"

########################################
# 3. Create macOS App Bundle
########################################

create_macos_app_bundle

########################################
# 4. Create DMG (optional)
########################################

if is_yes "${CREATE_INSTALLER:-no}"; then
  create_macos_dmg
fi

########################################
# 5. Summary
########################################

print_build_summary_macos
