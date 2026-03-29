#!/usr/bin/env bash

###
# Dispatch setup to platform-specific scripts:
#   linux / windows → setup_linux.sh
#   anything else → setup_${TARGET_PLATFORM}.sh (e.g. macos, android)
# Run via make setup-<platform> or TARGET_PLATFORM=… ./scripts/setup.sh
# After resolving REPO_ROOT, cwd is set to REPO_ROOT so callers need not cd (e.g. Docker sudo bash -lc from $HOME).

set -euo pipefail

# shellcheck source=/dev/null
source "$(dirname "${BASH_SOURCE[0]}")/functions.sh"

# Sets TARGET_PLATFORM when unset (Darwin → macos, else linux); Makefile usually sets it explicitly.
default_target_platform_from_uname

setup_sh_usage() {
  echo "Usage: scripts/setup.sh [-h|--help] [args…]"
  echo "  Optional layer args: Android (setup_android.sh): all|toolchain|deps. Windows cross (setup_linux.sh): all|toolchain|deps."
  echo "  TARGET_PLATFORM must be set (or inferred). Prefer: make setup-linux, make setup-windows, make setup-android, make setup-macos"
  echo ""
  echo "Environment: TARGET_PLATFORM, USE_AQT, USE_VCPKG, VCPKG_DIR / BUILD_DIR (android and windows), VCPKG_TRIPLET (android)"
  echo ""
  echo "Examples:"
  echo "  make setup-linux"
  echo "  TARGET_PLATFORM=windows scripts/setup.sh"
  echo "  TARGET_PLATFORM=android scripts/setup.sh toolchain   # Docker image build (first RUN); then deps (second RUN)"
  echo "  TARGET_PLATFORM=windows scripts/setup.sh toolchain    # Docker: toolchain in image; deps on docker run"
  echo ""
  echo "Linux/Windows host installs: scripts/setup_linux.sh (same as via this script)."
  echo "Then run make linux or make windows to build."
}

case "${1:-}" in
  -h|--help)
    setup_sh_usage
    exit 0
    ;;
esac

# Repo root for relative tools and child scripts; required when callers use e.g. `bash -lc` (cwd may be $HOME, not WORKDIR).
cd "$REPO_ROOT"

case "${TARGET_PLATFORM}" in
  linux|windows)
    exec "${REPO_ROOT}/scripts/setup_linux.sh" "$@"
    ;;
  android)
    exec "${REPO_ROOT}/scripts/setup_android.sh" "$@"
    ;;
  *)
    exec "${REPO_ROOT}/scripts/setup_${TARGET_PLATFORM}.sh" "$@"
    ;;
esac
