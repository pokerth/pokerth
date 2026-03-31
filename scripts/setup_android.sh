#!/usr/bin/env bash
# Android setup: SDK/NDK, Gradle, then deps (Qt aqt + vcpkg). Used by make setup-android and Docker ensure.
#
# "System packages" (SKIP_SYSTEM_PACKAGES) means ONLY OS package-manager installs: apt — same merge as
# docker/android/Dockerfile (grep apt-packages.txt + android-apt-packages.txt).
# and, on hosts that support it, brew — what a Dockerfile base stage or CI can RUN before setup. Not: sdkmanager (SDK/NDK),
# pip/aqt (Qt), Gradle zips, or vcpkg — those are separate steps in this script.
# CACHE / bind — mainly vcpkg tree under the repo cache / bind mount (protobuf overlay is part of vcpkg); SDK/NDK/Qt may live there too
# depending on path env, but they are not "system packages" in the name above.
# Args: optional first argument all|toolchain|deps (default is all).
# Env: VCPKG_DIR / VCPKG_TRIPLET, ROOT, BUILD_DIR, SKIP_SYSTEM_PACKAGES, optional QT_OUTPUT_DIR
# (optional QT_OUTPUT_DIR; else ${ANDROID_CACHE_ROOT}/Qt). Android manifest: ${MANIFEST_ENV} (default .manifest.env).
# Native host: leave SKIP_SYSTEM_PACKAGES unset so the apt block runs (Linux). macOS: no apt here — brew/manual or
# SKIP_SYSTEM_PACKAGES=yes if already satisfied. Docker: ENV SKIP_SYSTEM_PACKAGES=yes when base already ran apt list.
set -euo pipefail

# shellcheck source=/dev/null
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/functions.sh"

