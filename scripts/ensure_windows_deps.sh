#!/usr/bin/env bash
# Run setup when vcpkg is missing or incomplete, then run make. Used by scripts/build_docker.sh
# and by the Windows devcontainer (postCreateCommand). Use from repo root: scripts/ensure_windows_deps.sh windows
# When run as root (e.g. from make windows-docker), we chown the cache dir to vscode after setup so the
# bind mount is not left root-owned; then we run make as vscode so repo artifacts are not root-owned.
set -euo pipefail

VCPKG_DIR="${VCPKG_DIR:-/opt/pokerth-windows/vcpkg}"
QT_OUTPUT_DIR="${QT_OUTPUT_DIR:-/opt/pokerth-windows/Qt}"
VCPKG_TRIPLET="${VCPKG_TRIPLET:-x64-mingw-static}"
CACHE_DIR="$(dirname "$VCPKG_DIR")"   # /opt/pokerth-windows
VSCODE_UID="${VSCODE_UID:-1000}"
VSCODE_GID="${VSCODE_GID:-1000}"

# Run setup if vcpkg binary or installed dir missing, or if a required port (e.g. protobuf) is not installed (incomplete/interrupted run)
vcpkg_ok=
if [ -f "$VCPKG_DIR/vcpkg" ] && [ -d "$VCPKG_DIR/installed" ]; then
  if "$VCPKG_DIR/vcpkg" list --triplet="$VCPKG_TRIPLET" 2>/dev/null | grep -q "^protobuf "; then
    vcpkg_ok=1
  fi
fi

if [ -z "${vcpkg_ok:-}" ]; then
  echo "Running setup (vcpkg + Qt)..."
  SKIP_SYSTEM_PACKAGES=yes \
    VCPKG_DIR="$VCPKG_DIR" \
    QT_OUTPUT_DIR="$QT_OUTPUT_DIR" \
    TARGET_PLATFORM=windows \
    USE_AQT=yes \
    USE_VCPKG=yes \
    ./scripts/setup.sh
fi

# If we are root (e.g. make windows-docker): chown cache so bind mount is not root-owned; then run make as vscode. The Makefile is the only place that creates the stamp; we set SETUP_ALREADY_DONE=1 so the stamp rule only touches the file (setup was already run above).
if [ "$(id -u)" -eq 0 ]; then
  [ -d "$CACHE_DIR" ] && chown -R "${VSCODE_UID}:${VSCODE_GID}" "$CACHE_DIR"
  exec runuser -u vscode -- env SETUP_ALREADY_DONE=1 make "$@"
fi

exec make "$@"
