#!/usr/bin/env bash

###
# Shared functions and configuration for PokerTH
# Sourced by setup and build scripts. Do not execute directly.

########################################
# Common Configuration
########################################

# Canonical version definitions: scripts/versions.env. If the build fails on a version (e.g. QT_VERSION), check there.
_func_dir="$(cd "$(dirname "${BASH_SOURCE[0]:-.}")" && pwd)"
if [ -f "$_func_dir/versions.env" ]; then
  # shellcheck source=/dev/null
  . "$_func_dir/versions.env"
fi

# Qt installation directory
QT_OUTPUT_DIR="${QT_OUTPUT_DIR:-$HOME/Qt}"

# vcpkg directory
VCPKG_DIR="${VCPKG_DIR:-$HOME/vcpkg}"

if [ "$(basename "$_func_dir")" = "scripts" ]; then
  REPO_ROOT="${REPO_ROOT:-$(cd "$_func_dir/.." && pwd)}"
else
  REPO_ROOT="${REPO_ROOT:-$_func_dir}"
fi
unset _func_dir

########################################
# Qt Modules List
# Base Qt includes: Core, Network, Sql, Xml, Widgets, Svg, Qml, Quick, QuickControls2, Multimedia, etc.
# Only specify additional optional modules here if needed
# Empty by default - base Qt has everything PokerTH needs
########################################

QT_MODULES=(
  # Add optional modules here if needed
  qtwebsockets   # Required for PokerTH WebSockets support
  qtmultimedia   # Required for PokerTH Multimedia support
  # qtsvg         # Usually included in base
)

########################################
# vcpkg Ports List
########################################

VCPKG_PORTS=(
  # Boost components (required by CMakeLists.txt)
  boost-any
  boost-asio
  boost-atomic
  boost-chrono
  boost-container
  boost-date-time
  boost-filesystem      # Required: find_package(Boost REQUIRED COMPONENTS filesystem)
  boost-foreach
  boost-interprocess
  boost-iostreams       # Required: find_package(Boost REQUIRED COMPONENTS iostreams)
  boost-lambda
  boost-program-options # Required: find_package(Boost REQUIRED COMPONENTS program_options)
  boost-random          # Required: find_package(Boost REQUIRED COMPONENTS random)
  boost-system
  boost-thread         # Required: find_package(Boost REQUIRED COMPONENTS thread)
  boost-serialization
  boost-smart-ptr
  # Protocol and crypto
  protobuf              # Required: find_package(Protobuf CONFIG REQUIRED)
  openssl               # Required: find_package(OpenSSL REQUIRED)
  # Protobuf dependencies (required for protobuf >= 5.27 on Windows/macOS)
  abseil                # Required: find_package(absl CONFIG REQUIRED) for Windows/macOS
  utf8-range            # Required: find_package(utf8_range CONFIG REQUIRED) for Windows/macOS
)
# Note: Main 'boost' package removed - it would include all submodules including boost-cobalt
# Only essential modules are installed instead
# Note: curl removed - using Qt Network instead
# Note: Qt6 is installed via aqtinstall, not vcpkg

########################################
# Download command: adjust timeouts/retries here.
########################################
CURL_DOWNLOAD_CMD="curl --connect-timeout 15 --max-time 180 --retry 3 --retry-delay 5 -fSL"

curl_cmd() { $CURL_DOWNLOAD_CMD "$@"; }

########################################
# Helper Functions
########################################

log() {
  echo "▶ $1"
}

