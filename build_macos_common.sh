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
# Deliberate floor: at least one player runs macOS 12 (Monterey) and cannot
# update. Exported, so that everything built here — the vcpkg ports as well as
# PokerTH itself — uses the same target; without the export the ports silently
# take the build machine's SDK default and their objects end up newer than the
# binary that links them. Keep this the single place where the target is set.
export MACOSX_DEPLOYMENT_TARGET=12.0

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

# pokerth_version <SCRIPT_DIR>
# Release version of the sources that are about to be packaged, read from the
# single source of truth src/game_defs.h. Never hardcode it here: a version
# written into this script only describes the script, not the checkout it
# packages, so a stale working copy ships silently under the new name.
pokerth_version() {
  local DEFS="$1/src/game_defs.h"
  local VERSION
  VERSION=$(sed -n 's/^#define[[:space:]]*POKERTH_BETA_RELEASE_STRING[[:space:]]*"\([^"]*\)".*/\1/p' "$DEFS")
  if [ -z "$VERSION" ]; then
      echo "Error: cannot read POKERTH_BETA_RELEASE_STRING from $DEFS" >&2
      return 1
  fi
  echo "$VERSION"
}

# merge_universal_libs <BASE_TRIPLET> <OTHER_TRIPLET> <OUT_TRIPLET>
# Combines two per-architecture vcpkg trees into installed/<OUT_TRIPLET>, which
# is the tree the vcpkg toolchain later hands to CMake: headers, pkg-config and
# CMake config files are architecture independent and simply come from the base
# tree, every static library is replaced by a fat one.
# The base tree must be the one for the host, so that the tools it contains
# (protoc) can still be executed during the build.
merge_universal_libs() {
  local BASE_TREE="$VCPKG_DIR/installed/$1"
  local OTHER_TREE="$VCPKG_DIR/installed/$2"
  local OUT="$VCPKG_DIR/installed/$3"
  local LIB REL COUNTERPART
  local MERGED=0 SKIPPED=0

  log "Merging $1 + $2 into $3…"
  rm -rf "$OUT"
  cp -R "$BASE_TREE" "$OUT"

  while IFS= read -r -d '' LIB; do
      REL="${LIB#"$OUT/"}"
      COUNTERPART="$OTHER_TREE/$REL"
      if [ ! -f "$COUNTERPART" ]; then
          log "  Warning: no counterpart for $REL — stays single-architecture"
          SKIPPED=$((SKIPPED + 1))
          continue
      fi
      lipo -create "$LIB" "$COUNTERPART" -output "$LIB.universal"
      mv "$LIB.universal" "$LIB"
      MERGED=$((MERGED + 1))
  done < <(find "$OUT" -type f \( -name '*.a' -o -name '*.dylib' \) -print0)

  if [ "$MERGED" -eq 0 ]; then
      echo "Error: no library could be merged — is $OTHER_TREE missing?" >&2
      return 1
  fi
  log "  $MERGED libraries are universal now ($SKIPPED skipped)"
}

# log_source_state <SCRIPT_DIR>
# Prints version and git state of the tree being packaged, so an outdated
# checkout is visible in the build log instead of only in the finished bundle.
log_source_state() {
  local SCRIPT_DIR="$1"
  local VERSION
  VERSION=$(pokerth_version "$SCRIPT_DIR")
  log "Packaging PokerTH $VERSION from $SCRIPT_DIR"
  if git -C "$SCRIPT_DIR" rev-parse --git-dir >/dev/null 2>&1; then
      local DESCRIBE BRANCH COMMIT_DATE
      DESCRIBE=$(git -C "$SCRIPT_DIR" describe --tags --always --dirty)
      BRANCH=$(git -C "$SCRIPT_DIR" rev-parse --abbrev-ref HEAD)
      COMMIT_DATE=$(git -C "$SCRIPT_DIR" log -1 --format=%cd --date=short)
      log "  git: $DESCRIBE on $BRANCH, last commit $COMMIT_DATE"
  else
      log "  git: not a repository — cannot verify that the sources are up to date"
  fi
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
  openssl
)
# Note: Main 'boost' package removed - it would include all submodules including boost-cobalt
# Only essential modules are installed instead
# Note: curl removed - using Qt Network instead
# Note: openssl (needed by the built-in server via boost::asio::ssl) comes from
# vcpkg rather than Homebrew: Homebrew formulae are single-architecture, so
# linking them would make a universal binary impossible.

