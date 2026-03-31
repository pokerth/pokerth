#!/usr/bin/env bash

###
# Shared functions and configuration for PokerTH
# Sourced by setup and build scripts. Do not execute directly.

########################################
# Common Configuration
########################################

# Canonical version pins: scripts/versions.env (repo file; sourced once when this file is sourced).
_func_dir="$(cd "$(dirname "${BASH_SOURCE[0]:-.}")" && pwd)"
# shellcheck source=/dev/null
. "$_func_dir/versions.env"

# Qt installation directory
QT_OUTPUT_DIR="${QT_OUTPUT_DIR:-$HOME/Qt}"

# vcpkg directory
VCPKG_DIR="${VCPKG_DIR:-$HOME/vcpkg}"

if [ "$(basename "$_func_dir")" = "scripts" ]; then
  REPO_ROOT="${REPO_ROOT:-$(cd "$_func_dir/.." && pwd)}"
  SCRIPT_DIR="${SCRIPT_DIR:-$_func_dir}"
else
  REPO_ROOT="${REPO_ROOT:-$_func_dir}"
fi
unset _func_dir

# Default basename for the Android handoff file (written by setup_android.sh, sourced by build_android.sh only).
MANIFEST_ENV="${MANIFEST_ENV:-.manifest.env}"

# Extra aqt modules (setup_linux / setup_macos); base Qt covers most of CMakeLists.
QT_MODULES=(qtwebsockets qtmultimedia)

########################################
# vcpkg Ports List
########################################

# Boost/protobuf/openssl/abseil — see CMakeLists.txt / protobuf on Windows+macOS
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
# curl for downloads: adjust timeouts/retries here.
########################################
CURL_CMD="curl --connect-timeout 15 --max-time 180 --retry 3 --retry-delay 5 -fSL"

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

# Apt package lists: same merge as docker/*/Dockerfile base stage —
#   grep -hv '^#' scripts/apt-packages.txt scripts/${TARGET_PLATFORM}-apt-packages.txt
# Writes one package name per line (no comments) to stdout.
apt_packages_merged_lines() {
  local scripts_dir="${1:?apt_packages_merged_lines: scripts directory}"
  local target_platform="${2:?apt_packages_merged_lines: TARGET_PLATFORM}"
  local common="${scripts_dir}/apt-packages.txt"
  local platform="${scripts_dir}/${target_platform}-apt-packages.txt"
  [ -f "$common" ] && [ -f "$platform" ] || error "Missing $common or $platform (pair must match docker/*/Dockerfile)"
  grep -hv '^#' "$common" "$platform"
}

# True if var is "yes" or "true" (for USE_AQT, CLEAN, CREATE_INSTALLER, etc.)
is_yes() {
  local v="${1:-}"
  [ "$v" = "yes" ] || [ "$v" = "true" ]
}

########################################
# Platform (Makefile is the entry point; passes TARGET_PLATFORM)
########################################

# If TARGET_PLATFORM is unset, set from host OS (Darwin → macos, else linux).
default_target_platform_from_uname() {
  if [ -n "${TARGET_PLATFORM:-}" ]; then
    return 0
  fi
  case "$(uname -s)" in
    Darwin) TARGET_PLATFORM="macos" ;;
    *) TARGET_PLATFORM="linux" ;;
  esac
}

# Scripts that take no positional args: if any args, run callback and exit 0.
# Usage: exit_with_usage_if_args <callback> "$@"  (pass script's "$@" from main)
exit_with_usage_if_args() {
  local cb="${1:?}"
  shift
  if [ $# -eq 0 ]; then
    return 0
  fi
  "$cb"
  exit 0
}

# Modes: android (CLEAN only), macos|host (BUILD_TARGET default; host sets USE_AQT/USE_VCPKG defaults).
init_build_defaults() {
  local mode="${1:?init_build_defaults: mode (android|macos|host)}"
  CLEAN="${CLEAN:-no}"
  case "$mode" in
    android) ;;
    macos|host)
      BUILD_TARGET="${BUILD_TARGET:-pokerth_client}"
      if [ "$mode" = "host" ]; then
        USE_AQT="${USE_AQT:-no}"
        USE_VCPKG="${USE_VCPKG:-no}"
      fi
      ;;
    *) error "init_build_defaults: unknown mode '$mode' (use android, macos, or host)" ;;
  esac
}

