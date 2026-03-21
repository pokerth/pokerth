#!/usr/bin/env bash

###
# Setup script for PokerTH build environment on Linux
# Installs all required dependencies and tools
# Run this once as root/sudo, then use make linux / make windows or TARGET_PLATFORM=linux|windows ./scripts/build.sh (defaults to build_<platform>/)

set -euo pipefail

# Source shared build environment (sets REPO_ROOT when in scripts/)
source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"

########################################
# Linux-specific Configuration
########################################

require_target_platform

if [ $# -gt 0 ]; then
  echo "Usage: scripts/setup.sh (prefer: make setup-linux, make setup-windows, make setup-android, make setup-macos)"
  echo "  No arguments. Set environment variables to change behavior."
  echo ""
  echo "Environment: TARGET_PLATFORM, USE_AQT, USE_VCPKG, VCPKG_DIR / BUILD_DIR (android and windows), VCPKG_TRIPLET (android)"
  echo ""
  echo "Examples:"
  echo "  make setup-linux"
  echo "  TARGET_PLATFORM=windows scripts/setup.sh"
  echo "  TARGET_PLATFORM=android scripts/setup.sh  # VCPKG_DIR defaults to BUILD_DIR/vcpkg or build_android/vcpkg"
  echo ""
  echo "Then run make linux or make windows to build."
  exit 0
fi

# Dispatch to platform-specific setup or fall through for linux/windows shared path
run_setup "$TARGET_PLATFORM"

# Setup Qt paths based on target platform (USE_AQT/USE_VCPKG resolved by run_setup)
setup_linux_paths "$TARGET_PLATFORM" "$USE_AQT" "$USE_VCPKG"

# Windows cross-build: x86_64 is the only tested path.
if [ "$TARGET_PLATFORM" = "windows" ] && [ "$(uname -m)" != "x86_64" ]; then
  log "Warning: Windows cross-build is only tested on x86_64."
fi

########################################
# Linux-specific Helper functions
########################################

detect_distro() {
  if [ -f /etc/os-release ]; then
    . /etc/os-release
    echo "$ID"
  elif command_exists lsb_release; then
    lsb_release -si | tr '[:upper:]' '[:lower:]'
  else
    echo "unknown"
  fi
}

check_root() {
  if [ "$EUID" -eq 0 ]; then
    log "Running as root - system packages will be installed"
    return 0
  elif command_exists sudo && sudo -n true 2>/dev/null; then
    log "Running with sudo access - system packages will be installed"
    return 0
  else
    log "Note: Some operations may require sudo. You may be prompted for your password."
    return 1
  fi
}

########################################
# 1. Detect Linux distribution
########################################

DISTRO=$(detect_distro)
log "Detected Linux distribution: $DISTRO"

case "$DISTRO" in
  ubuntu|debian)
    PKG_MANAGER="apt"
    if [ "$EUID" -eq 0 ]; then
      INSTALL_CMD="apt-get install -y"
      UPDATE_CMD="apt-get update"
    else
      INSTALL_CMD="sudo apt-get install -y"
      UPDATE_CMD="sudo apt-get update"
    fi
    ;;
  fedora|rhel|centos)
    PKG_MANAGER="dnf"
    if [ "$EUID" -eq 0 ]; then
      INSTALL_CMD="dnf install -y"
      UPDATE_CMD="dnf check-update || true"
    else
      INSTALL_CMD="sudo dnf install -y"
      UPDATE_CMD="sudo dnf check-update || true"
    fi
    ;;
  arch|manjaro)
    PKG_MANAGER="pacman"
    if [ "$EUID" -eq 0 ]; then
      INSTALL_CMD="pacman -S --noconfirm"
      UPDATE_CMD="pacman -Sy"
    else
      INSTALL_CMD="sudo pacman -S --noconfirm"
      UPDATE_CMD="sudo pacman -Sy"
    fi
    ;;
  *)
    log "Unknown distribution, assuming Debian/Ubuntu-like (apt)"
    PKG_MANAGER="apt"
    if [ "$EUID" -eq 0 ]; then
      INSTALL_CMD="apt-get install -y"
      UPDATE_CMD="apt-get update"
    else
      INSTALL_CMD="sudo apt-get install -y"
      UPDATE_CMD="sudo apt-get update"
    fi
    ;;
esac

########################################
# 2. Install base packages
########################################
# Skip when SKIP_SYSTEM_PACKAGES=yes (e.g. Dockerfile already ran apt).
SKIP_SYSTEM_PACKAGES="${SKIP_SYSTEM_PACKAGES:-no}"
if is_yes "$SKIP_SYSTEM_PACKAGES"; then
  log "Skipping system package install (SKIP_SYSTEM_PACKAGES=yes)."
