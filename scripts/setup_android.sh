#!/usr/bin/env bash
# Android setup: provision SDK/NDK/Gradle/Qt (host) + vcpkg ports. Used by make setup-android and Docker ensure.
# Env: VCPKG_DIR (required), VCPKG_TRIPLET, ROOT, BUILD_DIR, SKIP_QT_INSTALL.
# Requires scripts/versions.env (Qt/Android/Gradle version pins).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
if [ ! -f "$SCRIPT_DIR/versions.env" ]; then
  echo "ERROR: $SCRIPT_DIR/versions.env is required for setup_android.sh"
  exit 1
fi
# shellcheck source=/dev/null
. "$SCRIPT_DIR/versions.env"
for _v in QT_VERSION GRADLE_VERSION ANDROID_NDK_VERSION ANDROID_BUILD_TOOLS_VERSION \
          ANDROID_CMAKE_VERSION ANDROID_CMDLINE_TOOLS_VERSION ANDROID_TARGET_SDK_VERSION; do
  if [ -z "${!_v:-}" ]; then
    echo "ERROR: $_v must be set in scripts/versions.env"
    exit 1
  fi
done
unset _v

VCPKG_ROOT="${VCPKG_DIR:?Set VCPKG_DIR to the vcpkg directory for Android}"
export VCPKG_DIR="$VCPKG_ROOT"
# shellcheck source=/dev/null
source "$SCRIPT_DIR/functions.sh"

ROOT="${ROOT:-$(dirname "$VCPKG_ROOT")}"
TRIPLET="${VCPKG_TRIPLET:-arm64-android}"
BUILD_DIR="${BUILD_DIR:-build_android}"
QT_ARCH="${QT_ARCH:-arm64_v8a}"

# ANDROID_CACHE_ROOT: where SDK/NDK/Qt live. Default $HOME/.pokerth-android (persists across make clean). Docker sets it to ROOT.
ANDROID_CACHE_ROOT="${ANDROID_CACHE_ROOT:-$HOME/.pokerth-android}"

# Resolve ANDROID_SDK_ROOT, ANDROID_NDK_ROOT, JAVA_HOME for later use and for .android_env
need_provision_sdk=0
if [ -z "${ANDROID_SDK_ROOT:-}" ]; then
  ANDROID_SDK_ROOT="${ANDROID_CACHE_ROOT}/android-sdk"
  ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}"
  need_provision_sdk=1
elif [ -z "${ANDROID_NDK_ROOT:-}" ]; then
  ANDROID_NDK_ROOT="${ANDROID_SDK_ROOT}/ndk/${ANDROID_NDK_VERSION}"
fi

TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake"
if [ "$need_provision_sdk" = "1" ] || [ ! -f "$TOOLCHAIN_FILE" ]; then
  need_provision_sdk=1
fi

# Export for aqt, vcpkg, and child processes (ANDROID_NDK_HOME = NDK_ROOT, some tools expect either)
export ANDROID_SDK_ROOT ANDROID_NDK_ROOT ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"

if [ "$need_provision_sdk" = "1" ]; then
  echo "Provisioning Android SDK/NDK (${ANDROID_SDK_ROOT})..."

  # Install host packages (skip when SKIP_SYSTEM_PACKAGES=yes, e.g. Docker)
  if [ "${SKIP_SYSTEM_PACKAGES:-no}" != "yes" ] && [ -f "$SCRIPT_DIR/android-apt-packages.txt" ]; then
    echo "Installing Android host packages..."
    PKGS=$(grep -v '^#' "$SCRIPT_DIR/android-apt-packages.txt" | tr '\n' ' ')
    if command -v apt-get >/dev/null 2>&1; then
      sudo apt-get update
      sudo apt-get install -y $PKGS
    else
      echo "ERROR: apt-get required for package install. Install manually: $PKGS"
      exit 1
    fi
  fi

  # Resolve JAVA_HOME (prefer Java 17 for Android Gradle plugin)
  if [ -z "${JAVA_HOME:-}" ]; then
    for jdk in java-17-openjdk-amd64 java-17-openjdk; do
      if [ -d "/usr/lib/jvm/$jdk" ]; then
        JAVA_HOME="/usr/lib/jvm/$jdk"
        break
      fi
    done
  fi
  if [ -z "${JAVA_HOME:-}" ]; then
    echo "ERROR: Java 17 not found. Run setup with SKIP_SYSTEM_PACKAGES=no to install, or: sudo apt install openjdk-17-jdk"
    exit 1
  fi
  export JAVA_HOME

  mkdir -p "$ANDROID_SDK_ROOT"
  if [ ! -f "${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager" ]; then
    echo "Downloading Android command line tools..."
    CMDLINE_ZIP="commandlinetools-linux-${ANDROID_CMDLINE_TOOLS_VERSION}_latest.zip"
    curl_cmd "https://dl.google.com/android/repository/${CMDLINE_ZIP}" -o /tmp/cmdline-tools.zip
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
fi


# Provision Gradle (use persistent cache on host; ROOT in Docker)
GRADLE_ROOT="$([ "$need_provision_sdk" = "1" ] && echo "$ANDROID_CACHE_ROOT" || echo "$ROOT")"
GRADLE_DIR="${GRADLE_ROOT}/gradle/gradle-${GRADLE_VERSION}"
if [ ! -d "$GRADLE_DIR" ]; then
  echo "Provisioning Gradle ${GRADLE_VERSION}..."
  mkdir -p "$(dirname "$GRADLE_DIR")"
  curl_cmd "https://services.gradle.org/distributions/gradle-${GRADLE_VERSION}-bin.zip" -o /tmp/gradle.zip
  unzip -q /tmp/gradle.zip -d "$(dirname "$GRADLE_DIR")"
  rm /tmp/gradle.zip