# Export VCPKG_DIR and set USE_AQT/USE_VCPKG for setup (android, macos, setup_linux.sh).
# VCPKG_DIR: env > VCPKG_ROOT > (BUILD_DIR or build_<platform>)/vcpkg. Windows requires USE_AQT/USE_VCPKG yes; else default no.
resolve_setup_platform_env() {
  local platform="${1:-$TARGET_PLATFORM}"
  local default_build="${BUILD_DIR:-build_${platform}}"
  export VCPKG_DIR="${VCPKG_DIR:-${VCPKG_ROOT:-${default_build}/vcpkg}}"
  if [ "$platform" = "windows" ]; then
    USE_AQT="yes"
    USE_VCPKG="yes"
  else
    USE_AQT="${USE_AQT:-no}"
    USE_VCPKG="${USE_VCPKG:-no}"
  fi
}

########################################
# CMake configure and deploy (centralized for linux/windows)
########################################

# aqt installs Qt for Android under .../<ver>/android_<arch> and host tools under .../<ver>/gcc_64.
# Call before resolving qt-cmake or writing .manifest.env so QT_HOST_PATH matches the on-disk gcc_64 kit.
sync_qt_host_path_from_android_kit() {
  [[ -n "${QT_ANDROID_DIR:-}" ]] || return 0
  local host="${QT_ANDROID_DIR%/*}/gcc_64"
  [[ -x "${host}/bin/qt-cmake" ]] || return 0
  if [[ -z "${QT_HOST_PATH:-}" || ! -x "${QT_HOST_PATH}/bin/qt-cmake" ]]; then
    QT_HOST_PATH="$host"
    export QT_HOST_PATH
  fi
}

# Sets QT_CMAKE_CMD: qt-cmake on PATH, else ${QT_HOST_PATH}/bin/qt-cmake (after sync_qt_host_path_from_android_kit).
resolve_qt_cmake_cmd() {
  local err_msg="${1:-}"
  sync_qt_host_path_from_android_kit
  if command_exists qt-cmake; then
    QT_CMAKE_CMD="qt-cmake"
  elif [[ -n "${QT_HOST_PATH:-}" && -x "${QT_HOST_PATH}/bin/qt-cmake" ]]; then
    QT_CMAKE_CMD="${QT_HOST_PATH}/bin/qt-cmake"
  else
    error "${err_msg:-qt-cmake not found. Install Qt host tools (see docs/building-developer.md).}"
  fi
}

# Internal: configure CMake for Windows cross-build.
_configure_cmake_windows() {
  log "Configuring CMake for Windows (Qt: ${QT_WINDOWS_DIR}, vcpkg: ${VCPKG_DIR})..."
  resolve_qt_cmake_cmd "qt-cmake not found. Required for Windows cross-compilation."
  CMAKE_CMD="$QT_CMAKE_CMD"
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
  _link_compile_commands_to_repo_root
}

# Symlink build dir compile_commands.json to repo root for clangd/Cursor.
_link_compile_commands_to_repo_root() {
  if [[ -f "$BUILD_DIR/compile_commands.json" ]]; then
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
  _link_compile_commands_to_repo_root
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

# aqt install-qt arch string for platform windows|linux (echo one line).
_aqt_resolve_desktop_arch() {
  local platform="$1" version="$2" list
  list=$(aqt list-qt "$platform" desktop --arch "$version" 2>/dev/null || echo "")
  case "$platform" in
    windows)
      case "$list" in
        *win64_mingw*) echo win64_mingw ;;
        *mingw_64*) echo mingw_64 ;;
        *) [[ "$version" =~ ^6\.(9|1[0-9]) ]] && echo win64_mingw || echo mingw_64 ;;
      esac
      ;;
    *)
      case "$list" in
        *linux_gcc_64*) echo linux_gcc_64 ;;
        *gcc_64*) echo gcc_64 ;;
        *) echo linux_gcc_64 ;;
      esac
      ;;
  esac
}

