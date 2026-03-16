#!/usr/bin/env bash

###
# Build script for PokerTH on Linux
# Supports native Linux builds and Windows cross-compilation
# Run make setup-linux / make setup-windows or scripts/setup.sh first to install dependencies

set -euo pipefail

# Source shared build environment (sets REPO_ROOT when in scripts/)
source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"

########################################
# Linux-specific Configuration
########################################

detect_target_platform_linux "$0"

show_usage() {
  echo "Usage: $BUILD_SCRIPT"
  echo "  No arguments. Set environment variables to change behavior."
  echo ""
  echo "Environment: TARGET_PLATFORM, BUILD_TARGET, USE_AQT, USE_VCPKG, CLEAN, CREATE_INSTALLER"
  echo ""
  echo "Examples:"
  echo "  $BUILD_SCRIPT"
  echo "  CREATE_INSTALLER=yes $BUILD_SCRIPT"
  echo "  CLEAN=yes $BUILD_SCRIPT"
  echo ""
  echo "Note: Run $SETUP_SCRIPT first if dependencies are missing."
}
if [ $# -gt 0 ]; then
  show_usage
  exit 0
fi

# Build target selection (can be overridden via environment variable)
# Options: pokerth_client, pokerth_qml-client, pokerth_dedicated_server, pokerth_official_server, pokerth_chatcleaner
BUILD_TARGET="${BUILD_TARGET:-pokerth_client}"

# Use system packages by default, set to "yes" to use aqtinstall for Qt
# For Windows cross-compilation, aqtinstall is required
USE_AQT="${USE_AQT:-no}"
USE_VCPKG="${USE_VCPKG:-no}"

# Reuse existing build directory by default; set to "yes" for a clean rebuild
CLEAN="${CLEAN:-no}"

# Optional: CREATE_INSTALLER=yes creates Linux AppImage (linuxdeployqt) or Windows NSIS installer (makensis; apt install nsis)

# Setup Qt paths based on target platform
setup_linux_paths "$TARGET_PLATFORM" "$USE_AQT" "$USE_VCPKG"

########################################
# Script-local functions (plan section 1)
########################################

configure_cmake_windows() {
  log "Configuring CMake build for Windows..."
  log "Building with:"
  log "  Qt Windows: ${QT_WINDOWS_DIR}"
  log "  Qt Host: ${QT_HOST_PATH}"
  log "  Qt6_DIR: ${Qt6_DIR}"
  log "  vcpkg: ${VCPKG_DIR}"
  log "  Toolchain: ${CMAKE_TOOLCHAIN_FILE}"
  log "  Target triplet: ${VCPKG_TARGET_TRIPLET}"
  if command_exists qt-cmake; then
    CMAKE_CMD="qt-cmake"
  elif [ -f "$QT_HOST_PATH/bin/qt-cmake" ]; then
    CMAKE_CMD="$QT_HOST_PATH/bin/qt-cmake"
  else
    error "qt-cmake not found. Required for Windows cross-compilation."
  fi
  VCPKG_OPENSSL_ROOT="$VCPKG_DIR/installed/$VCPKG_TARGET_TRIPLET"
  VCPKG_PROTOC="$VCPKG_DIR/installed/$VCPKG_TARGET_TRIPLET/tools/protobuf/protoc.exe"
  if [ -f "$VCPKG_PROTOC" ]; then
    PROTOC_EXECUTABLE="$VCPKG_PROTOC"
    log "Using Windows protoc: $PROTOC_EXECUTABLE"
  else
    PROTOC_EXECUTABLE=$(find "$VCPKG_DIR/installed/$VCPKG_TARGET_TRIPLET" -name "protoc.exe" -type f 2>/dev/null | head -1)
    [ -n "$PROTOC_EXECUTABLE" ] && log "Using Windows protoc: $PROTOC_EXECUTABLE" || log "⚠ Warning: Windows protoc not found, CMake will try to find it"
  fi
  CXX_FLAGS="-fpermissive -Wno-error"
  CMAKE_ARGS=(
    -S "$REPO_ROOT"
    -B "$BUILD_DIR"
    -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE"
    -DVCPKG_TARGET_TRIPLET="$VCPKG_TARGET_TRIPLET"
    -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"
    -DQt6_DIR="$Qt6_DIR"
    -DQT_HOST_PATH="$QT_HOST_PATH"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_CXX_STANDARD=17
    -DCMAKE_CXX_FLAGS="$CXX_FLAGS"
    -DCMAKE_C_FLAGS=""
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres
    -DCMAKE_SYSTEM_NAME=Windows
    -DCMAKE_SYSROOT="${MINGW_DIR}"
    -DCMAKE_SYSTEM_PREFIX_PATH="${MINGW_DIR}"
    -DCMAKE_FIND_ROOT_PATH="$VCPKG_DIR/installed/$VCPKG_TARGET_TRIPLET;${MINGW_DIR}"
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH
    -DOPENSSL_ROOT_DIR="$VCPKG_OPENSSL_ROOT"
    -DQT_NO_DEPLOY=ON
    -DQT_DEPLOY_SUPPORT=OFF
  )
  [ -n "$PROTOC_EXECUTABLE" ] && CMAKE_ARGS+=(-DProtobuf_PROTOC_EXECUTABLE="$PROTOC_EXECUTABLE")
  $CMAKE_CMD "${CMAKE_ARGS[@]}"
  # So Cursor/clangd see the same compile flags and includes as the build
  if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    ln -sf "$BUILD_DIR/compile_commands.json" "$REPO_ROOT/compile_commands.json"
    log "  ✓ compile_commands.json linked for clangd/Cursor"
  fi
}

configure_cmake_linux() {
  log "Configuring CMake build for Linux..."
  CMAKE_ARGS=(
    -S "$REPO_ROOT"
    -B "$BUILD_DIR"
    -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG"
  )
  if is_yes "$USE_AQT"; then
    CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="$QT_DIR")
    CMAKE_ARGS+=(-DQt6_DIR="$Qt6_DIR")
  fi
  if is_yes "$USE_VCPKG"; then
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE")
  fi
  cmake "${CMAKE_ARGS[@]}"
  # So Cursor/clangd see the same compile flags and includes as the build
  if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    ln -sf "$BUILD_DIR/compile_commands.json" "$REPO_ROOT/compile_commands.json"
    log "  ✓ compile_commands.json linked for clangd/Cursor"
  fi
}