# Universal build: one DMG that runs natively on Intel and on Apple Silicon.
# The libraries must therefore carry both slices — a host-dependent triplet
# (arm64-osx / x64-osx) would quietly restrict a release to the architecture of
# whichever machine happened to build it.
#
# Built one architecture at a time and merged afterwards, because a fat vcpkg
# pass fails: boost-context (pulled in by boost-asio) picks its hand-written
# assembly once, from the host's CMAKE_SYSTEM_PROCESSOR, and then compiles it
# for both slices — the foreign slice cannot be assembled.
OSX_ARCHITECTURES="x86_64;arm64"
VCPKG_TRIPLET="universal-osx"
VCPKG_OVERLAY_TRIPLETS="${SCRIPT_DIR:?SCRIPT_DIR must be set before sourcing this file}/cmake/vcpkg-triplets"

# The host's triplet first: its tree becomes the base of the merge, so that the
# tools in it (protoc) run on this machine.
if [[ "$(uname -m)" == "arm64" ]]; then
  VCPKG_ARCH_TRIPLETS=(arm64-osx-pokerth x64-osx-pokerth)
else
  VCPKG_ARCH_TRIPLETS=(x64-osx-pokerth arm64-osx-pokerth)
fi

for TRIPLET in "${VCPKG_ARCH_TRIPLETS[@]}"; do
  log "Installing vcpkg dependencies (${TRIPLET})…"
  "$VCPKG_DIR/vcpkg" install \
    --triplet="$TRIPLET" \
    --overlay-triplets="$VCPKG_OVERLAY_TRIPLETS" \
    "${VCPKG_PORTS[@]}"
done

merge_universal_libs "${VCPKG_ARCH_TRIPLETS[0]}" "${VCPKG_ARCH_TRIPLETS[1]}" "$VCPKG_TRIPLET"

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

# _make_icns <SCRIPT_DIR> <BUILD_DIR> <OUT_ICNS>
# Generates an .icns from pokerth.png (preferred) or pokerth.svg.
# No-op (returns 0) when no source image is available.
_make_icns() {
  local SCRIPT_DIR="$1"
  local BUILD_DIR="$2"
  local OUT_ICNS="$3"

  local ICON_SOURCE=""
  if [ -f "$SCRIPT_DIR/pokerth.png" ]; then
      ICON_SOURCE="$SCRIPT_DIR/pokerth.png"
      log "Converting PNG to .icns…"
  elif [ -f "$SCRIPT_DIR/pokerth.svg" ]; then
      ICON_SOURCE="$SCRIPT_DIR/pokerth.svg"
      log "Converting SVG to .icns…"
  else
      return 0
  fi

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

  iconutil -c icns "$ICONSET_DIR" -o "$OUT_ICNS"
  rm -rf "$ICONSET_DIR"
}