# MinGW Qt desktop kit under QT_OUTPUT_DIR/QT_VERSION (dirs on disk, else aqt list / version fallback).
# Sets global QT_WINDOWS_DIR. Used for Windows cross-build setup and check_qt_deps fallbacks.
set_qt_windows_dir_mingw_kit() {
  local base="${QT_OUTPUT_DIR}/${QT_VERSION}"
  if [[ -d "$base/win64_mingw" ]]; then QT_WINDOWS_DIR="$base/win64_mingw"
  elif [[ -d "$base/mingw_64" ]]; then QT_WINDOWS_DIR="$base/mingw_64"
  else QT_WINDOWS_DIR="$base/$(_aqt_resolve_desktop_arch windows "$QT_VERSION")"; fi
}

# Sets QT_DIR, QT_WINDOWS_DIR, QT_HOST_PATH, VCPKG_TARGET_TRIPLET, MINGW_DIR (Windows cross: x86_64 host gcc_64 only).
setup_linux_paths() {
  local target_platform="${1:-linux}"

  if [ "$target_platform" = "windows" ]; then
    # Windows cross-compilation requires aqtinstall
    USE_AQT="yes"
    USE_VCPKG="yes"
    set_qt_windows_dir_mingw_kit
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

  case "$platform:$arch" in
    windows:mingw_64) arch="$(_aqt_resolve_desktop_arch windows "$version")"; log "Using aqt desktop arch: $arch" ;;
    linux:gcc_64) arch="$(_aqt_resolve_desktop_arch linux "$version")"; log "Using aqt desktop arch: $arch" ;;
  esac

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

_install_pipx_with_user_pip() {
  if command_exists python3; then
    python3 -m pip install --user pipx --break-system-packages 2>/dev/null || python3 -m pip install --user pipx
  elif command_exists python; then
    python -m pip install --user pipx --break-system-packages 2>/dev/null || python -m pip install --user pipx
  else
    error "Python not found. Please install Python 3."
  fi
}

# Setup pipx and aqtinstall
# Usage: setup_pipx_aqt [pkg_manager] [install_cmd]
# pkg_manager: "apt", "dnf", "pacman", etc. (optional, for apt install)
# install_cmd: command to install packages (optional, e.g. "sudo apt-get install -y")
setup_pipx_aqt() {
  local pkg_manager="${1:-}"
  local install_cmd="${2:-}"

  if ! command_exists pipx; then
    log "Installing pipx..."
    if [ -n "$pkg_manager" ] && [ -n "$install_cmd" ] && [[ "$pkg_manager" =~ ^(apt|dnf|pacman)$ ]]; then
      $install_cmd pipx || {
        log "pipx not available via $pkg_manager, trying pip..."
        _install_pipx_with_user_pip
      }
    else
      _install_pipx_with_user_pip
    fi
    if command_exists python3; then python3 -m pipx ensurepath 2>/dev/null || true
    elif command_exists python; then python -m pipx ensurepath 2>/dev/null || true
    fi
  else
    log "pipx already installed"
  fi

  # User/pip installs put `pipx` under ~/.local/bin; `pipx ensurepath` does not update this shell's PATH.
  export PATH="$HOME/.local/bin:$PATH"

  if ! command_exists aqt; then
    log "Installing aqtinstall via pipx..."
    pipx install aqtinstall
  else
    log "aqtinstall already installed"
  fi
}