else
  log "Updating package lists..."
  $UPDATE_CMD

  log "Installing base packages via $PKG_MANAGER..."

  # Windows + apt: use shared list (same as docker/windows Dockerfile). Windows + dnf/pacman and Linux: use inline lists below.
  if [ "$TARGET_PLATFORM" = "windows" ] && [ "$PKG_MANAGER" = "apt" ] && [ -f "${REPO_ROOT}/scripts/windows-apt-packages.txt" ]; then
    BASE_PKGS=$(grep -v '^#' "${REPO_ROOT}/scripts/windows-apt-packages.txt" | tr '\n' ' ')
  else
    # Common base packages per package manager (shared by Windows and Linux builds)
    COMMON_APT="build-essential cmake ninja-build git python3 python3-pip pkg-config"
    COMMON_DNF="gcc gcc-c++ cmake ninja-build git python3 python3-pip pkgconfig"
    COMMON_PACMAN="base-devel cmake ninja git python python-pip pkgconf"
    # Platform-specific extras (Windows dnf/pacman use these; Windows apt uses windows-apt-packages.txt when available)
    WINDOWS_APT_EXTRA="mingw-w64 autoconf automake libtool curl zip unzip tar nsis imagemagick librsvg2-bin protobuf-compiler"
    WINDOWS_DNF_EXTRA="mingw64-gcc mingw64-gcc-c++ autoconf automake libtool curl zip unzip tar nsis ImageMagick"
    WINDOWS_PACMAN_EXTRA="mingw-w64-gcc autoconf automake libtool curl zip unzip tar nsis imagemagick"
    LINUX_APT_EXTRA="libssl-dev libprotobuf-dev protobuf-compiler libboost-all-dev libwebsocketpp-dev libfuse2 fuse"
    LINUX_DNF_EXTRA="openssl-devel protobuf-devel protobuf-compiler boost-devel websocketpp-devel fuse fuse-libs"
    LINUX_PACMAN_EXTRA="openssl protobuf boost websocketpp fuse2"

    case "$PKG_MANAGER" in
      apt)   PKG_SUFFIX="APT" ;;
      dnf)   PKG_SUFFIX="DNF" ;;
      pacman) PKG_SUFFIX="PACMAN" ;;
      *)     error "Unsupported package manager: $PKG_MANAGER" ;;
    esac
    ref_common="COMMON_${PKG_SUFFIX}"
    ref_extra="$([ "$TARGET_PLATFORM" = "windows" ] && echo "WINDOWS_" || echo "LINUX_")${PKG_SUFFIX}_EXTRA"
    BASE_PKGS="${!ref_common} ${!ref_extra}"
  fi
  $INSTALL_CMD $BASE_PKGS
fi

if [ "$TARGET_PLATFORM" = "windows" ]; then
  if ! command_exists x86_64-w64-mingw32-gcc; then
    error "MinGW-w64 toolchain not found. Please install mingw-w64 package."
  fi
  log "✓ MinGW-w64 toolchain found"
fi

########################################
# Script-local: install Qt for platform (plan section 2.2)
########################################

install_qt_for_platform() {
  # Docker flows: Qt is always in the image; skip install (SKIP_QT_INSTALL set by ensure / Makefile docker stamp).
  if is_yes "${SKIP_QT_INSTALL:-}"; then
    log "Qt comes from image (SKIP_QT_INSTALL=yes), skipping Qt install."
    return 0
  fi
  if is_yes "$USE_AQT"; then
    log "Using aqtinstall for Qt installation..."
    setup_pipx_aqt "$PKG_MANAGER" "$INSTALL_CMD"

    if [ "$TARGET_PLATFORM" = "windows" ]; then
      log "Installing Qt ${QT_VERSION} for Windows cross-compilation..."
      if [ ! -d "$QT_WINDOWS_DIR" ] || [ ! -f "$QT_WINDOWS_DIR/bin/qmake" ]; then
        install_qt_with_modules windows "$QT_VERSION" mingw_64 "$QT_OUTPUT_DIR" "${QT_MODULES[@]}"
        if [ -d "$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw" ] && [ -f "$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw/bin/qmake" ]; then
          QT_WINDOWS_DIR="$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw"
        elif [ -d "$QT_OUTPUT_DIR/$QT_VERSION/mingw_64" ] && [ -f "$QT_OUTPUT_DIR/$QT_VERSION/mingw_64/bin/qmake" ]; then
          QT_WINDOWS_DIR="$QT_OUTPUT_DIR/$QT_VERSION/mingw_64"
        fi
      else
        log "Qt Windows already installed at: $QT_WINDOWS_DIR"
      fi
      if [ ! -d "$QT_HOST_PATH" ] || [ ! -f "$QT_HOST_PATH/bin/qt-cmake" ]; then
        install_qt_with_modules linux "$QT_VERSION" gcc_64 "$QT_OUTPUT_DIR" "${QT_MODULES[@]}"
      else
        log "Qt host tools (gcc_64) already installed at: $QT_HOST_PATH"
      fi
    else
      if [ ! -d "$QT_DIR" ] || [ ! -f "$QT_DIR/bin/qmake" ]; then
        install_qt_with_modules linux "$QT_VERSION" gcc_64 "$QT_OUTPUT_DIR" "${QT_MODULES[@]}"
        log "Qt installed at: $QT_DIR"
      else
        log "Qt ${QT_VERSION} already installed at: $QT_DIR"
      fi
    fi
  else
    if [ "$TARGET_PLATFORM" = "windows" ]; then
      error "Windows cross-compilation requires aqtinstall. Set USE_AQT=yes or TARGET_PLATFORM=windows (which auto-enables it)."
    fi
    log "Installing system Qt packages..."
    case "$PKG_MANAGER" in
      apt)
        $INSTALL_CMD \
          qt6-base-dev \
          qt6-svg-dev \
          qt6-declarative-dev \
          qt6-tools-dev \
          linguist-qt6 \
          qt6-websockets-dev \
          qt6-multimedia-dev \
          libqt6sql6-mysql
        ;;
      dnf)
        $INSTALL_CMD \
          qt6-qtbase-devel \
          qt6-qtsvg-devel \
          qt6-qtdeclarative-devel \
          qt6-qttools-devel \
          qt6-qtwebsockets-devel \
          qt6-qtmultimedia-devel \
          qt6-qtbase-mysql
        ;;
      pacman)
        $INSTALL_CMD \
          qt6-base \
          qt6-svg \
          qt6-declarative \
          qt6-tools \
          qt6-websockets \
          qt6-multimedia
        ;;
    esac
  fi
}

