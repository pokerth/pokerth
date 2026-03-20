#!/usr/bin/env bash
echo "DEPRECATED: Do not run this script. Use scripts/setup_android.sh, or TARGET_PLATFORM=android scripts/setup.sh, or make setup-android / make android-docker. Kept for reference only (same idea as docker/windows/build_windows.sh)." >&2

# Legacy copy of Android vcpkg port install; maintained path is scripts/setup_android.sh.
# Expects: ROOT, VCPKG_ROOT, VCPKG_TRIPLET.
set -euo pipefail

ROOT="${ROOT:-/opt/pokerth-android}"
VCPKG_ROOT="${VCPKG_ROOT:-$ROOT/vcpkg}"
TRIPLET="${VCPKG_TRIPLET:-arm64-android}"

"$VCPKG_ROOT/vcpkg" install \
  boost-system:${TRIPLET} \
  boost-filesystem:${TRIPLET} \
  boost-thread:${TRIPLET} boost-regex:${TRIPLET} \
  boost-chrono:${TRIPLET} \
  boost-date-time:${TRIPLET} boost-serialization:${TRIPLET} \
  boost-asio:${TRIPLET} \
  boost-interprocess:${TRIPLET} \
  boost-iostreams:${TRIPLET} \
  boost-program-options:${TRIPLET} \
  boost-lambda:${TRIPLET} \
  boost-foreach:${TRIPLET} \
  boost-uuid:${TRIPLET} \
  openssl:${TRIPLET} \
  protobuf:x64-linux

mkdir -p "$ROOT/vcpkg-overlays/protobuf"
cp -r "$VCPKG_ROOT/ports/protobuf/"* "$ROOT/vcpkg-overlays/protobuf/"
sed -i '1i\# Workaround für TLS-Emulation\nset(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-warn-execstack")\nset(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-warn-execstack")\n' "$ROOT/vcpkg-overlays/protobuf/portfile.cmake"

"$VCPKG_ROOT/vcpkg" remove "protobuf:${TRIPLET}" --recurse 2>/dev/null || true
rm -rf "$VCPKG_ROOT/buildtrees/protobuf"
rm -rf "$VCPKG_ROOT/packages/protobuf_${TRIPLET}"
rm -rf /tmp/vcpkg-buildtrees/protobuf

"$VCPKG_ROOT/vcpkg" install "protobuf:${TRIPLET}" \
  --overlay-ports="$ROOT/vcpkg-overlays/protobuf" \
  --x-buildtrees-root=/tmp/vcpkg-buildtrees \
  --no-binarycaching