create_windows_deploy_dir() {
  log "Preparing Windows deployment directory..."
  DEPLOY_DIR="$BUILD_DIR/deploy"
  rm -rf "$DEPLOY_DIR"
  mkdir -p "$DEPLOY_DIR"
  BINARY_NAME="${BUILD_TARGET//-/_}.exe"
  [ ! -f "$BUILD_DIR/bin/$BINARY_NAME" ] && BINARY_NAME="$BUILD_TARGET.exe"
  if [ -f "$BUILD_DIR/bin/$BINARY_NAME" ]; then
    cp "$BUILD_DIR/bin/$BINARY_NAME" "$DEPLOY_DIR/"
    log "  ✓ Copied $BINARY_NAME"
  else
    error "Executable not found: $BUILD_DIR/bin/$BINARY_NAME"
  fi
  if [ -d "$REPO_ROOT/data" ]; then
    cp -rL "$REPO_ROOT/data" "$DEPLOY_DIR/" 2>/dev/null || cp -r "$REPO_ROOT/data" "$DEPLOY_DIR/"
    log "  ✓ Data directory copied"
  else
    log "  ⚠ Warning: data directory not found"
  fi
  log "Copying Qt DLLs..."
  for dll in Qt6Core Qt6Gui Qt6Widgets Qt6Network Qt6Sql Qt6Xml Qt6Multimedia; do
    [ -f "${QT_WINDOWS_DIR}/bin/${dll}.dll" ] && cp "${QT_WINDOWS_DIR}/bin/${dll}.dll" "$DEPLOY_DIR/" 2>/dev/null && log "  ✓ ${dll}.dll" || true
  done
  # Prefer system MinGW runtime DLLs (same toolchain that linked the exe).
  # Qt's DLLs may be from a different GCC version and cause "procedure entry point ... could not be found".
  log "Copying MinGW runtime DLLs..."
  for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    copied=
    for path in /usr/lib/gcc/x86_64-w64-mingw32 /usr/x86_64-w64-mingw32/lib; do
      [ ! -d "$path" ] && continue
      dll_path=$(find "$path" -name "$dll" 2>/dev/null | head -1)
      if [ -n "$dll_path" ] && [ -f "$dll_path" ]; then
        cp "$dll_path" "$DEPLOY_DIR/" && log "  ✓ $dll (from system toolchain)" && copied=1 && break
      fi
    done
    if [ -z "$copied" ] && [ -f "${QT_WINDOWS_DIR}/bin/${dll}" ]; then
      cp "${QT_WINDOWS_DIR}/bin/${dll}" "$DEPLOY_DIR/" && log "  ✓ $dll (from Qt, fallback)" || true
    fi
  done
  log "Copying Qt plugins..."
  mkdir -p "$DEPLOY_DIR/plugins/platforms"
  [ -d "${QT_WINDOWS_DIR}/plugins/platforms" ] && cp "${QT_WINDOWS_DIR}/plugins/platforms"/*.dll "$DEPLOY_DIR/plugins/platforms/" 2>/dev/null && log "  ✓ Platform plugins" || true
  cat > "$DEPLOY_DIR/qt.conf" << 'EOF'
[Paths]
Plugins = plugins
EOF
  log "  ✓ qt.conf created"
  log "Setting executable bit on DLLs..."
  find "$DEPLOY_DIR" -name "*.dll" -exec chmod +x {} \;
  log "  ✓ DLLs marked executable"
  log "Windows deployment directory ready: $DEPLOY_DIR"
  log "  → Ready for native Windows execution"
  log "  → Ready for NSIS installer (installer.nsi expects ../../build_windows/deploy)"
}

create_windows_nsis_installer() {
  NSIS_DIR="$REPO_ROOT/docker/windows"
  [ ! -f "$NSIS_DIR/installer.nsi" ] && { log "  ✗ installer.nsi not found at $NSIS_DIR (required for CREATE_INSTALLER=yes)"; exit 1; }
  command_exists makensis || { log "  ✗ makensis not found (required for CREATE_INSTALLER=yes). Install with: apt install nsis"; exit 1; }
  log "Creating Windows installer with NSIS..."
  if [ ! -f "$NSIS_DIR/pokerth.ico" ]; then
    if [ ! -f "$REPO_ROOT/pokerth.svg" ]; then
      error "pokerth.ico not found in $NSIS_DIR and $REPO_ROOT/pokerth.svg missing. Copy an existing .ico to $NSIS_DIR or add pokerth.svg."
    fi
    if ! command_exists convert; then
      error "pokerth.ico not found and convert (ImageMagick) not available. Install ImageMagick (apt install imagemagick) or copy an existing .ico to $NSIS_DIR."
    fi
    log "  Creating pokerth.ico from pokerth.svg..."
    ICON_CREATED=
    # Prefer rsvg-convert + convert (avoids ImageMagick's broken SVG delegate on many systems)
    if command_exists rsvg-convert; then
      TMP_PNG="${TMPDIR:-/tmp}/pokerth_ico_$$.png"
      if rsvg-convert -w 256 -h 256 -o "$TMP_PNG" "$REPO_ROOT/pokerth.svg" 2>/dev/null && \
         convert -background none "$TMP_PNG" -define icon:auto-resize=256,128,64,48,32,16 "$NSIS_DIR/pokerth.ico" 2>/dev/null; then
        ICON_CREATED=1
      fi
      rm -f "$TMP_PNG"
    fi
    # Fallback: direct SVG->ICO via ImageMagick (works when RSVG delegate is configured correctly)
    if [ -z "${ICON_CREATED:-}" ] && convert -background none -density 256 "$REPO_ROOT/pokerth.svg" \
      -define icon:auto-resize=256,128,64,48,32,16 "$NSIS_DIR/pokerth.ico" 2>/dev/null; then
      ICON_CREATED=1
    fi
    if [ -z "${ICON_CREATED:-}" ]; then
      error "Failed to create pokerth.ico from pokerth.svg. Install librsvg (apt install librsvg2-bin) and ImageMagick (apt install imagemagick), or copy an existing .ico to $NSIS_DIR."
    fi
    log "  ✓ Icon created"
  fi
  [ -f "$NSIS_DIR/pokerth.ico" ] && cp "$NSIS_DIR/pokerth.ico" "$DEPLOY_DIR/" 2>/dev/null || true
  if ! (cd "$NSIS_DIR" && makensis -NOCD installer.nsi); then
    error "NSIS failed (see output above). Ensure pokerth.ico exists in $NSIS_DIR."
  fi
  INSTALLER_SRC=$(find "$NSIS_DIR" -maxdepth 1 -name "PokerTH-*-Setup.exe" -type f -printf "%T@ %p\n" 2>/dev/null | sort -n | tail -1 | cut -d' ' -f2-)
  if [ -n "$INSTALLER_SRC" ]; then
    cp "$INSTALLER_SRC" "$BUILD_DIR/"
    INSTALLER="$BUILD_DIR/$(basename "$INSTALLER_SRC")"
    log "  ✓ Installer created: $INSTALLER"
  else
    log "  ✓ NSIS completed (check $NSIS_DIR for .exe)"
  fi
}

create_linux_deploy_dir() {
  log "Preparing Linux deployment directory..."
  BINARY_NAME="${BUILD_TARGET//-/_}"
  [ ! -f "$BUILD_DIR/bin/$BINARY_NAME" ] && BINARY_NAME="$BUILD_TARGET"
  DEPLOY_DIR="$BUILD_DIR/deploy"
  rm -rf "$DEPLOY_DIR"
  mkdir -p "$DEPLOY_DIR"
  if [ -f "$BUILD_DIR/bin/$BINARY_NAME" ]; then
    cp "$BUILD_DIR/bin/$BINARY_NAME" "$DEPLOY_DIR/"
    log "  ✓ Copied $BINARY_NAME"
  else
    error "Binary not found: $BUILD_DIR/bin/$BINARY_NAME"
  fi
  if [ -d "$REPO_ROOT/data" ]; then
    ln -sf ../../data "$DEPLOY_DIR/data"
    log "  ✓ Data directory linked (deploy/data -> ../../data)"
  else
    log "  ⚠ Warning: data directory not found"
  fi
  if [ -d "$REPO_ROOT/data" ] && [ ! -e "$BUILD_DIR/bin/data" ]; then
    ln -sf ../../data "$BUILD_DIR/bin/data"
    log "  ✓ Data linked in bin (bin/data -> ../../data)"
  fi
  log "Linux deployment directory ready: $DEPLOY_DIR"
  log "  → Run from deploy: cd $DEPLOY_DIR && ./$BINARY_NAME"
  log "  → Or from bin: $BUILD_DIR/bin/$BINARY_NAME"
}

create_linux_appimage() {
  if ! command_exists linuxdeployqt; then
    # Use ~/bin if setup.sh installed linuxdeployqt there
    if [ -x "${HOME}/bin/linuxdeployqt" ]; then
      export PATH="${HOME}/bin:${PATH}"
    fi
  fi
  command_exists linuxdeployqt || error "linuxdeployqt not found (required for CREATE_INSTALLER=yes). Run make setup-linux to install it, or install from https://github.com/probonopd/linuxdeployqt."
  log "Creating AppImage with linuxdeployqt..."

  # linuxdeployqt needs qmake; on Qt6-only systems /usr/bin/qmake may point to missing Qt5. Prefer Qt6 qmake.
  QMAKE_FOR_DEPLOY=""
  # Resolve Qt6 dir: use env, or read from CMakeCache.txt if build was already configured
  QT6_CMAKE_DIR="${Qt6_DIR:-}"
  if [ -z "$QT6_CMAKE_DIR" ] && [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    QT6_CMAKE_DIR="$(grep -E '^Qt6_DIR:STATIC=' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null | cut -d= -f2-)"
  fi
  if [ -n "$QT6_CMAKE_DIR" ] && [ -d "$QT6_CMAKE_DIR" ]; then
    QT6_PREFIX="$(cd "$(dirname "$(dirname "$QT6_CMAKE_DIR")")" 2>/dev/null && pwd)"
    for q in "$QT6_PREFIX/qt6/bin/qmake" "$QT6_PREFIX/qt6/bin/qmake6" /usr/lib/qt6/bin/qmake /usr/lib/qt6/bin/qmake6 /usr/lib/x86_64-linux-gnu/qt6/bin/qmake /usr/lib/x86_64-linux-gnu/qt6/bin/qmake6; do
      if [ -x "$q" ]; then
        QMAKE_FOR_DEPLOY="$q"
        break
      fi
    done
  fi
  if [ -z "$QMAKE_FOR_DEPLOY" ] && command_exists qmake6; then
    QMAKE_FOR_DEPLOY="$(command -v qmake6)"
  fi
  if [ -z "$QMAKE_FOR_DEPLOY" ] && [ -x /usr/bin/qmake6 ]; then
    QMAKE_FOR_DEPLOY="/usr/bin/qmake6"
  fi
  if [ -n "$QMAKE_FOR_DEPLOY" ]; then
    export PATH="$(dirname "$QMAKE_FOR_DEPLOY"):$PATH"
    LINUXDEPLOYQT_EXTRA_ARGS=(-qmake="$QMAKE_FOR_DEPLOY")
    log "  Using qmake: $QMAKE_FOR_DEPLOY"
  else
    LINUXDEPLOYQT_EXTRA_ARGS=()
  fi

  if is_yes "${LINUXDEPLOYQT_ALLOW_NEW_GLIBC:-no}"; then
    LINUXDEPLOYQT_EXTRA_ARGS+=(-unsupported-allow-new-glibc)
    log "  (LINUXDEPLOYQT_ALLOW_NEW_GLIBC=yes: allowing newer host glibc for testing; AppImage may not run on older distros)"
  fi
  linuxdeployqt "$DEPLOY_DIR/$BINARY_NAME" -appimage "${LINUXDEPLOYQT_EXTRA_ARGS[@]}"
}

print_build_summary() {
  log "Build complete!"
  echo ""
  echo "✓ Platform: $TARGET_PLATFORM"
  echo "✓ Build directory: $BUILD_DIR"
  echo "✓ Target: $BUILD_TARGET"
  echo ""
  if [ "$TARGET_PLATFORM" = "windows" ]; then
    if is_yes "${CREATE_INSTALLER:-no}"; then
      if [ -n "${INSTALLER:-}" ]; then
        echo "Installer: $INSTALLER"
      else
        echo "Installer: docker/windows/PokerTH-*-Setup.exe"
      fi
      echo ""
      echo "To test with Wine: cd $BUILD_DIR/deploy && wine pokerth_client.exe"
    else
      echo "Windows deployment directory: $BUILD_DIR/deploy"
      echo ""
      echo "This directory contains everything needed:"
      echo "  - Executable and all DLLs"
      echo "  - Data directory (stylesheets, icons, sounds, translations)"
      echo "  - Qt plugins"
      echo ""
      echo "Usage:"
      echo "  1. Copy entire 'deploy' directory to Windows and run pokerth_client.exe"
      echo "  2. Or create installer: make windows-installer"
      echo "     (output: docker/windows/PokerTH-*-Setup.exe)"
      echo ""
      echo "To test with Wine:"
      echo "  cd $BUILD_DIR/deploy"
      echo "  wine pokerth_client.exe"
    fi
  else
    echo "Linux deployment directory: $BUILD_DIR/deploy"
    echo ""
    echo "To run:"
    echo "  cd $BUILD_DIR/deploy && ./$BINARY_NAME"
    echo "  # or: $BUILD_DIR/bin/$BINARY_NAME  (bin/data points to repo data)"
    echo ""
    if is_yes "$USE_AQT"; then
      echo "Note: Qt was installed via aqtinstall at: $QT_DIR"
      echo "      Make sure Qt libraries are in your LD_LIBRARY_PATH or use system packages"
    fi
  fi
  echo ""
}

########################################
# 1. Check dependencies
########################################

log "Checking build dependencies..."

check_dependency cmake "$SETUP_SCRIPT"
check_dependency ninja "$SETUP_SCRIPT"
check_dependency git "$SETUP_SCRIPT"

if [ "$TARGET_PLATFORM" = "windows" ]; then
  check_dependency x86_64-w64-mingw32-gcc "$SETUP_SCRIPT"
  check_dependency x86_64-w64-mingw32-g++ "$SETUP_SCRIPT"
  check_dependency x86_64-w64-mingw32-windres "$SETUP_SCRIPT"
  log "✓ MinGW-w64 toolchain found"
fi

check_qt_deps "$TARGET_PLATFORM" "$USE_AQT" "$QT_DIR" "$SETUP_SCRIPT"

if is_yes "$USE_VCPKG"; then
  check_vcpkg_deps "$SETUP_SCRIPT"
fi

log "✓ All dependencies found"

########################################
# 2. Build PokerTH
########################################

if [ "$TARGET_PLATFORM" = "windows" ]; then
  BUILD_DIR="$REPO_ROOT/build_windows"
else
  BUILD_DIR="$REPO_ROOT/build_linux"
fi

if is_yes "$CLEAN"; then
  log "Cleaning and reconfiguring build for $TARGET_PLATFORM..."
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  NEED_CONFIGURE=1
else
  log "Reusing existing build directory for $TARGET_PLATFORM..."
  mkdir -p "$BUILD_DIR"
  if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    # Linux native build uses Ninja; skip configure only if build.ninja exists
    if [ "$TARGET_PLATFORM" = "linux" ] && [ ! -f "$BUILD_DIR/build.ninja" ]; then
      NEED_CONFIGURE=1
      log "Reconfiguring (build.ninja missing; may be from different generator or partial build)."
    else
      NEED_CONFIGURE=0
      log "Skipping configure (CMakeCache.txt present); building only."
    fi
  else
    NEED_CONFIGURE=1
  fi
fi

if [ "$NEED_CONFIGURE" = "1" ]; then
  if [ "$TARGET_PLATFORM" = "windows" ]; then
    configure_cmake_windows
  else
    configure_cmake_linux
  fi
fi

log "Building ${BUILD_TARGET}..."
cmake --build "$BUILD_DIR" --target "$BUILD_TARGET" --parallel $(nproc)

########################################
# 3. Create Windows deployment directory (always for Windows builds)
########################################

if [ "$TARGET_PLATFORM" = "windows" ]; then
  create_windows_deploy_dir
  if is_yes "${CREATE_INSTALLER:-no}"; then
    create_windows_nsis_installer
  fi
fi

########################################
# 4. Create Linux deployment directory (always, like Windows)
########################################

if [ "$TARGET_PLATFORM" != "windows" ]; then
  create_linux_deploy_dir
  if is_yes "${CREATE_INSTALLER:-no}"; then
    create_linux_appimage
  fi
fi

########################################
# Summary
########################################

print_build_summary
