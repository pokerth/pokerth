#!/usr/bin/env bash
# vcpkg clone + Android-triplet ports for PokerTH (Docker ensure + host: make setup-android).
# Env: VCPKG_DIR (vcpkg root), VCPKG_TRIPLET (e.g. arm64-android), ROOT (cache root for overlays; default: parent of VCPKG_DIR).
set -euo pipefail

VCPKG_ROOT="${VCPKG_DIR:?Set VCPKG_DIR to the vcpkg directory for Android}"
ROOT="${ROOT:-$(dirname "$VCPKG_ROOT")}"
TRIPLET="${VCPKG_TRIPLET:-arm64-android}"

if [ ! -f "$VCPKG_ROOT/vcpkg" ]; then
  mkdir -p "$(dirname "$VCPKG_ROOT")"
  if [ -d "$VCPKG_ROOT" ]; then
    # Interrupted clones can leave partial checkouts.
    # Require both a valid git work tree and bootstrap script; otherwise recreate.
    if ! git -C "$VCPKG_ROOT" rev-parse --is-inside-work-tree >/dev/null 2>&1 || \
       [ ! -f "$VCPKG_ROOT/bootstrap-vcpkg.sh" ]; then
      echo "Cleaning incomplete vcpkg directory: $VCPKG_ROOT"
      # VCPKG_ROOT can be a bind-mount root inside Docker; deleting the mount point fails.
      # Clear contents instead, including dotfiles.
      rm -rf "$VCPKG_ROOT"/.[!.]* "$VCPKG_ROOT"/..?* "$VCPKG_ROOT"/* 2>/dev/null || true
    fi
  fi
  if [ ! -d "$VCPKG_ROOT/.git" ]; then
    echo "Cloning vcpkg into $VCPKG_ROOT ..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
  fi
  echo "Bootstrapping vcpkg ..."
  "$VCPKG_ROOT/bootstrap-vcpkg.sh"
fi

"$VCPKG_ROOT/vcpkg" install \
  boost-system:"${TRIPLET}" \
  boost-filesystem:"${TRIPLET}" \
  boost-thread:"${TRIPLET}" boost-regex:"${TRIPLET}" \
  boost-chrono:"${TRIPLET}" \
  boost-date-time:"${TRIPLET}" boost-serialization:"${TRIPLET}" \
  boost-asio:"${TRIPLET}" \
  boost-interprocess:"${TRIPLET}" \
  boost-iostreams:"${TRIPLET}" \
  boost-program-options:"${TRIPLET}" \
  boost-lambda:"${TRIPLET}" \
  boost-foreach:"${TRIPLET}" \
  boost-uuid:"${TRIPLET}" \
  openssl:"${TRIPLET}" \
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

echo "Android vcpkg setup complete (${TRIPLET})."