error() {
  echo "✗ ERROR: $1" >&2
  exit 1
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

# True if var is "yes" or "true" (for USE_AQT, CLEAN, CREATE_INSTALLER, etc.)
is_yes() {
  local v="${1:-}"
  [ "$v" = "yes" ] || [ "$v" = "true" ]
}

########################################
# Platform Detection
########################################

detect_arch() {
  uname -m
}

is_arm64() {
  [[ "$(detect_arch)" == "arm64" ]]
}

########################################
# Platform (Makefile is the entry point; passes TARGET_PLATFORM)
########################################

# Require TARGET_PLATFORM. Scripts are invoked by Makefile with TARGET_PLATFORM set.
require_target_platform() {
  if [ -z "${TARGET_PLATFORM:-}" ]; then
    error "TARGET_PLATFORM required. Use make setup-<platform> or make <platform> (e.g. make setup-linux, make linux)."
  fi
}

# Resolve and export VCPKG_DIR based on platform and BUILD_DIR.
# Usage: resolve_vcpkg_dir <platform>
# Order: VCPKG_DIR > VCPKG_ROOT > (BUILD_DIR or build_<platform>)/vcpkg
resolve_vcpkg_dir() {
  local platform="${1:-$TARGET_PLATFORM}"
  local default_build="${BUILD_DIR:-build_${platform}}"
  export VCPKG_DIR="${VCPKG_DIR:-${VCPKG_ROOT:-${default_build}/vcpkg}}"
}

# Resolve USE_AQT and USE_VCPKG for the given platform.
# Usage: resolve_use_aqt_use_vcpkg <platform>
# Windows requires both; linux/android use env or default "no".
resolve_use_aqt_use_vcpkg() {
  local platform="${1:-$TARGET_PLATFORM}"
  if [ "$platform" = "windows" ]; then
    USE_AQT="yes"
    USE_VCPKG="yes"
  else
    USE_AQT="${USE_AQT:-no}"
    USE_VCPKG="${USE_VCPKG:-no}"
  fi
}

# Dispatch setup to platform-specific script or fall through for linux/windows.
# Usage: run_setup [platform]
# android → exec setup_android.sh; macos → exec setup_macos.sh; else return (shared path continues).
run_setup() {
  local platform="${1:-$TARGET_PLATFORM}"
  resolve_vcpkg_dir "$platform"
  resolve_use_aqt_use_vcpkg "$platform"
  case "$platform" in
    android)
      log "Android setup..."
      exec "${REPO_ROOT}/scripts/setup_android.sh"
      ;;
    macos)
      log "Macos setup..."
      exec "${REPO_ROOT}/scripts/setup_macos.sh"
      ;;
    *) return 0 ;;
  esac
}

# Dispatch build to platform-specific script or fall through for linux/windows.
# Usage: run_build [platform]
# macos → exec build_macos.sh; android → exec build_android.sh; else return (shared path continues).
run_build() {
  local platform="${1:-$TARGET_PLATFORM}"
  case "$platform" in
    macos)
      exec "${REPO_ROOT}/scripts/build_macos.sh"
      ;;
    android)
      exec "${REPO_ROOT}/scripts/build_android.sh"
      ;;
    *) return 0 ;;
  esac
}

########################################
# CMake configure and deploy (centralized for linux/windows)
########################################

# Internal: configure CMake for Windows cross-build.
_configure_cmake_windows() {
  log "Configuring CMake for Windows (Qt: ${QT_WINDOWS_DIR}, vcpkg: ${VCPKG_DIR})..."
  if command_exists qt-cmake; then
    CMAKE_CMD="qt-cmake"
  elif [ -f "$QT_HOST_PATH/bin/qt-cmake" ]; then
    CMAKE_CMD="$QT_HOST_PATH/bin/qt-cmake"
  else
    error "qt-cmake not found. Required for Windows cross-compilation."
  fi
  VCPKG_OPENSSL_ROOT="$VCPKG_DIR/installed/$VCPKG_TARGET_TRIPLET"
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
  $CMAKE_CMD "${CMAKE_ARGS[@]}"
  if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    ln -sf "$BUILD_DIR/compile_commands.json" "$REPO_ROOT/compile_commands.json"
    log "  ✓ compile_commands.json linked for clangd/Cursor"
  fi
}

