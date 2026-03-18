#!/usr/bin/env bash
# Ensure vcpkg (and optionally Qt for Windows) deps for Docker builds, then run make when requested.
# Used by scripts/build_docker.sh and by the Windows devcontainer (postCreateCommand).
# Usage: scripts/ensure_docker_deps.sh <target> [make args...]
#   target: windows | windows-installer | android | android-in-docker
# When run as root we chown the cache dir to vscode then exec make as vscode; otherwise exec make.
set -euo pipefail

# On error, print where we failed so 'make android-docker' logs are easier to debug.
trap 'e=$?; echo "ensure_docker_deps.sh failed (exit $e) at line $LINENO (target=$TARGET). Last step above: clone vcpkg, bootstrap vcpkg, or install_vcpkg_android.sh. Run the container and run this script manually to see full errors." >&2; exit $e' ERR

VSCODE_UID="${VSCODE_UID:-1000}"
VSCODE_GID="${VSCODE_GID:-1000}"

# Returns 0 if vcpkg at VCPKG_ROOT has binary, installed dir, and PORT installed for TRIPLET.
# vcpkg list outputs "port[features]:triplet@version" so match port at start of line followed by : or [
vcpkg_ready() {
  local vcpkg_root="$1" triplet="$2" port="$3"
  [ -f "$vcpkg_root/vcpkg" ] && [ -d "$vcpkg_root/installed" ] || return 1
  "$vcpkg_root/vcpkg" list --triplet="$triplet" 2>/dev/null | grep -qE "^${port}(\[|:)" || return 1
  return 0
}

# Clone and bootstrap vcpkg at VCPKG_ROOT if the binary is missing.
vcpkg_bootstrap_if_missing() {
  local vcpkg_root="$1"
  if [ ! -f "$vcpkg_root/vcpkg" ]; then
    echo "Cloning vcpkg into $vcpkg_root ..."
    git clone https://github.com/microsoft/vcpkg.git "$vcpkg_root"
    echo "Bootstrapping vcpkg ..."
    "$vcpkg_root/bootstrap-vcpkg.sh"
  fi
}

# If RUN_MAKE is set: as root chown CACHE_DIR and exec make as vscode; else exec make. EXTRA_ENV optional (e.g. SETUP_ALREADY_DONE=1).
run_make_if_requested() {
  local cache_dir="$1" run_make="${2:-}" extra_env="${3:-}"
  [ -z "$run_make" ] && return 0
  if [ "$(id -u)" -eq 0 ]; then
    [ -d "$cache_dir" ] && chown -R "${VSCODE_UID}:${VSCODE_GID}" "$cache_dir" 2>/dev/null || true
    exec runuser -u vscode -- env $extra_env make "$run_make" "$@"
  fi
  exec make "$run_make" "$@"
}

if [ $# -lt 1 ]; then
  echo "Usage: $0 <target> [make args...]" >&2
  echo "  target: windows | windows-installer | android | android-in-docker" >&2
  exit 1
fi

TARGET="$1"
shift

# Both platforms: ROOT = cache mount root, VCPKG_ROOT = $ROOT/vcpkg, CACHE_DIR = $ROOT (chown this).
# We consider vcpkg ready when this port is installed (last-installed so partial runs re-run and vcpkg resumes).
CHECK_PORT="protobuf"
# Windows: devcontainer mounts .../build at /opt/pokerth-windows (clone at ROOT/vcpkg).
# Android: build_docker mounts .../build/vcpkg at /opt/pokerth-android/vcpkg only (SDK/NDK/Qt stay in image).
case "$TARGET" in
  windows|windows-installer)
    ROOT="${ROOT:-/opt/pokerth-windows}"
    VCPKG_ROOT="${VCPKG_ROOT:-$ROOT/vcpkg}"
    CACHE_DIR="$ROOT"
    TRIPLET="${VCPKG_TRIPLET:-x64-mingw-static}"

    if ! vcpkg_ready "$VCPKG_ROOT" "$TRIPLET" "$CHECK_PORT"; then
      echo "Running setup (vcpkg + Qt)..."
      SKIP_SYSTEM_PACKAGES=yes \
        VCPKG_DIR="$VCPKG_ROOT" \
        QT_OUTPUT_DIR="$ROOT/Qt" \
        TARGET_PLATFORM=windows \
        USE_AQT=yes \
        USE_VCPKG=yes \
        ./scripts/setup.sh
    fi

    run_make_if_requested "$CACHE_DIR" "$TARGET" "SETUP_ALREADY_DONE=1" "$@"
    ;;

  android|android-in-docker)
    ROOT="${ROOT:-/opt/pokerth-android}"
    VCPKG_ROOT="${VCPKG_ROOT:-$ROOT/vcpkg}"
    CACHE_DIR="$ROOT"
    TRIPLET="${VCPKG_ARCH:-arm64}-android"

    if ! vcpkg_ready "$VCPKG_ROOT" "$TRIPLET" "$CHECK_PORT"; then
      echo "Setting up Android vcpkg (clone + bootstrap + install + protobuf overlay)..."
      vcpkg_bootstrap_if_missing "$VCPKG_ROOT"
      # When root: chown ROOT to vscode (best-effort) then run install as vscode so vcpkg-overlays and vcpkg mount are writable.
      if [ "$(id -u)" -eq 0 ]; then
        [ -d "$ROOT" ] && chown -R "${VSCODE_UID}:${VSCODE_GID}" "$ROOT" 2>/dev/null || true
        ROOT="$ROOT" VCPKG_ROOT="$VCPKG_ROOT" runuser -u vscode -- docker/android/.devcontainer/install_vcpkg_android.sh
      else
        ROOT="$ROOT" VCPKG_ROOT="$VCPKG_ROOT" docker/android/.devcontainer/install_vcpkg_android.sh
      fi
      echo "Android vcpkg setup done."
    fi

    run_make_target=""
    run_make_extra_env=""
    [ "$TARGET" = "android-in-docker" ] && { run_make_target="android-in-docker"; run_make_extra_env="SETUP_ALREADY_DONE=1"; }
    run_make_if_requested "$CACHE_DIR" "$run_make_target" "$run_make_extra_env" "$@"
    exit 0
    ;;

  *)
    echo "Unknown target: $TARGET (use windows, windows-installer, android, or android-in-docker)" >&2
    exit 1
    ;;
esac