# Clone and bootstrap vcpkg at VCPKG_DIR when the vcpkg executable is missing.
# Handles interrupted/partial clones (e.g. Docker bind mounts). Does not git pull.
# For Windows/Linux flows that update and rebuild the binary, use setup_vcpkg().
ensure_vcpkg_clone_bootstrap_if_missing() {
  local root="${VCPKG_DIR:?ensure_vcpkg_clone_bootstrap_if_missing: VCPKG_DIR required}"
  if [ -f "$root/vcpkg" ]; then
    return 0
  fi
  log "ensure_vcpkg_clone_bootstrap_if_missing: no vcpkg binary at $root/vcpkg (VCPKG_DIR=$root)"
  if [ "${VCPKG_BOOTSTRAP_DEBUG:-}" = "1" ] && [ -d "$root" ]; then
    ls -la "$root" 2>&1 | head -40 | while IFS= read -r line || [ -n "$line" ]; do log "  $line"; done
    log "  git: $(git -C "$root" rev-parse --is-inside-work-tree 2>&1) bootstrap: $([ -f "$root/bootstrap-vcpkg.sh" ] && echo ok || echo missing)"
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
  # Clean working tree before bootstrap (dirty tree can break scripts/bootstrap).
  log "Resetting vcpkg to HEAD ..."
  (cd "$root" && git fetch --all && git reset --hard '@{upstream}')
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
    (cd "$VCPKG_DIR" && patch -p1 --forward < "${REPO_ROOT}/scripts/patches/vcpkg-openssl-mingw-no-quic.patch") || true
  fi

  if [ -n "$triplet" ]; then
    log "Installing vcpkg dependencies (${triplet})..."
    vcpkg_install "$triplet" "${VCPKG_PORTS[@]}"
  fi
}

# Run vcpkg install; on "File exists" (interrupted copy) remove partial package and retry once.
# Usage: vcpkg_install <triplet> [port port:triplet ...]
vcpkg_install() {
  local triplet="$1"
  shift
  local ports=("${@:-${VCPKG_PORTS[@]}}")
  local vcpkg_log
  vcpkg_log=$(mktemp) || exit 1
  trap "rm -f '$vcpkg_log'" RETURN

  if ! VCPKG_DISABLE_METRICS=1 "$VCPKG_DIR/vcpkg" install --no-print-usage \
    --triplet="$triplet" \
    --disable-metrics \
    "${ports[@]}" 2>&1 | tee "$vcpkg_log"; then
    if grep -q "File exists" "$vcpkg_log" && grep -q "packages/" "$vcpkg_log"; then
      local pkg_dir
      pkg_dir=$(grep "File exists" "$vcpkg_log" | grep -o 'packages/[^/]*' | head -1)
      if [ -n "$pkg_dir" ] && [ -d "$VCPKG_DIR/$pkg_dir" ]; then
        log "Removing partial package $pkg_dir (interrupted copy) and retrying install..."
        rm -rf "$VCPKG_DIR/$pkg_dir"
        VCPKG_DISABLE_METRICS=1 "$VCPKG_DIR/vcpkg" install --no-print-usage \
          --triplet="$triplet" \
          --disable-metrics \
          "${ports[@]}"
        return
      fi
    fi
    return 1
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
      # Qt 6+ MinGW kit: use Qt6Config.cmake (same rules as set_qt_windows_dir_mingw_kit).
      local qt_windows_dir="" arch_name="" base="${QT_OUTPUT_DIR}/${QT_VERSION}"
      if [[ -n "${QT_WINDOWS_DIR:-}" && -d "${QT_WINDOWS_DIR}" && -f "${QT_WINDOWS_DIR}/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
        qt_windows_dir="$QT_WINDOWS_DIR"
      else
        local cand
        for cand in "$base/win64_mingw" "$base/mingw_64"; do
          if [[ -f "$cand/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
            qt_windows_dir="$cand"
            break
          fi
        done
        if [[ -z "$qt_windows_dir" ]]; then
          set_qt_windows_dir_mingw_kit
          qt_windows_dir="$QT_WINDOWS_DIR"
        fi
      fi
      arch_name="${qt_windows_dir##*/}"

      local qt_host_path="${QT_HOST_PATH:-$QT_OUTPUT_DIR/$QT_VERSION/gcc_64}"

      if [[ ! -d "$qt_windows_dir" || ! -f "$qt_windows_dir/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
        error "Qt Windows ($arch_name) not found at $qt_windows_dir. Please run $setup_script TARGET_PLATFORM=windows"
      fi
      if [[ ! -d "$qt_host_path" || ! -f "$qt_host_path/bin/qt-cmake" ]]; then
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