# build_app_bundle <BUILD_DIR> <BUILD_TARGET> <USE_QML 0|1> <SCRIPT_DIR> <APP_NAME> <BUNDLE_ID>
# Creates "$BUILD_DIR/$APP_NAME.app", deploys Qt frameworks and code-signs it.
# Multiple bundles can be created side by side in the same BUILD_DIR.
build_app_bundle() {
  local BUILD_DIR="$1"
  local BUILD_TARGET="$2"
  local USE_QML="$3"      # 1 = QML client, 0 = widget client
  local SCRIPT_DIR="$4"
  local APP_NAME="${5:-PokerTH}"
  local BUNDLE_ID="${6:-net.pokerth.PokerTH}"

  local APP_BUNDLE="$BUILD_DIR/${APP_NAME}.app"
  local APP_CONTENTS="$APP_BUNDLE/Contents"
  local APP_MACOS="$APP_CONTENTS/MacOS"
  local APP_RESOURCES="$APP_CONTENTS/Resources"
  local VERSION
  VERSION=$(pokerth_version "$SCRIPT_DIR")

  log "Creating app bundle structure ($APP_NAME.app)…"
  rm -rf "$APP_BUNDLE"
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

  _make_icns "$SCRIPT_DIR" "$BUILD_DIR" "$APP_RESOURCES/pokerth.icns"

  log "Creating Info.plist…"
  cat > "$APP_CONTENTS/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>$APP_NAME</string>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundleDisplayName</key>
    <string>$APP_NAME</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundleVersion</key>
    <string>$VERSION</string>
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
      local SIGN=(codesign --force --sign "$CODESIGN_IDENTITY" --timestamp --options runtime)
      local ENTITLEMENTS="$SCRIPT_DIR/pokerth_macos.entitlements"
      local MACHO FRAMEWORK

      # Signed inside out. Notarization rejects a bundle containing anything
      # unsigned or ad-hoc signed, and signing the bundle last is what seals the
      # nested signatures: the Qt plug-ins under PlugIns/ and the QML modules
      # under Resources/ have to be covered too, not just Frameworks/.
      while IFS= read -r -d '' MACHO; do
          "${SIGN[@]}" "$MACHO"
      done < <(find "$APP_CONTENTS" -type f -name "*.dylib" -not -path "*/*.framework/*" -print0)

      # Frameworks are signed as bundles, not as their inner binary.
      while IFS= read -r -d '' FRAMEWORK; do
          "${SIGN[@]}" "$FRAMEWORK"
      done < <(find "$APP_CONTENTS/Frameworks" -maxdepth 1 -type d -name "*.framework" -print0 2>/dev/null)

      "${SIGN[@]}" "$APP_MACOS/$APP_NAME"
      # The hardened runtime needs the JIT entitlements for the QML engine.
      "${SIGN[@]}" --entitlements "$ENTITLEMENTS" "$APP_BUNDLE"
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

# create_dmg <BUILD_DIR> <VOLUME_NAME> <DMG_PATH> <APP_BUNDLE>...
# Assembles a DMG containing one or more .app bundles plus an /Applications
# symlink, with a visual drag-to-install layout.
create_dmg() {
  local BUILD_DIR="$1"
  local VOLUME_NAME="$2"
  local DMG_PATH="$3"
  shift 3
  local APP_BUNDLES=("$@")

  local DMG_TEMP_DIR="$BUILD_DIR/dmg_temp"
  local DMG_BACKGROUND_DIR="$DMG_TEMP_DIR/.background"

  log "Creating DMG installer with visual layout…"
  rm -f "$DMG_PATH"
  rm -rf "$DMG_TEMP_DIR"
  mkdir -p "$DMG_TEMP_DIR"
  mkdir -p "$DMG_BACKGROUND_DIR"

  local APP_BUNDLE
  for APP_BUNDLE in "${APP_BUNDLES[@]}"; do
      cp -R "$APP_BUNDLE" "$DMG_TEMP_DIR/"
  done
  ln -s /Applications "$DMG_TEMP_DIR/Applications"

  # ── Layout geometry ──────────────────────────────────────────────────────
  # Apps stacked vertically on the left, Applications centred on the right.
  local n=${#APP_BUNDLES[@]}
  local APP_X=130 APPS_X=390
  local FIRST_Y=140 ROW_STEP=150
  local APPS_Y=$(( FIRST_Y + (n - 1) * ROW_STEP / 2 ))
  local WIN_LEFT=100 WIN_TOP=100
  local WIN_RIGHT=$(( WIN_LEFT + 500 ))
  local WIN_BOTTOM=$(( WIN_TOP + FIRST_Y + (n - 1) * ROW_STEP + 160 ))

  # Build the per-item "set position" statements for the AppleScript.
  local POS_STATEMENTS=""
  local i=0
  for APP_BUNDLE in "${APP_BUNDLES[@]}"; do
      local APP_BASENAME
      APP_BASENAME="$(basename "$APP_BUNDLE")"
      local Y=$(( FIRST_Y + i * ROW_STEP ))
      POS_STATEMENTS+="           set position of item \"$APP_BASENAME\" of container window to {$APP_X, $Y}"$'\n'
      i=$(( i + 1 ))
  done
  POS_STATEMENTS+="           set position of item \"Applications\" of container window to {$APPS_X, $APPS_Y}"

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

  # Size the read/write image from the payload plus a safety margin so that
  # multiple (large) Qt bundles always fit.
  local PAYLOAD_MB
  PAYLOAD_MB=$(du -sm "$DMG_TEMP_DIR" | awk '{print $1}')
  local DMG_SIZE_MB=$(( PAYLOAD_MB + 150 ))
  [ "$DMG_SIZE_MB" -lt 500 ] && DMG_SIZE_MB=500

  local TMP_DMG="$BUILD_DIR/temp.dmg"
  rm -f "$TMP_DMG"
  hdiutil create -srcfolder "$DMG_TEMP_DIR" -volname "$VOLUME_NAME" -fs HFS+ \
        -fsargs "-c c=64,a=16,e=16" -format UDRW -size "${DMG_SIZE_MB}m" "$TMP_DMG"

  local DEVICE
  DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "$TMP_DMG" | \
           grep '^/dev/' | sed 1q | awk '{print $1}')

  sleep 2

  # Purely cosmetic (icon placement and background). Scripting the Finder needs
  # a GUI session, which a CI runner does not have — never let it fail the build.
  osascript <<DMG_SCRIPT || log "Warning: Finder layout not applied (no GUI session?) — DMG stays functional"
   tell application "Finder"
     tell disk "$VOLUME_NAME"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {$WIN_LEFT, $WIN_TOP, $WIN_RIGHT, $WIN_BOTTOM}
           set viewOptions to the icon view options of container window
           set arrangement of viewOptions to not arranged
           set icon size of viewOptions to 128
           set background picture of viewOptions to file ".background:background.png"
           delay 1
$POS_STATEMENTS
           close
           open
           update without registering applications
           delay 2
           -- Leave no window open: as long as the Finder shows the volume it
           -- holds a reference to it and the eject below fails as "busy".
           close
     end tell
   end tell
DMG_SCRIPT

  sync

  # Even after closing the window the volume can stay busy for a moment (Finder
  # writing .DS_Store, Spotlight indexing). Retry before resorting to force.
  local TRIES=0
  until hdiutil detach "$DEVICE" >/dev/null 2>&1; do
      TRIES=$((TRIES + 1))
      if [ "$TRIES" -ge 15 ]; then
          log "Warning: volume still busy after $TRIES attempts — forcing eject"
          hdiutil detach "$DEVICE" -force || true
          sleep 2
          break
      fi
      sleep 2
  done

  # Converting a still-mounted image would silently produce a damaged DMG.
  if [ -e "$DEVICE" ]; then
      echo "Error: $DEVICE is still attached — not converting a mounted image" >&2
      return 1
  fi

  rm -f "$DMG_PATH"
  hdiutil convert "$TMP_DMG" -format UDZO -imagekey zlib-level=9 -o "$DMG_PATH"
  rm -f "$TMP_DMG"
  rm -rf "$DMG_TEMP_DIR"
}

# build_bundle_and_dmg <BUILD_DIR> <BUILD_TARGET> <USE_QML 0|1> <SCRIPT_DIR>
# Backwards-compatible single-client wrapper: one PokerTH.app in one PokerTH.dmg.
build_bundle_and_dmg() {
  local BUILD_DIR="$1"
  local BUILD_TARGET="$2"
  local USE_QML="$3"
  local SCRIPT_DIR="$4"

  local APP_NAME="PokerTH"
  local APP_BUNDLE="$BUILD_DIR/${APP_NAME}.app"
  local VERSION
  VERSION=$(pokerth_version "$SCRIPT_DIR")
  # Version in the file name: the DMG is uploaded under it, so it must come
  # from the packaged sources rather than being typed in at upload time.
  local DMG_PATH="$BUILD_DIR/${APP_NAME}-${VERSION}.dmg"

  log_source_state "$SCRIPT_DIR"

  build_app_bundle "$BUILD_DIR" "$BUILD_TARGET" "$USE_QML" "$SCRIPT_DIR" \
      "$APP_NAME" "net.pokerth.PokerTH"
  create_dmg "$BUILD_DIR" "$APP_NAME" "$DMG_PATH" "$APP_BUNDLE"

  log "Build complete!"
  echo ""
  echo "✓ App Bundle: $APP_BUNDLE"
  echo "✓ DMG Installer: $DMG_PATH"
  echo ""
  echo "To run: open $APP_BUNDLE"
  echo "To install: open $DMG_PATH"
}

# build_combined_bundles_and_dmg <BUILD_DIR> <SCRIPT_DIR>
# Builds BOTH clients into two app bundles and packs them into a single DMG,
# mirroring the Windows combined installer (Widget + QML in one package).
#   PokerTH.app          → QML client   (net.pokerth.PokerTH)
#   PokerTH Classic.app  → widget client (net.pokerth.PokerTH.Classic)
build_combined_bundles_and_dmg() {
  local BUILD_DIR="$1"
  local SCRIPT_DIR="$2"

  local QML_APP="$BUILD_DIR/PokerTH.app"
  local WIDGET_APP="$BUILD_DIR/PokerTH Classic.app"
  local VERSION
  VERSION=$(pokerth_version "$SCRIPT_DIR")
  # Named exactly as the release artifact, so file name and bundle version can
  # no longer drift apart (they did for 2.1.4: a 2.1.3 build was uploaded as
  # PokerTH-2.1.4-Combined.dmg).
  local DMG_PATH="$BUILD_DIR/PokerTH-${VERSION}-Combined.dmg"

  log_source_state "$SCRIPT_DIR"

  # QML = modern/primary client, Widget = classic client.
  build_app_bundle "$BUILD_DIR" "pokerth_qml-client" "1" "$SCRIPT_DIR" \
      "PokerTH" "net.pokerth.PokerTH"
  build_app_bundle "$BUILD_DIR" "pokerth_client" "0" "$SCRIPT_DIR" \
      "PokerTH Classic" "net.pokerth.PokerTH.Classic"

  create_dmg "$BUILD_DIR" "PokerTH" "$DMG_PATH" "$QML_APP" "$WIDGET_APP"

  log "Build complete!"
  echo ""
  echo "✓ QML App Bundle:    $QML_APP"
  echo "✓ Widget App Bundle: $WIDGET_APP"
  echo "✓ Combined DMG:      $DMG_PATH"
  echo ""
  echo "The DMG contains BOTH clients — drag either (or both) to Applications."
  echo "To install: open $DMG_PATH"
}
