#!/usr/bin/env bash

###
# Setup script for PokerTH build environment on macOS
# Installs all required dependencies and tools
# Run this once, then use make macos or scripts/build_macos.sh as a regular user

set -euo pipefail

# Source shared build environment (sets REPO_ROOT when in scripts/)
source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"

########################################
# macOS-specific Configuration
########################################

BREW_PREFIX_DEFAULT="/opt/homebrew"   # Apple Silicon
QT_DIR="$QT_OUTPUT_DIR/$QT_VERSION/macos"

if [ $# -gt 0 ]; then
  echo "Usage: make setup-macos or ./scripts/setup_macos.sh"
  echo "  No arguments."
  echo ""
  echo "Then run make macos or ./scripts/build_macos.sh to build."
  exit 0
fi

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
brew update
brew install \
  cmake \
  ninja \
  git \
  python \
  pkg-config \
  libiodbc \
  libpq \
  imagemagick
# Optional: Mimer SQL client (libmimerapi) is not in Homebrew; install from
# https://developer.mimer.com/downloads if you need the Qt Mimer SQL driver.

# On Apple Silicon, Qt SQL plugins may expect /usr/local/opt; symlink so macdeployqt finds them.
BREW_PREFIX="${HOMEBREW_PREFIX:-$(brew --prefix 2>/dev/null)}"
if [ -n "$BREW_PREFIX" ] && [ "$BREW_PREFIX" != "/usr/local" ] && [ -d "$BREW_PREFIX/opt" ]; then
  for pkg in libiodbc libpq; do
    if [ -d "$BREW_PREFIX/opt/$pkg" ] && [ ! -d "/usr/local/opt/$pkg" ]; then
      log "Linking $pkg for Qt SQL plugins (macdeployqt expects /usr/local/opt on some Qt builds)..."
      sudo mkdir -p /usr/local/opt
      sudo ln -sf "$BREW_PREFIX/opt/$pkg" "/usr/local/opt/$pkg"
    fi
  done
fi

########################################
# 3. pipx and aqtinstall
########################################

setup_pipx_aqt

########################################
# 4. vcpkg
########################################

log "Installing vcpkg build tools..."
brew install \
  autoconf \
  autoconf-archive \
  automake \
  libtool

VCPKG_TRIPLET=$(get_vcpkg_triplet_macos)
setup_vcpkg "$VCPKG_TRIPLET"

########################################
# 5. Qt installation (aqtinstall)
########################################

# Check if Qt is already installed
if [ -d "$QT_DIR" ] && [ -f "$QT_DIR/bin/qmake" ] && [ -f "$QT_DIR/bin/macdeployqt" ]; then
  log "Qt ${QT_VERSION} already installed at: $QT_DIR"
else
  install_qt_with_modules mac "$QT_VERSION" clang_64 "$QT_OUTPUT_DIR" "${QT_MODULES[@]}"

  log "Qt installed at: $QT_DIR"
fi

########################################
# Summary
########################################

log "Setup complete!"
echo ""
echo "✓ Homebrew packages installed"
echo "✓ Qt: $QT_DIR"
echo "✓ vcpkg: $VCPKG_DIR"
echo ""
echo "You can now run make macos (or make) as a regular user to build PokerTH"
echo ""
