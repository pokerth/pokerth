#!/usr/bin/env bash
# Install vcpkg ports for Android (PokerTH deps). Single source of truth for the port list.
# Used by docker/android/.devcontainer/Dockerfile and scripts/ensure_docker_deps.sh (android).
# Expects: ROOT, VCPKG_ROOT, VCPKG_ARCH.
set -euo pipefail

ROOT="${ROOT:-/opt/pokerth-android}"
VCPKG_ROOT="${VCPKG_ROOT:-$ROOT/vcpkg}"
VCPKG_ARCH="${VCPKG_ARCH:-arm64}"
TRIPLET="${VCPKG_ARCH}-android"

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
rm -rf "$VCPKG_ROOT/packages/protobuf_${VCPKG_ARCH}-android"
rm -rf /tmp/vcpkg-buildtrees/protobuf

"$VCPKG_ROOT/vcpkg" install "protobuf:${TRIPLET}" \
  --overlay-ports="$ROOT/vcpkg-overlays/protobuf" \
  --x-buildtrees-root=/tmp/vcpkg-buildtrees \
  --no-binarycaching