provision_android_sdk_ndk() {
  echo "Provisioning Android SDK/NDK (${ANDROID_SDK_ROOT})..."

  # apt-only block (SKIP_SYSTEM_PACKAGES=yes skips this — e.g. Docker base already ran the same lists).
  if [ "${SKIP_SYSTEM_PACKAGES:-no}" != "yes" ]; then
    echo "Installing Android host packages..."
    PKGS="$(apt_packages_merged_lines "$SCRIPT_DIR" android | tr '\n' ' ')"
    if command -v apt-get >/dev/null 2>&1; then
      sudo apt-get update
      sudo apt-get install -y --no-install-recommends $PKGS
    else
      echo "ERROR: This script only runs apt on Linux. On macOS install the same tools (e.g. via brew) or set SKIP_SYSTEM_PACKAGES=yes if they are already installed. Packages: $PKGS"
      exit 1
    fi
  fi

  mkdir -p "$ANDROID_SDK_ROOT"
  if [ ! -f "${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager" ]; then
    echo "Downloading Android command line tools..."
    CMDLINE_ZIP="commandlinetools-linux-${ANDROID_CMDLINE_TOOLS_VERSION}_latest.zip"
    $CURL_CMD "https://dl.google.com/android/repository/${CMDLINE_ZIP}" -o /tmp/cmdline-tools.zip
    unzip -q /tmp/cmdline-tools.zip -d /tmp
    mkdir -p "${ANDROID_SDK_ROOT}/cmdline-tools/latest"
    if [ -d /tmp/cmdline-tools ]; then
      mv /tmp/cmdline-tools/* "${ANDROID_SDK_ROOT}/cmdline-tools/latest/"
    else
      mv /tmp/commandlinetools/* "${ANDROID_SDK_ROOT}/cmdline-tools/latest/" 2>/dev/null || true
    fi
    rm -rf /tmp/cmdline-tools /tmp/commandlinetools* /tmp/cmdline-tools.zip
  fi

  echo "Installing SDK components (platform-tools, build-tools, ndk, cmake)..."
  yes | "${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager" --sdk_root="${ANDROID_SDK_ROOT}" --licenses 2>/dev/null || true
  "${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager" --sdk_root="${ANDROID_SDK_ROOT}" \
    "platform-tools" \
    "platforms;android-${ANDROID_TARGET_SDK_VERSION}" \
    "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" \
    "ndk;${ANDROID_NDK_VERSION}" \
    "cmake;${ANDROID_CMAKE_VERSION}"
}

# Toolchain stage only: set Qt paths from ROOT/env without running aqt (deps/all run provision_qt_for_android).
# Qt base: optional QT_OUTPUT_DIR (e.g. devcontainer), else ${ANDROID_CACHE_ROOT}/Qt (ANDROID_CACHE_ROOT set below before use).
set_qt_paths_from_root_or_env() {
  local _qb
  _qb="${QT_OUTPUT_DIR:-${ANDROID_CACHE_ROOT}/Qt}"
  QT_ANDROID_DIR="${QT_ANDROID_DIR:-${_qb}/${QT_VERSION}/android_${QT_ARCH}}"
  QT_HOST_PATH="${QT_HOST_PATH:-${_qb}/${QT_VERSION}/gcc_64}}"
}

# Implemented only in this file (not scripts/functions.sh). Related: install_qt_with_modules / setup_pipx_aqt
# in functions.sh handle desktop Qt (linux/windows); Android needs all_os android + host linux_gcc_64 here.
provision_qt_for_android() {
  QT_MODULES="
    qt3d qt5compat qtcharts qtconnectivity qtdatavis3d qtgraphs qtgrpc
    qthttpserver qtimageformats qtlocation qtlottie qtmultimedia qtnetworkauth
    qtpositioning qtquick3d qtquick3dphysics qtquicktimeline qtremoteobjects
    qtscxml qtsensors qtserialbus qtserialport qtshadertools qtspeech
    qtvirtualkeyboard qtwebchannel qtwebsockets qtwebview
  "
  local _qb
  _qb="${QT_OUTPUT_DIR:-${ANDROID_CACHE_ROOT}/Qt}"
  QT_ANDROID_DIR="${QT_ANDROID_DIR:-${_qb}/${QT_VERSION}/android_${QT_ARCH}}"
  QT_HOST_PATH="${QT_HOST_PATH:-${_qb}/${QT_VERSION}/gcc_64}}"
  if [ ! -f "${QT_ANDROID_DIR}/lib/cmake/Qt6/Qt6Config.cmake" ]; then
    echo "Provisioning Qt ${QT_VERSION} for Android..."
    command -v aqt >/dev/null || {
      echo "Installing aqtinstall..."
      python3 -m venv "${ANDROID_CACHE_ROOT}/venv" 2>/dev/null || true
      "${ANDROID_CACHE_ROOT}/venv/bin/pip" install --upgrade pip aqtinstall 2>/dev/null || true
      export PATH="${ANDROID_CACHE_ROOT}/venv/bin:$PATH"
    }
    mkdir -p "$_qb"
    (cd "$_qb" && aqt install-qt all_os android "${QT_VERSION}" "android_${QT_ARCH}" --autodesktop --modules ${QT_MODULES})
    (cd "$_qb" && aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --modules ${QT_MODULES})
  fi
}

# Writes Android-specific exports into ${MANIFEST_ENV}.
_write_android_manifest() {
  sync_qt_host_path_from_android_kit
  cat <<ENVEOF
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}"
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}"
export ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"
export JAVA_HOME="${JAVA_HOME:-}"
export GRADLE_ROOT="${GRADLE_ROOT}"
export QT_ANDROID_DIR="${QT_ANDROID_DIR:-}"
export QT_HOST_PATH="${QT_HOST_PATH:-}"
export VCPKG_ROOT="${VCPKG_ROOT}"
export PATH="${GRADLE_ROOT}/gradle/gradle-${GRADLE_VERSION}/bin:${ANDROID_SDK_ROOT}/platform-tools:${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin${QT_HOST_PATH:+:${QT_HOST_PATH}/bin}:\$PATH"
ENVEOF
}

# When ROOT is the image bind root (/opt/pokerth-*), also write here so a repo workspace bind mount
# cannot hide ${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}.
_sync_manifest_to_root() {
  if [[ -n "${ROOT:-}" && "${ROOT}" =~ ^/opt/pokerth- ]]; then
    _write_android_manifest > "${ROOT}/${MANIFEST_ENV}"
  fi
}

# vcpkg ports + protobuf overlay live in the deps layer (setup.sh deps / docker run).
_setup_android_vcpkg_layer() {
  ensure_vcpkg_clone_bootstrap_if_missing

  # vcpkg ${TRIPLET} ports need ANDROID_NDK_ROOT (and SDK cmake) — already provisioned above. Qt is not required for boost/openssl/protobuf.
  # Host protobuf (x64-linux) uses the Linux compiler only.
  vcpkg_install "${TRIPLET}" \
    boost-system:"${TRIPLET}" boost-filesystem:"${TRIPLET}" \
    boost-thread:"${TRIPLET}" boost-regex:"${TRIPLET}" boost-chrono:"${TRIPLET}" \
    boost-date-time:"${TRIPLET}" boost-serialization:"${TRIPLET}" boost-asio:"${TRIPLET}" \
    boost-interprocess:"${TRIPLET}" boost-iostreams:"${TRIPLET}" boost-program-options:"${TRIPLET}" \
    boost-lambda:"${TRIPLET}" boost-foreach:"${TRIPLET}" boost-uuid:"${TRIPLET}" \
    openssl:"${TRIPLET}" \
    protobuf:x64-linux

  # Keep vcpkg overlay artifacts on the same tree as vcpkg itself
  # (VCPKG_DIR → docker/<kind>/build/vcpkg in repo; persistent via workspace bind in Docker/devcontainer).
  PROTOBUF_OVERLAY_DIR="$VCPKG_ROOT/vcpkg-overlays/protobuf/${TRIPLET}"
  mkdir -p "$PROTOBUF_OVERLAY_DIR"
  PROTOBUF_OVERLAY_HASH_FILE="$VCPKG_ROOT/vcpkg-overlays/.protobuf_overlay_hash_${TRIPLET}"
  PROTOBUF_INSTALLED_ROOT="$VCPKG_ROOT/installed/$TRIPLET"
  PROTOBUF_CONFIG_OK=0
  if [ -f "$PROTOBUF_INSTALLED_ROOT/lib/cmake/protobuf/ProtobufConfig.cmake" ] || \
     [ -f "$PROTOBUF_INSTALLED_ROOT/share/protobuf/protobuf-config.cmake" ] || \
     [ -f "$PROTOBUF_INSTALLED_ROOT/share/protobuf/ProtobufConfig.cmake" ]; then
    PROTOBUF_CONFIG_OK=1
  fi

  # Rebuild overlay dir each run, then hash it. Reinstall protobuf only if overlay hash changed
  # or if protobuf config for this triplet is missing/corrupt (interrupted install recovery).
  rm -rf "$PROTOBUF_OVERLAY_DIR"
  mkdir -p "$PROTOBUF_OVERLAY_DIR"
  cp -r "$VCPKG_ROOT/ports/protobuf/"* "$PROTOBUF_OVERLAY_DIR/"
  sed -i '1i\# Workaround für TLS-Emulation\nset(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-warn-execstack")\nset(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-warn-execstack")\n' "$PROTOBUF_OVERLAY_DIR/portfile.cmake"

  PROTOBUF_OVERLAY_HASH="$(
    cd "$PROTOBUF_OVERLAY_DIR"
    find . -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $1}'
  )"
  PREV_PROTOBUF_OVERLAY_HASH=""
  if [ -f "$PROTOBUF_OVERLAY_HASH_FILE" ]; then
    PREV_PROTOBUF_OVERLAY_HASH="$(cat "$PROTOBUF_OVERLAY_HASH_FILE")"
  fi

  if [ "$PROTOBUF_CONFIG_OK" != "1" ] || [ "$PROTOBUF_OVERLAY_HASH" != "$PREV_PROTOBUF_OVERLAY_HASH" ]; then
    if [ "$PROTOBUF_CONFIG_OK" != "1" ]; then
      echo "protobuf config missing; forcing reinstall for ${TRIPLET}."
    elif [ "$PROTOBUF_OVERLAY_HASH" != "$PREV_PROTOBUF_OVERLAY_HASH" ]; then
      echo "protobuf overlay changed; reinstalling for ${TRIPLET}."
    fi
    "$VCPKG_ROOT/vcpkg" remove "protobuf:${TRIPLET}" --recurse 2>/dev/null || true
    rm -rf "$VCPKG_ROOT/buildtrees/protobuf"
    rm -rf "$VCPKG_ROOT/packages/protobuf_${TRIPLET}"

    # Use default buildtrees ($VCPKG_ROOT/buildtrees) so protobuf build cache persists on bind mount; /tmp is ephemeral (lost on container restart).
    "$VCPKG_ROOT/vcpkg" install --no-print-usage "protobuf:${TRIPLET}" \
      --overlay-ports="$PROTOBUF_OVERLAY_DIR" \
      --no-binarycaching
    printf '%s\n' "$PROTOBUF_OVERLAY_HASH" > "$PROTOBUF_OVERLAY_HASH_FILE"
  fi

  echo "Android vcpkg setup complete (${TRIPLET})."
}

########################################
# Main
########################################

LAYER=""
case "${1:-}" in
  all|toolchain|deps)
    LAYER="$1"
    shift
    ;;
  "")
    ;;
  *)
    echo "setup_android.sh: unknown argument: $1 (expected all, toolchain, or deps)" >&2
    exit 1
    ;;
esac
if [ $# -gt 0 ]; then
  echo "setup_android.sh: unexpected argument: $1" >&2
  exit 1
fi
LAYER="${LAYER:-all}"
case "$LAYER" in
  all|toolchain|deps) ;;
  *)
    echo "Invalid layer: ${LAYER} (use all, toolchain, or deps)." >&2
    exit 1
    ;;
esac

# Second phase only: reuse toolchain output (same paths as _write_android_manifest below).
# Docker image build: toolchain writes ${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV} into the image layer; ${ROOT}/${MANIFEST_ENV}
# on the bind mount is optional. Prefer the repo-relative file so split Dockerfile RUN steps still see it.
if [ "$LAYER" = "deps" ]; then
  BUILD_DIR="${BUILD_DIR:-build_android}"
  resolve_setup_platform_env android
  export VCPKG_DIR
  export VCPKG_ROOT="$VCPKG_DIR"
  ROOT="${ROOT:-$(dirname "$VCPKG_DIR")}"
  TRIPLET="${VCPKG_TRIPLET:-arm64-android}"
  ENV_FILE="${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}"
  ANDROID_ENV_SOURCE=""
  if [ -f "$ENV_FILE" ]; then
    ANDROID_ENV_SOURCE="$ENV_FILE"
  elif [ -n "${ROOT:-}" ] && [ -f "${ROOT}/${MANIFEST_ENV}" ]; then
    ANDROID_ENV_SOURCE="${ROOT}/${MANIFEST_ENV}"
  fi
  if [ -z "$ANDROID_ENV_SOURCE" ]; then
    echo "deps layer requires ${MANIFEST_ENV} from toolchain (tried ${ENV_FILE} and ${ROOT:-ROOT}/${MANIFEST_ENV})." >&2
    exit 1
  fi
  set -a
  # shellcheck source=/dev/null
  . "$ANDROID_ENV_SOURCE"
  set +a

  # Qt (aqt) lives in deps, not toolchain. Refresh ANDROID_CACHE_ROOT if unset after source.
  if [ -z "${ANDROID_CACHE_ROOT:-}" ]; then
    if [[ -n "${ROOT:-}" && "${ROOT}" =~ ^/opt/pokerth- ]]; then
      ANDROID_CACHE_ROOT="$ROOT"
    else
      ANDROID_CACHE_ROOT="${HOME}/.pokerth-android"
    fi
  fi
  export QT_ARCH="${QT_ARCH:-arm64_v8a}"
  provision_qt_for_android
  export QT_ANDROID_DIR QT_HOST_PATH
  ENV_FILE="${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}"
  mkdir -p "$(dirname "$ENV_FILE")"
  _write_android_manifest > "$ENV_FILE"
  _sync_manifest_to_root

  _setup_android_vcpkg_layer
  exit 0
fi

BUILD_DIR="${BUILD_DIR:-build_android}"
resolve_setup_platform_env android
export VCPKG_DIR
export VCPKG_ROOT="$VCPKG_DIR"

ROOT="${ROOT:-$(dirname "$VCPKG_DIR")}"
TRIPLET="${VCPKG_TRIPLET:-arm64-android}"
QT_ARCH="${QT_ARCH:-arm64_v8a}"

# ANDROID_CACHE_ROOT: SDK/NDK/Qt/venv. Host: ~/.pokerth-android. When ROOT is the standard Docker bind root (/opt/pokerth-*), colocate caches on ROOT.
if [ -z "${ANDROID_CACHE_ROOT:-}" ]; then
  if [[ -n "${ROOT:-}" && "${ROOT}" =~ ^/opt/pokerth- ]]; then
    ANDROID_CACHE_ROOT="$ROOT"
  else
    ANDROID_CACHE_ROOT="${HOME}/.pokerth-android"
  fi
fi

# Resolve ANDROID_SDK_ROOT / ANDROID_NDK_ROOT for checks and for ${MANIFEST_ENV}.
# Important: don't force sdkmanager runs just because ANDROID_SDK_ROOT isn't set.
# In Docker runs, SDK/NDK are baked into the image; we only provision when something is actually missing.
if [ -z "${ANDROID_SDK_ROOT:-}" ]; then
  ANDROID_SDK_ROOT="${ANDROID_CACHE_ROOT}/android-sdk"
fi
if [ -z "${ANDROID_NDK_ROOT:-}" ]; then
  ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}"
fi

TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake"
SDKMANAGER_FILE="${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager"

missing_component=0
if [ ! -f "$TOOLCHAIN_FILE" ]; then missing_component=1; fi
if [ ! -f "$SDKMANAGER_FILE" ]; then missing_component=1; fi
if [ ! -d "${ANDROID_SDK_ROOT}/platform-tools" ]; then missing_component=1; fi
if [ ! -d "${ANDROID_SDK_ROOT}/platforms/android-${ANDROID_TARGET_SDK_VERSION}" ]; then missing_component=1; fi
if [ ! -d "${ANDROID_SDK_ROOT}/build-tools/${ANDROID_BUILD_TOOLS_VERSION}" ]; then missing_component=1; fi
if [ ! -d "${ANDROID_SDK_ROOT}/cmake/${ANDROID_CMAKE_VERSION}" ]; then missing_component=1; fi

# Export for aqt, vcpkg, and child processes (ANDROID_NDK_HOME = NDK_ROOT, some tools expect either)
export ANDROID_SDK_ROOT ANDROID_NDK_ROOT ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"

if [ "$missing_component" = "1" ]; then
  provision_android_sdk_ndk
fi

# Toolchain stage: Qt path hints in ${MANIFEST_ENV} only (aqt runs in deps or LAYER=all).
if [ "$LAYER" = "toolchain" ]; then
  set_qt_paths_from_root_or_env
fi

# Resolve JAVA_HOME (prefer Java 17 for Android Gradle plugin). Runs after SDK provision so host packages from provision_android_sdk_ndk are available when apt ran.
if [ -z "${JAVA_HOME:-}" ]; then
  for jdk in java-17-openjdk-amd64 java-17-openjdk; do
    if [ -d "/usr/lib/jvm/$jdk" ]; then
      JAVA_HOME="/usr/lib/jvm/$jdk"
      break
    fi
  done
fi
export JAVA_HOME

# Gradle: same Java/toolchain family as JDK + Android Gradle Plugin; ideally SYSTEM/image-baked, else cached under GRADLE_ROOT.
# GRADLE_ROOT: ANDROID_CACHE_ROOT when SDK was just provisioned; else ROOT (e.g. Docker bind). Exported via ${MANIFEST_ENV} for deps layer.
GRADLE_ROOT="$([ "$missing_component" = "1" ] && echo "$ANDROID_CACHE_ROOT" || echo "$ROOT")"
export GRADLE_ROOT
GRADLE_DIR="${GRADLE_ROOT}/gradle/gradle-${GRADLE_VERSION}"
if [ ! -d "$GRADLE_DIR" ]; then
  echo "Provisioning Gradle ${GRADLE_VERSION}..."
  mkdir -p "$(dirname "$GRADLE_DIR")"
  $CURL_CMD "https://services.gradle.org/distributions/gradle-${GRADLE_VERSION}-bin.zip" -o /tmp/gradle.zip
  unzip -q /tmp/gradle.zip -d "$(dirname "$GRADLE_DIR")"
  rm /tmp/gradle.zip
fi

# Write env for build_android.sh to source (build_android.sh: REPO_BUILD_ROOT/${MANIFEST_ENV} first, then ROOT/${MANIFEST_ENV}).
ENV_FILE="${REPO_ROOT}/${BUILD_DIR}/${MANIFEST_ENV}"
mkdir -p "$(dirname "$ENV_FILE")"
_write_android_manifest > "$ENV_FILE"
_sync_manifest_to_root

if [ "$LAYER" = "toolchain" ]; then
  echo "Android toolchain layer complete (SDK/NDK/Gradle + ${MANIFEST_ENV}). Qt (aqt) and vcpkg: run ./scripts/setup.sh deps or all."
  exit 0
fi

# LAYER=all: Qt (aqt when missing) then refresh ${MANIFEST_ENV}, then vcpkg.
provision_qt_for_android
export QT_ANDROID_DIR QT_HOST_PATH
_write_android_manifest > "$ENV_FILE"
_sync_manifest_to_root

_setup_android_vcpkg_layer
