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
# Linux/Windows target detection
########################################

# Set TARGET_PLATFORM, SETUP_SCRIPT, BUILD_SCRIPT. TARGET_PLATFORM env overrides;
# otherwise derived from script name ($0) when called from scripts/setup.sh / scripts/build.sh.
detect_target_platform_linux() {
  local name
  name="$(basename "${1:-}")"
  if [[ "$name" == *windows* ]]; then
    TARGET_PLATFORM="${TARGET_PLATFORM:-windows}"
  else
    TARGET_PLATFORM="${TARGET_PLATFORM:-linux}"
  fi
  SETUP_SCRIPT="scripts/setup.sh"
  BUILD_SCRIPT="scripts/build.sh"
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
    # Linux host tools use linux_gcc_64, but directory is still gcc_64
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

  if [ ${#modules[@]} -eq 0 ]; then
    # No modules, install base Qt only
    log "Installing Qt ${version} base (no additional modules) for ${arch}..."
    if [ "$platform" = "windows" ]; then
      # Windows requires --autodesktop flag
      aqt install-qt "$platform" desktop "$version" "$arch" --outputdir "$output_dir" --autodesktop
    else
      aqt install-qt "$platform" desktop "$version" "$arch" --outputdir "$output_dir"
    fi
  else
    # Try with modules first
    log "Installing Qt ${version} with modules: ${modules[*]}..."
    local aqt_cmd
    if [ "$platform" = "windows" ]; then
      aqt_cmd="aqt install-qt $platform desktop $version $arch --outputdir $output_dir --autodesktop --modules ${modules[*]}"
    else
      aqt_cmd="aqt install-qt $platform desktop $version $arch --outputdir $output_dir --modules ${modules[*]}"
    fi

    local aqt_output
    aqt_output=$(eval "$aqt_cmd" 2>&1) || {
      # Check if error is about missing modules
      if echo "$aqt_output" | grep -q "were not found"; then
        log "⚠ Some modules were not found for Qt ${version}"
        log "  Installing base Qt without optional modules..."
        if [ "$platform" = "windows" ]; then
          aqt install-qt "$platform" desktop "$version" "$arch" --outputdir "$output_dir" --autodesktop
        else
          aqt install-qt "$platform" desktop "$version" "$arch" --outputdir "$output_dir"
        fi
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

  if ! VCPKG_DISABLE_METRICS=1 "$VCPKG_DIR/vcpkg" install \
    --triplet="$triplet" \
    --disable-metrics \
    "${VCPKG_PORTS[@]}" 2>&1 | tee "$vcpkg_log"; then
    if grep -q "File exists" "$vcpkg_log" && grep -q "packages/" "$vcpkg_log"; then
      local pkg_dir
      pkg_dir=$(grep "File exists" "$vcpkg_log" | grep -o 'packages/[^/]*' | head -1)
      if [ -n "$pkg_dir" ] && [ -d "$VCPKG_DIR/$pkg_dir" ]; then
        log "Removing partial package $pkg_dir (interrupted copy) and retrying install..."
        rm -rf "$VCPKG_DIR/$pkg_dir"
        VCPKG_DISABLE_METRICS=1 "$VCPKG_DIR/vcpkg" install \
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
        error "Qt host tools (gcc_64) not found at $qt_host_path. Please run $setup_script TARGET_PLATFORM=windows"
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