fi

# Provision Qt (host only; Docker uses SKIP_QT_INSTALL=yes)
if [ "${SKIP_QT_INSTALL:-}" != "yes" ]; then
  QT_MODULES="
    qt3d qt5compat qtcharts qtconnectivity qtdatavis3d qtgraphs qtgrpc
    qthttpserver qtimageformats qtlocation qtlottie qtmultimedia qtnetworkauth
    qtpositioning qtquick3d qtquick3dphysics qtquicktimeline qtremoteobjects
    qtscxml qtsensors qtserialbus qtserialport qtshadertools qtspeech
    qtvirtualkeyboard qtwebchannel qtwebsockets qtwebview
  "
  QT_ANDROID_DIR="${QT_ANDROID_DIR:-${ANDROID_CACHE_ROOT}/Qt/${QT_VERSION}/android_${QT_ARCH}}"
  QT_HOST_PATH="${QT_HOST_PATH:-${ANDROID_CACHE_ROOT}/Qt/${QT_VERSION}/gcc_64}"
  if [ ! -f "${QT_ANDROID_DIR}/lib/cmake/Qt6/Qt6Config.cmake" ]; then
    echo "Provisioning Qt ${QT_VERSION} for Android..."
    command -v aqt >/dev/null || {
      echo "Installing aqtinstall..."
      python3 -m venv "${ANDROID_CACHE_ROOT}/venv" 2>/dev/null || true
      "${ANDROID_CACHE_ROOT}/venv/bin/pip" install --upgrade pip aqtinstall 2>/dev/null || true
      export PATH="${ANDROID_CACHE_ROOT}/venv/bin:$PATH"
    }
    mkdir -p "${ANDROID_CACHE_ROOT}/Qt"
    (cd "${ANDROID_CACHE_ROOT}/Qt" && aqt install-qt all_os android "${QT_VERSION}" "android_${QT_ARCH}" --autodesktop --modules ${QT_MODULES})
    (cd "${ANDROID_CACHE_ROOT}/Qt" && aqt install-qt linux desktop "${QT_VERSION}" linux_gcc_64 --modules ${QT_MODULES})
  fi
else
  # Docker: use ROOT-based paths from container env
  QT_ANDROID_DIR="${QT_ANDROID_DIR:-${ROOT}/Qt/${QT_VERSION}/android_${QT_ARCH}}"
  QT_HOST_PATH="${QT_HOST_PATH:-${ROOT}/Qt/${QT_VERSION}/gcc_64}"
fi

# Write env for build_android.sh to source
_write_android_env() {
  cat <<ENVEOF
export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT}"
export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT}"
export ANDROID_NDK_HOME="${ANDROID_NDK_ROOT}"
export JAVA_HOME="${JAVA_HOME:-}"
export QT_ANDROID_DIR="${QT_ANDROID_DIR}"
export QT_HOST_PATH="${QT_HOST_PATH}"
export VCPKG_ROOT="${VCPKG_ROOT}"
export PATH="${GRADLE_ROOT}/gradle/gradle-${GRADLE_VERSION}/bin:${ANDROID_SDK_ROOT}/platform-tools:${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin:${QT_HOST_PATH}/bin:\$PATH"
ENVEOF
}
ENV_FILE="${REPO_ROOT}/${BUILD_DIR}/.android_env"
mkdir -p "$(dirname "$ENV_FILE")"
_write_android_env > "$ENV_FILE"
# Docker: also write under ROOT so it survives repo bind-mount; build_android sources this when repo-local file is missing.
if [ -n "${ROOT:-}" ]; then
  _write_android_env > "${ROOT}/.android_env"
fi

ensure_vcpkg_clone_bootstrap_if_missing

"$VCPKG_ROOT/vcpkg" install --no-print-usage \
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
PROTOBUF_OVERLAY_STAMP="$ROOT/vcpkg-overlays/.stamp_protobuf_${TRIPLET}"

# Only remove+reinstall if overlay not yet applied (idempotent)
if [ ! -f "$PROTOBUF_OVERLAY_STAMP" ]; then
  cp -r "$VCPKG_ROOT/ports/protobuf/"* "$ROOT/vcpkg-overlays/protobuf/"
  sed -i '1i\# Workaround für TLS-Emulation\nset(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-warn-execstack")\nset(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-warn-execstack")\n' "$ROOT/vcpkg-overlays/protobuf/portfile.cmake"

  "$VCPKG_ROOT/vcpkg" remove "protobuf:${TRIPLET}" --recurse 2>/dev/null || true
  rm -rf "$VCPKG_ROOT/buildtrees/protobuf"
  rm -rf "$VCPKG_ROOT/packages/protobuf_${TRIPLET}"
  # FIXME: Fixed /tmp path can clash with parallel Android setups; use a private temp root (mktemp -d) and thread through --x-buildtrees-root.
  rm -rf /tmp/vcpkg-buildtrees/protobuf

  "$VCPKG_ROOT/vcpkg" install --no-print-usage "protobuf:${TRIPLET}" \
    --overlay-ports="$ROOT/vcpkg-overlays/protobuf" \
    --x-buildtrees-root=/tmp/vcpkg-buildtrees \
    --no-binarycaching
  # FIXME: how do we know to remove the stamp if we install the original protobuf?
  touch "$PROTOBUF_OVERLAY_STAMP"
fi

echo "Android vcpkg setup complete (${TRIPLET})."