########################################
# 3. Install Qt (system packages or aqtinstall)
########################################

install_qt_for_platform

########################################
# 4. vcpkg (optional, required for Windows)
########################################

if is_yes "$USE_VCPKG"; then
  log "Setting up vcpkg..."

  # Install build tools for vcpkg (if not already installed for Windows)
  if [ "$TARGET_PLATFORM" != "windows" ]; then
    vcpkg_pkgs="autoconf automake libtool"
    [ "$PKG_MANAGER" = "apt" ] && vcpkg_pkgs="autoconf autoconf-archive automake libtool"
    $INSTALL_CMD $vcpkg_pkgs
  fi

  setup_vcpkg "$VCPKG_TARGET_TRIPLET"
  log "✓ vcpkg setup complete"
fi

########################################
# 5. linuxdeployqt (Linux only, for make linux-installer)
########################################

if [ "$TARGET_PLATFORM" = "linux" ]; then
  if command_exists linuxdeployqt; then
    log "✓ linuxdeployqt already in PATH"
  else
    log "Installing linuxdeployqt for AppImage creation (make linux-installer)..."
    BIN_DIR="${HOME}/bin"
    mkdir -p "$BIN_DIR"
    export PATH="$BIN_DIR:$PATH"
    if [ -f "$BIN_DIR/linuxdeployqt" ]; then
      log "✓ linuxdeployqt already present in $BIN_DIR"
    else
      LINUXDEPLOYQT_URL="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
      if command_exists curl; then
        curl -sL -o "$BIN_DIR/linuxdeployqt" "$LINUXDEPLOYQT_URL" || error "Failed to download linuxdeployqt. Install manually from https://github.com/probonopd/linuxdeployqt/releases"
      elif command_exists wget; then
        wget -q -O "$BIN_DIR/linuxdeployqt" "$LINUXDEPLOYQT_URL" || error "Failed to download linuxdeployqt. Install manually from https://github.com/probonopd/linuxdeployqt/releases"
      else
        error "curl or wget required to download linuxdeployqt. Install curl or wget, or install linuxdeployqt manually from https://github.com/probonopd/linuxdeployqt/releases"
      fi
      chmod +x "$BIN_DIR/linuxdeployqt"
      log "✓ linuxdeployqt installed to $BIN_DIR/linuxdeployqt"
    fi
    if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
      log "Add to your .bashrc to use linuxdeployqt: export PATH=\"\$HOME/bin:\$PATH\""
    fi
  fi
fi

########################################
# Summary
########################################

log "Setup complete!"
echo ""
echo "✓ Platform: $TARGET_PLATFORM"
echo "✓ System packages installed"
if is_yes "$USE_AQT"; then
  if [ "$TARGET_PLATFORM" = "windows" ]; then
    echo "✓ Qt Windows: $QT_WINDOWS_DIR"
    echo "✓ Qt Host: $QT_HOST_PATH"
  else
    echo "✓ Qt: $QT_DIR"
  fi
fi
if is_yes "$USE_VCPKG"; then
  echo "✓ vcpkg: $VCPKG_DIR"
fi
if [ "$TARGET_PLATFORM" = "linux" ] && command_exists linuxdeployqt; then
  echo "✓ linuxdeployqt: $(command -v linuxdeployqt) (for make linux-installer)"
fi
echo ""
if [ "$TARGET_PLATFORM" = "windows" ]; then
  echo "You can now run make windows (or make) as a regular user to build PokerTH"
else
  echo "You can now run make linux (or make) as a regular user to build PokerTH"
fi
echo ""
