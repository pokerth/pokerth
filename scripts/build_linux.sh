#!/usr/bin/env bash

###
# Build PokerTH on a Linux host for TARGET_PLATFORM=linux (native) or windows (MinGW cross).
# Invoked by scripts/build.sh; may be run directly with TARGET_PLATFORM set.

set -euo pipefail

# shellcheck source=/dev/null
source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"

# Sets TARGET_PLATFORM when unset; Makefile / build.sh normally pass it (linux | windows).
default_target_platform_from_uname

########################################
# Usage
########################################

show_usage() {
  echo "Usage: scripts/build_linux.sh (prefer: make linux, make windows)"
  echo "  Or: scripts/build.sh (dispatches here for linux/windows)."
  echo "  No arguments. Set environment variables to change behavior."
  echo ""
  echo "Environment: TARGET_PLATFORM (optional; default based on host OS), REPO_BUILD_ROOT (optional; default build_<platform>, same as Makefile when unset on host), BUILD_TARGET, USE_AQT, USE_VCPKG, CLEAN, CREATE_INSTALLER"
  echo ""
  echo "Examples:"
  echo "  make linux"
  echo "  CREATE_INSTALLER=yes make linux-installer"
  echo "  CLEAN=yes make linux"
  echo ""
  echo "Note: Run make setup-<platform> first if dependencies are missing."
}
exit_with_usage_if_args show_usage "$@"

# BUILD_TARGET, CLEAN, USE_AQT, USE_VCPKG — see init_build_defaults host in functions.sh
init_build_defaults host

# Optional: CREATE_INSTALLER=yes creates Linux AppImage (linuxdeployqt) or Windows NSIS installer (makensis; apt install nsis)

# Setup Qt paths based on target platform (overwrites USE_AQT/USE_VCPKG for windows — see functions.sh)
setup_linux_paths "$TARGET_PLATFORM"

########################################
# Installer helpers (deploy helpers are in functions.sh)
########################################

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
  DEPLOY_PATH_REL="../../${BUILD_DIR_REL}/deploy"
  if ! (cd "$NSIS_DIR" && makensis -NOCD -DDeployPath="$DEPLOY_PATH_REL" installer.nsi); then
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

create_linux_appimage() {
  if ! command_exists linuxdeployqt; then
    # Use ~/bin if setup_linux.sh installed linuxdeployqt there
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
    is_yes "${CREATE_INSTALLER:-no}" && echo "Installer: ${INSTALLER:-docker/windows/PokerTH-*-Setup.exe}"
    echo "Deploy: $BUILD_DIR/deploy (exe, DLLs, data, Qt plugins)"
    echo "  Copy deploy/ to Windows, or: make windows-installer"
    echo "  Test with Wine: cd $BUILD_DIR/deploy && wine pokerth_client.exe"
  else
    echo "Deploy: $BUILD_DIR/deploy — run: cd $BUILD_DIR/deploy && ./$BINARY_NAME"
    is_yes "$USE_AQT" && echo "  (Qt via aqtinstall: $QT_DIR)"
  fi
  echo ""
}

########################################
# 1. Check dependencies
########################################

log "Checking build dependencies..."

check_dependency cmake "make setup-${TARGET_PLATFORM}"
check_dependency ninja "make setup-${TARGET_PLATFORM}"
check_dependency git "make setup-${TARGET_PLATFORM}"

if [ "$TARGET_PLATFORM" = "windows" ]; then
  check_dependency x86_64-w64-mingw32-gcc "make setup-windows"
  check_dependency x86_64-w64-mingw32-g++ "make setup-windows"
  check_dependency x86_64-w64-mingw32-windres "make setup-windows"
  log "✓ MinGW-w64 toolchain found"
fi

check_qt_deps "$TARGET_PLATFORM" "$USE_AQT" "$QT_DIR" "make setup-${TARGET_PLATFORM}"

if is_yes "$USE_VCPKG"; then
  check_vcpkg_deps "make setup-${TARGET_PLATFORM}"
fi

log "✓ All dependencies found"

########################################
# 2. Build PokerTH
########################################
# Repo-relative build tree: Makefile passes REPO_BUILD_ROOT (matches stamp path in Docker).
# Manual runs default to build_<TARGET_PLATFORM> (same as host Makefile when IN_DOCKER is unset).

if [ -n "${REPO_BUILD_ROOT:-}" ]; then
  BUILD_DIR_REL="$REPO_BUILD_ROOT"
else
  BUILD_DIR_REL="build_${TARGET_PLATFORM}"
fi
BUILD_DIR="$REPO_ROOT/$BUILD_DIR_REL"

if is_yes "$CLEAN"; then
  log "Cleaning and reconfiguring build for $TARGET_PLATFORM..."
  rm -rf "$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  NEED_CONFIGURE=1
else
  log "Reusing existing build directory for $TARGET_PLATFORM..."
  mkdir -p "$BUILD_DIR"
  if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
    # Only reconfigure if cache was for a different repo path (e.g. make windows vs make windows-docker).
    # If the build dir is broken (e.g. missing Makefile), run CLEAN=yes and retry.
    CACHED_HOME="$(grep -E '^CMAKE_HOME_DIRECTORY' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null | cut -d= -f2- | sed 's|/$||')"
    REPO_NORM="${REPO_ROOT%/}"
    if [ -n "$CACHED_HOME" ] && [ "$CACHED_HOME" != "$REPO_NORM" ]; then
      NEED_CONFIGURE=1
      log "Build dir was for a different path; reconfiguring for $REPO_NORM."
      rm -rf "$BUILD_DIR"
      mkdir -p "$BUILD_DIR"
    else
      NEED_CONFIGURE=0
      log "Skipping configure (CMakeCache.txt present); building only."
    fi
  else
    NEED_CONFIGURE=1
  fi
fi

if [ "$NEED_CONFIGURE" = "1" ]; then
  case "$TARGET_PLATFORM" in
    windows) _configure_cmake_windows ;;
    linux)   _configure_cmake_linux ;;
    *)       error "configure: unsupported TARGET_PLATFORM $TARGET_PLATFORM" ;;
  esac
fi

log "Building ${BUILD_TARGET}..."
cmake --build "$BUILD_DIR" --target "$BUILD_TARGET" --parallel "$(nproc)"

########################################
# 3. Create deployment directory (always)
########################################

case "$TARGET_PLATFORM" in
  windows) _create_windows_deploy_dir ;;
  linux)   _create_linux_deploy_dir ;;
  *)       error "deploy: unsupported TARGET_PLATFORM $TARGET_PLATFORM" ;;
esac

########################################
# 4. Create installer if requested
########################################

if is_yes "${CREATE_INSTALLER:-no}"; then
  if [ "$TARGET_PLATFORM" = "windows" ]; then
    create_windows_nsis_installer
  else
    create_linux_appimage
  fi
fi

########################################
# Summary
########################################

print_build_summary