# Internal: configure CMake for Linux native build.
_configure_cmake_linux() {
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
  if command_exists protoc; then
    CMAKE_ARGS+=(-DProtobuf_PROTOC_EXECUTABLE="$(command -v protoc)")
  fi
  cmake "${CMAKE_ARGS[@]}"
  if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    ln -sf "$BUILD_DIR/compile_commands.json" "$REPO_ROOT/compile_commands.json"
    log "  ✓ compile_commands.json linked for clangd/Cursor"
  fi
}

# Configure CMake for platform. Usage: configure_cmake_for_platform <platform>
# platform: linux | windows. Requires BUILD_DIR, setup_linux_paths, check_qt_deps.
configure_cmake_for_platform() {
  case "${1:-$TARGET_PLATFORM}" in
    windows) _configure_cmake_windows ;;
    linux)   _configure_cmake_linux ;;
    *)       error "configure_cmake_for_platform: unsupported platform $1" ;;
  esac
}

# Internal: create Windows deploy directory.
_create_windows_deploy_dir() {
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
  for dll in Qt6Core Qt6Gui Qt6Widgets Qt6Network Qt6Sql Qt6Xml Qt6Multimedia Qt6WebSockets Qt6MultimediaWidgets Qt6Qml Qt6Quick Qt6QuickControls2 Qt6Svg; do
    [ -f "${QT_WINDOWS_DIR}/bin/${dll}.dll" ] && cp "${QT_WINDOWS_DIR}/bin/${dll}.dll" "$DEPLOY_DIR/" 2>/dev/null || true
  done
  for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    copied=
    for path in /usr/lib/gcc/x86_64-w64-mingw32 /usr/x86_64-w64-mingw32/lib; do
      [ ! -d "$path" ] && continue
      dll_path=$(find "$path" -name "$dll" 2>/dev/null | head -1)
      if [ -n "$dll_path" ] && [ -f "$dll_path" ]; then
        cp "$dll_path" "$DEPLOY_DIR/" && copied=1 && break
      fi
    done
    [ -z "$copied" ] && [ -f "${QT_WINDOWS_DIR}/bin/${dll}" ] && cp "${QT_WINDOWS_DIR}/bin/${dll}" "$DEPLOY_DIR/" 2>/dev/null || true
  done
  for subdir in platforms styles imageformats sqldrivers tls generic; do
    mkdir -p "$DEPLOY_DIR/plugins/$subdir"
    [ -d "${QT_WINDOWS_DIR}/plugins/$subdir" ] && cp "${QT_WINDOWS_DIR}/plugins/$subdir"/*.dll "$DEPLOY_DIR/plugins/$subdir/" 2>/dev/null || true
  done
  [ ! -f "$DEPLOY_DIR/plugins/platforms/qwindows.dll" ] && [ -d "${QT_WINDOWS_DIR}/plugins/platforms" ] && \
    qwin=$(find "${QT_WINDOWS_DIR}" -name "qwindows.dll" 2>/dev/null | head -1) && [ -n "$qwin" ] && cp "$qwin" "$DEPLOY_DIR/plugins/platforms/" 2>/dev/null || true
  VCPKG_BIN="${VCPKG_DIR}/installed/${VCPKG_TARGET_TRIPLET}/bin"
  [ -d "$VCPKG_BIN" ] && for pat in zlib libpng libjpeg; do cp "$VCPKG_BIN"/${pat}*.dll "$DEPLOY_DIR/" 2>/dev/null || true; done
  cat > "$DEPLOY_DIR/qt.conf" << 'EOF'
[Paths]
Plugins = plugins
EOF
  cp "$REPO_ROOT/scripts/pokerth_launcher.bat" "$DEPLOY_DIR/"
  cp "$REPO_ROOT/scripts/run_pokerth.sh" "$DEPLOY_DIR/"
  chmod +x "$DEPLOY_DIR/run_pokerth.sh"
  find "$DEPLOY_DIR" -name "*.dll" -exec chmod +x {} \;
  log "Windows deploy ready: $DEPLOY_DIR (Qt/MinGW/plugins copied)"
}

# Internal: create Linux deploy directory.
_create_linux_deploy_dir() {
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

# Create deploy directory for platform. Usage: create_deploy_for_platform <platform>
# Sets DEPLOY_DIR and BINARY_NAME. platform: linux | windows.
create_deploy_for_platform() {
  case "${1:-$TARGET_PLATFORM}" in
    windows) _create_windows_deploy_dir ;;
    linux)   _create_linux_deploy_dir ;;
    *)       error "create_deploy_for_platform: unsupported platform $1" ;;
  esac
}

# Usage: check_dependency <command> [setup_script]
# setup_script is used in error message (e.g. make setup-linux or scripts/setup.sh)
check_dependency() {
  local cmd="${1:?}"
  local setup_script="${2:-run the setup script}"
  if ! command_exists "$cmd"; then
    error "$cmd not found. Please run $setup_script first to install dependencies."
  fi
}

########################################
# Linux-specific Functions
########################################

# Setup Linux Qt paths based on target platform
# Usage: setup_linux_paths TARGET_PLATFORM [USE_AQT] [USE_VCPKG]
# Sets: QT_DIR, QT_WINDOWS_DIR, QT_HOST_PATH, VCPKG_TARGET_TRIPLET, MINGW_DIR
# Windows cross-build is x86_64 only (host tools are gcc_64); ARM is not supported.
setup_linux_paths() {
  local target_platform="${1:-linux}"
  local use_aqt="${2:-no}"
  local use_vcpkg="${3:-no}"

  if [ "$target_platform" = "windows" ]; then
    # Windows cross-compilation requires aqtinstall
    USE_AQT="yes"
    USE_VCPKG="yes"
    # Detect actual Qt Windows directory (Qt 6.9+ may use win64_mingw, older uses mingw_64)
    # Check which one actually exists
    if [ -d "$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw" ]; then
      QT_WINDOWS_DIR="$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw"
    elif [ -d "$QT_OUTPUT_DIR/$QT_VERSION/mingw_64" ]; then
      QT_WINDOWS_DIR="$QT_OUTPUT_DIR/$QT_VERSION/mingw_64"
    else
      # Fallback: try win64_mingw first for Qt 6.9+, then mingw_64
      if [[ "$QT_VERSION" =~ ^6\.(9|1[0-9]) ]]; then
        QT_WINDOWS_DIR="$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw"
      else
        QT_WINDOWS_DIR="$QT_OUTPUT_DIR/$QT_VERSION/mingw_64"
      fi
    fi
    # Linux host tools for MinGW build (x86_64 only; ARM not supported)
    QT_HOST_PATH="$QT_OUTPUT_DIR/$QT_VERSION/gcc_64"
    QT_DIR="$QT_WINDOWS_DIR"
    VCPKG_TARGET_TRIPLET="x64-mingw-static"
    MINGW_DIR="/usr/x86_64-w64-mingw32"
  else
    # Linux native build - directory is gcc_64 even though aqt uses linux_gcc_64
    QT_DIR="$QT_OUTPUT_DIR/$QT_VERSION/gcc_64"
    VCPKG_TARGET_TRIPLET="x64-linux"
  fi
}

########################################
# Common Setup Functions
########################################

# Install pipx via pip (fallback when package manager fails)
# Usage: install_pipx_via_pip
install_pipx_via_pip() {
  log "Installing pipx via pip..."
  if command_exists python3; then
    python3 -m pip install --user pipx --break-system-packages 2>/dev/null || \
    python3 -m pip install --user pipx
  elif command_exists python; then
    python -m pip install --user pipx --break-system-packages 2>/dev/null || \
    python -m pip install --user pipx
  else
    error "Python not found. Please install Python 3."
  fi
}

# Ensure pipx is in PATH
# Usage: ensure_pipx_path
ensure_pipx_path() {
  if command_exists python3; then
    python3 -m pipx ensurepath 2>/dev/null || true
  elif command_exists python; then
    python -m pipx ensurepath 2>/dev/null || true
  fi
  export PATH="$HOME/.local/bin:$PATH"
}

# Install Qt with optional modules (handles missing modules gracefully)
# Usage: install_qt_with_modules [platform] [version] [arch] [output_dir] [modules...]
install_qt_with_modules() {
  local platform="$1"
  local version="$2"
  local arch="$3"
  local output_dir="$4"
  shift 4
  local aqt_opts=""
  local modules=("$@")

  # For Windows, Qt 6.9+ uses win64_mingw instead of mingw_64
  if [ "$platform" = "windows" ] && [ "$arch" = "mingw_64" ]; then
    # Check which architecture is available
    local available_archs
    available_archs=$(aqt list-qt windows desktop --arch "$version" 2>/dev/null || echo "")
    if echo "$available_archs" | grep -q "win64_mingw"; then
      arch="win64_mingw"
      log "Using architecture: $arch (Qt 6.9+ format)"
    elif echo "$available_archs" | grep -q "mingw_64"; then
      arch="mingw_64"
      log "Using architecture: $arch (older Qt format)"
    else
      # Default based on version
      if [[ "$version" =~ ^6\.(9|1[0-9]) ]]; then
        arch="win64_mingw"
        log "Using architecture: $arch (Qt 6.9+ default)"
      else
        arch="mingw_64"
        log "Using architecture: $arch (older Qt default)"
      fi
    fi
  fi

  # For Linux, use linux_gcc_64 instead of gcc_64
  if [ "$platform" = "linux" ] && [ "$arch" = "gcc_64" ]; then
    # Check which architecture is available
    local available_archs
    available_archs=$(aqt list-qt linux desktop --arch "$version" 2>/dev/null || echo "")
    if echo "$available_archs" | grep -q "linux_gcc_64"; then
      arch="linux_gcc_64"
      log "Using architecture: $arch (Linux format)"
    elif echo "$available_archs" | grep -q "gcc_64"; then
      arch="gcc_64"
      log "Using architecture: $arch (fallback format)"
    else
      # Default to linux_gcc_64 for Linux
      arch="linux_gcc_64"
      log "Using architecture: $arch (Linux default)"
    fi
  fi

  if [ "$platform" = "windows" ]; then
    # Windows requires --autodesktop flag
    aqt_opts="$aqt_opts --autodesktop"
  fi

  if [ ${#modules[@]} -eq 0 ]; then
    # No modules, install base Qt only
    log "Installing Qt ${version} base (no additional modules) for ${arch}..."
    aqt install-qt "$aqt_opts" "$platform" desktop "$version" "$arch" --outputdir "$output_dir"
  else
    # Try with modules first
    log "Installing Qt ${version} with modules: ${modules[*]}..."
    local aqt_cmd
    aqt_cmd="aqt install-qt $aqt_opts $platform desktop $version $arch --outputdir $output_dir --modules ${modules[*]}"

    local aqt_output
    aqt_output=$(eval "$aqt_cmd" 2>&1) || {
      # Check if error is about missing modules
      if echo "$aqt_output" | grep -q "were not found"; then
        log "⚠ Some modules were not found for Qt ${version}"
        log "  Installing base Qt without optional modules..."
        aqt install-qt "$aqt_opts" "$platform" desktop "$version" "$arch" --outputdir "$output_dir"
      else
        # Other error, re-raise it
        echo "$aqt_output" >&2
        return 1
      fi
    }
  fi
}

# Setup pipx and aqtinstall
# Usage: setup_pipx_aqt [pkg_manager] [install_cmd]
# pkg_manager: "apt", "dnf", "pacman", etc. (optional, for apt install)
# install_cmd: command to install packages (optional, e.g. "sudo apt-get install -y")
setup_pipx_aqt() {
  local pkg_manager="${1:-}"
  local install_cmd="${2:-}"

  # Install pipx if not available
  if ! command_exists pipx; then
    log "Installing pipx..."

    # Try package manager first if available (apt, dnf, or pacman)
    if [ -n "$pkg_manager" ] && [ -n "$install_cmd" ] && [[ "$pkg_manager" =~ ^(apt|dnf|pacman)$ ]]; then
      $install_cmd pipx || {
        log "pipx not available via $pkg_manager, trying pip..."
        install_pipx_via_pip
      }
    else
      # No supported package manager, use pip
      install_pipx_via_pip
    fi

    ensure_pipx_path
  else
    log "pipx already installed"
  fi

  # Install aqtinstall via pipx (not available in apt)
  if ! command_exists aqt; then
    log "Installing aqtinstall via pipx..."
    pipx install aqtinstall
  else
    log "aqtinstall already installed"
  fi

  export PATH="$HOME/.local/bin:$PATH"
}

# Clone and bootstrap vcpkg at VCPKG_DIR when the vcpkg executable is missing.
# Handles interrupted/partial clones (e.g. Docker bind mounts). Does not git pull.
# For Windows/Linux flows that update and rebuild the binary, use setup_vcpkg().
ensure_vcpkg_clone_bootstrap_if_missing() {
  local root="${VCPKG_DIR:?ensure_vcpkg_clone_bootstrap_if_missing: VCPKG_DIR required}"
  if [ -f "$root/vcpkg" ]; then
    return 0
  fi
  mkdir -p "$(dirname "$root")"
  if [ -d "$root" ]; then
    if ! git -C "$root" rev-parse --is-inside-work-tree >/dev/null 2>&1 || \
       [ ! -f "$root/bootstrap-vcpkg.sh" ]; then
      log "Cleaning incomplete vcpkg directory: $root"
      rm -rf "$root"/.[!.]* "$root"/..?* "$root"/* 2>/dev/null || true
    fi
  fi
  if [ ! -d "$root/.git" ]; then
    log "Cloning vcpkg into $root ..."
    git clone https://github.com/microsoft/vcpkg.git "$root"
  fi
  log "Bootstrapping vcpkg ..."
  "$root/bootstrap-vcpkg.sh" -disableMetrics
}

# Setup vcpkg
# Usage: setup_vcpkg [triplet]
# If triplet not provided, will determine based on platform
# When VCPKG_DIR is a mount (e.g. Docker volume), the dir may exist but be empty; we clone when bootstrap-vcpkg.sh is missing.
setup_vcpkg() {
  local triplet="${1:-}"

  if [ ! -f "$VCPKG_DIR/bootstrap-vcpkg.sh" ]; then
    if [ -d "$VCPKG_DIR" ]; then
      log "vcpkg path exists but is incomplete (e.g. empty volume); populating..."
      rm -rf "${VCPKG_DIR:?}"/* "${VCPKG_DIR:?}"/.git 2>/dev/null || true
    else
      mkdir -p "$VCPKG_DIR"
    fi
    log "Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
  else
    log "vcpkg directory already exists"
    log "Updating vcpkg (git pull)..."
    (cd "$VCPKG_DIR" && git pull --rebase 2>/dev/null) || true
  fi

  # Force rebuild of vcpkg binary with -disableMetrics
  if [ -f "$VCPKG_DIR/vcpkg" ]; then
    log "Removing existing vcpkg binary to rebuild with -disableMetrics..."
    rm -f "$VCPKG_DIR/vcpkg"
  fi

  log "Bootstrapping vcpkg..."
  "$VCPKG_DIR/bootstrap-vcpkg.sh" -disableMetrics

  if [ -n "$triplet" ] && [[ "$triplet" == *mingw* ]]; then
    log "Applying vcpkg OpenSSL MinGW no-quic patch (avoids SIO_UDP_NETRESET build error)..."
    (cd "$VCPKG_DIR" && patch -p1 --forward < "${REPO_ROOT}/docs/patches/vcpkg-openssl-mingw-no-quic.patch") || true
  fi

  if [ -n "$triplet" ]; then
    log "Installing vcpkg dependencies (${triplet})..."
    vcpkg_install "$triplet"
  fi
}

# Run vcpkg install; on "File exists" (interrupted copy) remove partial package and retry once.
vcpkg_install() {
  local triplet="$1"
  local vcpkg_log
  vcpkg_log=$(mktemp) || exit 1
  trap "rm -f '$vcpkg_log'" RETURN

  if ! VCPKG_DISABLE_METRICS=1 "$VCPKG_DIR/vcpkg" install --no-print-usage \
    --triplet="$triplet" \
    --disable-metrics \
    "${VCPKG_PORTS[@]}" 2>&1 | tee "$vcpkg_log"; then
    if grep -q "File exists" "$vcpkg_log" && grep -q "packages/" "$vcpkg_log"; then
      local pkg_dir
      pkg_dir=$(grep "File exists" "$vcpkg_log" | grep -o 'packages/[^/]*' | head -1)
      if [ -n "$pkg_dir" ] && [ -d "$VCPKG_DIR/$pkg_dir" ]; then
        log "Removing partial package $pkg_dir (interrupted copy) and retrying install..."
        rm -rf "$VCPKG_DIR/$pkg_dir"
        VCPKG_DISABLE_METRICS=1 "$VCPKG_DIR/vcpkg" install --no-print-usage \
          --triplet="$triplet" \
          --disable-metrics \
          "${VCPKG_PORTS[@]}"
        return
      fi
    fi
    return 1
  fi
}

# Get vcpkg triplet for macOS
# Usage: get_vcpkg_triplet_macos
get_vcpkg_triplet_macos() {
  if is_arm64; then
    echo "arm64-osx"
  else
    echo "x64-osx"
  fi
}

# Check Qt dependencies for build (Linux/Windows only)
# Usage: check_qt_deps [platform] [use_aqt] [qt_dir] [setup_script]
# platform: "linux" or "windows"
# Note: macOS has its own check in build_macos.sh due to macdeployqt requirement
check_qt_deps() {
  local platform="${1:-linux}"
  local use_aqt="${2:-no}"
  local qt_dir="${3:-}"
  local setup_script="${4:-make setup-linux or scripts/setup.sh}"

  if is_yes "$use_aqt"; then
    if [ "$platform" = "windows" ]; then
      # Check both possible directory names (win64_mingw for Qt 6.9+, mingw_64 for older)
      local qt_windows_dir="${QT_WINDOWS_DIR:-}"
      local arch_name=""

      # First check if QT_WINDOWS_DIR is set and looks valid
      # Qt 6+ may not ship qmake in target kits; rely on CMake config instead
      if [ -n "$qt_windows_dir" ] && [ -d "$qt_windows_dir" ] && [ -f "$qt_windows_dir/lib/cmake/Qt6/Qt6Config.cmake" ]; then
        # Use the provided directory
        if [[ "$qt_windows_dir" == *"win64_mingw"* ]]; then
          arch_name="win64_mingw"
        else
          arch_name="mingw_64"
        fi
      else
        # Try win64_mingw first (Qt 6.9+)
        if [ -d "$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw" ] && [ -f "$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw/lib/cmake/Qt6/Qt6Config.cmake" ]; then
          qt_windows_dir="$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw"
          arch_name="win64_mingw"
        # Fall back to mingw_64 (older Qt versions)
        elif [ -d "$QT_OUTPUT_DIR/$QT_VERSION/mingw_64" ] && [ -f "$QT_OUTPUT_DIR/$QT_VERSION/mingw_64/lib/cmake/Qt6/Qt6Config.cmake" ]; then
          qt_windows_dir="$QT_OUTPUT_DIR/$QT_VERSION/mingw_64"
          arch_name="mingw_64"
        else
          # Neither exists, determine expected name based on Qt version
          if [[ "$QT_VERSION" =~ ^6\.(9|1[0-9]) ]]; then
            qt_windows_dir="$QT_OUTPUT_DIR/$QT_VERSION/win64_mingw"
            arch_name="win64_mingw"
          else
            qt_windows_dir="$QT_OUTPUT_DIR/$QT_VERSION/mingw_64"
            arch_name="mingw_64"
          fi
        fi
      fi

      local qt_host_path="${QT_HOST_PATH:-$QT_OUTPUT_DIR/$QT_VERSION/gcc_64}"

      if [ ! -d "$qt_windows_dir" ] || [ ! -f "$qt_windows_dir/lib/cmake/Qt6/Qt6Config.cmake" ]; then
        error "Qt Windows ($arch_name) not found at $qt_windows_dir. Please run $setup_script TARGET_PLATFORM=windows"
      fi
      if [ ! -d "$qt_host_path" ] || [ ! -f "$qt_host_path/bin/qt-cmake" ]; then
        error "Qt host tools not found at $qt_host_path. Please run $setup_script TARGET_PLATFORM=windows"
      fi
      export CMAKE_PREFIX_PATH="$qt_windows_dir"
      export Qt6_DIR="$qt_windows_dir/lib/cmake/Qt6"
      export PATH="$qt_host_path/bin:$PATH"
      log "✓ Qt Windows: $qt_windows_dir"
      log "✓ Qt Host: $qt_host_path"
    else
      if [ -z "$qt_dir" ]; then
        qt_dir="$QT_OUTPUT_DIR/$QT_VERSION/gcc_64"
      fi
      if [ ! -d "$qt_dir" ] || [ ! -f "$qt_dir/bin/qmake" ]; then
        error "Qt not found at $qt_dir. Please run $setup_script USE_AQT=yes"
      fi
      export CMAKE_PREFIX_PATH="$qt_dir"
      export Qt6_DIR="$qt_dir/lib/cmake/Qt6"
      log "✓ Qt: $qt_dir"
    fi
  else
    if [ "$platform" = "windows" ]; then
      error "Windows cross-compilation requires aqtinstall. Run $setup_script TARGET_PLATFORM=windows"
    fi
    # Try to find Qt6_DIR from system installation
    if [ -d "/usr/lib/x86_64-linux-gnu/cmake/Qt6" ]; then
      export Qt6_DIR="/usr/lib/x86_64-linux-gnu/cmake/Qt6"
    elif [ -d "/usr/lib64/cmake/Qt6" ]; then
      export Qt6_DIR="/usr/lib64/cmake/Qt6"
    elif [ -d "/usr/lib/cmake/Qt6" ]; then
      export Qt6_DIR="/usr/lib/cmake/Qt6"
    else
      error "Qt6 not found. Please install Qt6 development packages or run $setup_script USE_AQT=yes"
    fi
    log "✓ Qt6 (system): $Qt6_DIR"
  fi
}

# Check vcpkg dependencies
# Usage: check_vcpkg_deps [setup_script]
check_vcpkg_deps() {
  local setup_script="${1:-make setup-linux or scripts/setup.sh}"

  if [ ! -d "$VCPKG_DIR" ] || [ ! -f "$VCPKG_DIR/vcpkg" ]; then
    error "vcpkg not found at $VCPKG_DIR. Please run $setup_script USE_VCPKG=yes"
  fi
  export CMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
  export VCPKG_ROOT="$VCPKG_DIR"
  log "✓ vcpkg: $VCPKG_DIR"
}
