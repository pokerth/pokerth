#!/usr/bin/env bash
# Run setup when vcpkg is missing or incomplete, then run make. Used by scripts/build_docker.sh (--setup-if-missing)
# and by the Windows devcontainer (postCreateCommand). Use from repo root: scripts/ensure_windows_deps.sh windows
set -euo pipefail

VCPKG_DIR="${VCPKG_DIR:-/opt/pokerth-windows/vcpkg}"
QT_OUTPUT_DIR="${QT_OUTPUT_DIR:-/opt/pokerth-windows/Qt}"
VCPKG_TRIPLET="${VCPKG_TRIPLET:-x64-mingw-static}"

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

exec make "$@"
