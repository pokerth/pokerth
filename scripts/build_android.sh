#!/usr/bin/env bash
set -euo pipefail

# Minimaler Android-Build-Helper für ${TARGET} (Template)
# Erwartet als Umgebungsvariablen:
#  ANDROID_SDK_ROOT, ANDROID_NDK_ROOT, JAVA_HOME, QT_ANDROID_DIR
# shellcheck source=/dev/null
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/functions.sh"

cd "$REPO_ROOT"

# Source manifest written by setup_android.sh (ANDROID_SDK_ROOT, NDK, JAVA_HOME, QT_ANDROID_DIR, PATH, etc.).
# Repo-local first, then ${ROOT} (Docker sync). At docker run the host file wins over anything baked at docker build.
# Likely failure: no file here after a fresh clone — run setup-android or ensure (deps) first.
_base="${REPO_ROOT}/${REPO_BUILD_ROOT:-build_android}"
if [ -f "${_base}/${MANIFEST_ENV:-.manifest.env}" ]; then
  # shellcheck source=/dev/null
  . "${_base}/${MANIFEST_ENV:-.manifest.env}"
elif [ -n "${ROOT:-}" ] && [ -f "${ROOT}/${MANIFEST_ENV:-.manifest.env}" ]; then
  # shellcheck source=/dev/null
  . "${ROOT}/${MANIFEST_ENV:-.manifest.env}"
fi
unset _base

ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-}"
ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-}"
JAVA_HOME="${JAVA_HOME:-}"
QT_ANDROID_DIR="${QT_ANDROID_DIR:-}"
QT_HOST_PATH="${QT_HOST_PATH:-}"

# Stamp can exist without .manifest.env (e.g. manifest deleted, SETUP_ALREADY_DONE=touch-only). Avoid ${ANDROID_SDK_ROOT}/build-tools → "/build-tools".
if [[ -z "${ANDROID_SDK_ROOT}" ]]; then
  error "ANDROID_SDK_ROOT unset (no ${MANIFEST_ENV:-.manifest.env} loaded?). Try: make setup-android (native) or ensure deps ran (Docker)."
fi

# Incremental Gradle by default (fast back-to-back android-docker). Full Android/Java clean: CLEAN=yes (forwarded by run_devcontainer.py from host).
init_build_defaults android

usage(){
  cat <<EOF
Usage: $0 [--arch arm64-v8a|armeabi-v7a|x86|x86_64] [--build-type Debug|Release] [--api-level 28]

Install Android SDK/NDK and a Qt-for-Android build. Set ANDROID_SDK_ROOT, ANDROID_NDK_ROOT, JAVA_HOME, QT_ANDROID_DIR.
EOF
}
ARCH=${ANDROID_ARCH:-arm64-v8a}
BUILD_TYPE=Release
API_LEVEL=${ANDROID_API_LEVEL:-$ANDROID_TARGET_SDK_VERSION}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch) ARCH="$2"; shift 2;;
    --build-type) BUILD_TYPE="$2"; shift 2;;
    --api-level) API_LEVEL="$2"; shift 2;;
    -h|--help) usage; exit 0;;
    *) echo "Unknown arg: $1"; usage; exit 1;;
  esac
done

echo "=== PokerTH Android build helper ==="
echo "arch=$ARCH build=$BUILD_TYPE api-level=$API_LEVEL"

# Validate ABIs
case "$ARCH" in
  arm64-v8a|armeabi-v7a|x86|x86_64) ;;
  *)
    echo "Unsupported arch: $ARCH"
    exit 1
    ;;
esac

: ${TARGET:=pokerth_client}

# Prüfe Android-Plattform
if [[ ! -d "${ANDROID_SDK_ROOT}/platforms/android-${API_LEVEL}" ]]; then
  echo "WARNING: Android platform android-${API_LEVEL} not found"
fi

# Finde Build-Tools-Version
if [[ -d "${ANDROID_SDK_ROOT}/build-tools" ]]; then
  BUILD_TOOLS_VERSION=$(ls -1 "${ANDROID_SDK_ROOT}/build-tools" | sort -V | tail -n1)
  if [[ -n "$BUILD_TOOLS_VERSION" ]]; then
    export ANDROID_SDK_BUILD_TOOLS_REVISION="$BUILD_TOOLS_VERSION"
    echo "Using Android Build Tools version: $BUILD_TOOLS_VERSION"
  else
    echo "ERROR: No build-tools found"
    exit 5
  fi
else
  echo "ERROR: ${ANDROID_SDK_ROOT}/build-tools directory not found"
  exit 5
fi

command -v cmake >/dev/null || { echo "cmake not found"; exit 2; }

# Error text: manifest path depends on REPO_BUILD_ROOT (build_android/ native vs docker/<kind>/build/ for IN_DOCKER=1).
_manifest_expected="${REPO_ROOT}/${REPO_BUILD_ROOT:-build_android}/${MANIFEST_ENV:-.manifest.env}"
resolve_qt_cmake_cmd "qt-cmake not found. Expected ${MANIFEST_ENV:-.manifest.env} at ${_manifest_expected}${ROOT:+ (alternate: ${ROOT}/${MANIFEST_ENV:-.manifest.env})} from setup deps, plus Qt host gcc_64 (QT_ANDROID_DIR/QT_HOST_PATH). See docs/building-developer.md."
unset _manifest_expected

TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake"
if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
  echo "Cannot find Android toolchain file: $TOOLCHAIN_FILE"
  exit 3
fi

# vcpkg integration
VCPKG_CMAKE_ARGS=()
if [[ -n "${VCPKG_ROOT:-}" ]]; then
  VCPKG_CMAKE_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  if [[ ! -f "$VCPKG_CMAKE_FILE" ]]; then
    echo "VCPKG_ROOT set but $VCPKG_CMAKE_FILE not found"
    exit 4
  fi

  case "$ARCH" in
    arm64-v8a) VCPKG_TRIPLET="arm64-android";;
    armeabi-v7a) VCPKG_TRIPLET="arm-android";;
    x86) VCPKG_TRIPLET="x86-android";;
    x86_64) VCPKG_TRIPLET="x64-android";;
  esac

  VCPKG_CMAKE_ARGS+=(
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_CMAKE_FILE"
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
    -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET"
  )
fi

# Same pattern as build_linux.sh / build.sh: REPO_BUILD_ROOT separates host (build_android) vs Docker (docker/android/build)
if [ -n "${REPO_BUILD_ROOT:-}" ]; then
  BUILD_DIR_REL="$REPO_BUILD_ROOT"
else
  BUILD_DIR_REL="build_android"
fi
BUILD_DIR="$REPO_ROOT/$BUILD_DIR_REL"
mkdir -p "$BUILD_DIR"

# Cached Z_VCPKG_ROOT_DIR must match manifest (e.g. after moving vcpkg from ROOT to docker/<kind>/build/vcpkg).
invalidate_cmake_cache_if_vcpkg_root_mismatch "$BUILD_DIR"

# CMake Initial Cache
cat > "$BUILD_DIR/InitialCache.cmake" <<EOF
set(ANDROID_SDK_BUILD_TOOLS_REVISION "$BUILD_TOOLS_VERSION" CACHE STRING "")
set(QT_ANDROID_SDK_BUILD_TOOLS_REVISION "$BUILD_TOOLS_VERSION" CACHE STRING "")
EOF

# QT_CMAKE_CMD from resolve_qt_cmake_cmd (functions.sh); manifest PATH may omit ${QT_HOST_PATH}/bin.
echo "Configuring CMake..."
"$QT_CMAKE_CMD" -S . -B "$BUILD_DIR" -G Ninja \
  -C "$BUILD_DIR/InitialCache.cmake" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
  -DANDROID_MIN_SDK_VERSION="$ANDROID_MIN_SDK_VERSION" \
  -DANDROID_TARGET_SDK_VERSION="$ANDROID_TARGET_SDK_VERSION" \
  "${VCPKG_CMAKE_ARGS[@]}" \
  -DANDROID_ABI="$ARCH" \
  -DANDROID_NATIVE_API_LEVEL="$API_LEVEL" \
  -DCMAKE_PREFIX_PATH="${QT_ANDROID_DIR}/lib/cmake" \
  -DCMAKE_FIND_ROOT_PATH=${QT_ANDROID_DIR} \
  -DQt6_DIR="${QT_ANDROID_DIR}/lib/cmake/Qt6" \
  ${QT_HOST_PATH:+-DQT_HOST_PATH="$QT_HOST_PATH" -DQT_HOST_PATH_CMAKE_DIR="${QT_HOST_PATH}/lib/cmake"} \
  -DCMAKE_INSTALL_PREFIX="$(pwd)/$BUILD_DIR/install" \
  -DProtobuf_USE_STATIC_LIBS=ON

echo "Building target '${TARGET}'..."
cmake --build "$BUILD_DIR" --target ${TARGET} -j $(nproc || echo 1)

echo "Build finished. Artefacts in: $BUILD_DIR"

# Android source directory based on target
if [[ $TARGET == "pokerth_qml-client" ]]; then
  ANDROID_SOURCE_DIR="${PWD}/src/gui/qt6-qml/android"
  BUILD_SUBDIR="src/gui/qt6-qml"
else
  ANDROID_SOURCE_DIR="${PWD}/src/gui/qt/android"
  BUILD_SUBDIR="src/gui/qt"
fi

ANDROID_BUILD_DIR="$BUILD_DIR/android-build"

# WICHTIG: Suche deployment-settings.json im BUILD-Verzeichnis, nicht im Source-Verzeichnis!
DEPLOY_JSON=$(find "$BUILD_DIR/$BUILD_SUBDIR" -type f -name "*deployment-settings.json" 2>/dev/null | head -n1 || true)

if [[ -z "$DEPLOY_JSON" ]]; then
  echo "WARNING: No deployment settings JSON found in $BUILD_DIR/$BUILD_SUBDIR"
  DEPLOY_JSON=$(find "$BUILD_DIR" -type f -name "*deployment-settings.json" 2>/dev/null | head -n1 || true)
fi

if [[ -z "$DEPLOY_JSON" ]]; then
  echo "ERROR: No deployment settings JSON found."
  exit 10
fi

echo "Found deployment settings: $DEPLOY_JSON"

# Patche deployment-settings.json - WICHTIG: Ändere application-binary!
if command -v jq >/dev/null 2>&1; then
  echo "Patching deployment settings JSON..."
  DEPLOY_DIR="$(dirname "$DEPLOY_JSON")"
  TMP_JSON="$(mktemp "$DEPLOY_DIR/.deployment-settings.json.tmp.XXXXXX")"

  # Patche ALLE relevanten Felder UND setze application-binary auf den tatsächlichen Target-Namen
  jq --arg bt "$BUILD_TOOLS_VERSION" \
     --arg al "$API_LEVEL" \
     --arg min_sdk "$ANDROID_MIN_SDK_VERSION" \
     --arg arch "$ARCH" \
     --arg target "$TARGET" \
     --arg android_src "$ANDROID_SOURCE_DIR" \
     --arg sdk "$ANDROID_SDK_ROOT" \
    '.["android-build-tools-revision"] = $bt |
     .["android-sdk-build-tools-revision"] = $bt |
     .["sdkBuildToolsRevision"] = $bt |
     .["sdk"] = $sdk |
     .["android-target-sdk-version"] = $al |
     .["android-min-sdk-version"] = $min_sdk |
     .["target-architecture"] = $arch |
     .["application-binary"] = $target |
     .["android-package-source-directory"] = $android_src' \
    "$DEPLOY_JSON" > "$TMP_JSON"
  mv -f "$TMP_JSON" "$DEPLOY_JSON"
else
  echo "WARNING: jq not found, cannot patch deployment settings"
fi

# CLEAN=yes: remove previous android-build (fresh template/Gradle state; replaces gradlew clean).
if is_yes "$CLEAN"; then
  echo "CLEAN=yes: removing $ANDROID_BUILD_DIR"
  rm -rf "$ANDROID_BUILD_DIR"
fi

# Erstelle Android Build-Verzeichnisstruktur
mkdir -p "$ANDROID_BUILD_DIR/libs/$ARCH"

# Kopiere Android-Manifest und Ressourcen BEVOR androiddeployqt läuft (-L: follow symlinks, e.g. ic_launcher → data/…)
if [[ -d "$ANDROID_SOURCE_DIR" ]]; then
  echo "Copying Android source files from: $ANDROID_SOURCE_DIR"
  cp -RLv "$ANDROID_SOURCE_DIR"/* "$ANDROID_BUILD_DIR/"
fi

mkdir -p "$ANDROID_BUILD_DIR/res/drawable"
mkdir -p "$ANDROID_BUILD_DIR/res/values"

# Erstelle immer das dynamische AndroidManifest.xml mit korrektem lib_name und Version
cat > "$ANDROID_BUILD_DIR/AndroidManifest.xml" <<MANIFEST
<?xml version="1.0"?>
<manifest package="org.pokerth.widget"
          xmlns:android="http://schemas.android.com/apk/res/android"
          android:versionName="2.0.6"
          android:versionCode="20"
          android:installLocation="auto">

    <uses-sdk
        android:minSdkVersion="$ANDROID_MIN_SDK_VERSION"
        android:targetSdkVersion="$API_LEVEL"/>

    <supports-screens
        android:largeScreens="true"
        android:normalScreens="true"
        android:anyDensity="true"
        android:smallScreens="true"/>

    <application
        android:hardwareAccelerated="true"
        android:name="org.qtproject.qt.android.bindings.QtApplication"
        android:label="PokerTH"
        android:icon="@drawable/ic_launcher"
        android:extractNativeLibs="true"
        android:usesCleartextTraffic="true"
        android:theme="@android:style/Theme.NoTitleBar.Fullscreen">

        <activity
            android:name="org.qtproject.qt.android.bindings.QtActivity"
            android:label="PokerTH"
            android:screenOrientation="landscape"
            android:launchMode="singleTop"
            android:windowSoftInputMode="adjustResize"
            android:exported="true"
            android:configChanges="orientation|uiMode|screenLayout|screenSize|smallestScreenSize|layoutDirection|locale|fontScale|keyboard|keyboardHidden|navigation|mcc|mnc|density">

            <intent-filter>
                <action android:name="android.intent.action.MAIN"/>
                <category android:name="android.intent.category.LAUNCHER"/>
            </intent-filter>

            <meta-data
                android:name="android.app.lib_name"
                android:value="$TARGET"/>

            <meta-data
                android:name="android.app.extract_android_style"
                android:value="minimal"/>

        </activity>
    </application>

    <uses-permission android:name="android.permission.INTERNET"/>
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE"/>

</manifest>
MANIFEST

# Finde .so-Datei - suche sowohl nach lib${TARGET}.so als auch nach Varianten
SO_FILE=$(find "$BUILD_DIR" -type f \( -name "lib${TARGET}.so" -o -name "lib${TARGET}_*.so" \) | head -n1)

if [[ -z "$SO_FILE" ]]; then
  echo "ERROR: Could not find lib${TARGET}*.so"
  exit 6
fi

EXPECTED_SO_NAME="lib${TARGET}_${ARCH}.so"
DEST_SO="$ANDROID_BUILD_DIR/libs/$ARCH/$EXPECTED_SO_NAME"
if command -v realpath >/dev/null 2>&1 && [ -e "$DEST_SO" ] && [ "$(realpath "$SO_FILE")" = "$(realpath "$DEST_SO")" ]; then
  echo "Library already in place: $DEST_SO"
else
  cp -v "$SO_FILE" "$DEST_SO"
fi

# Erstelle auch einen Symlink mit dem ursprünglichen Namen
ln -sf "$EXPECTED_SO_NAME" "$ANDROID_BUILD_DIR/libs/$ARCH/lib${TARGET}.so"

# Überprüfe, ob die Datei kopiert wurde
if [[ ! -f "$DEST_SO" ]]; then
  echo "ERROR: Failed to copy library"
  exit 9
fi

# Download OpenSSL 3.x libraries for Android (required for Qt Network HTTPS)
OPENSSL_DIR="$ANDROID_BUILD_DIR/libs/$ARCH"
mkdir -p "$OPENSSL_DIR"
OPENSSL_BASE_URL="https://github.com/KDAB/android_openssl/raw/master/ssl_3"
case "$ARCH" in
  arm64-v8a) OPENSSL_ARCH="arm64-v8a";;
  armeabi-v7a) OPENSSL_ARCH="armeabi-v7a";;
  x86_64) OPENSSL_ARCH="x86_64";;
  x86) OPENSSL_ARCH="x86";;
  *) OPENSSL_ARCH="$ARCH";;
esac
if [[ ! -s "$OPENSSL_DIR/libssl_3.so" ]]; then
  $CURL_CMD -o "$OPENSSL_DIR/libssl_3.so" "$OPENSSL_BASE_URL/$OPENSSL_ARCH/libssl_3.so" || echo "WARNING: Failed to download libssl_3.so"
fi
if [[ ! -s "$OPENSSL_DIR/libcrypto_3.so" ]]; then
  $CURL_CMD -o "$OPENSSL_DIR/libcrypto_3.so" "$OPENSSL_BASE_URL/$OPENSSL_ARCH/libcrypto_3.so" || echo "WARNING: Failed to download libcrypto_3.so"
fi

ANDROIDDEPLOYQT="${QT_HOST_PATH}/bin/androiddeployqt"
if [[ ! -x "$ANDROIDDEPLOYQT" ]]; then
  echo "ERROR: androiddeployqt not found at $ANDROIDDEPLOYQT"
  exit 7
fi

# Full androiddeployqt (no --aux-mode): Qt runs buildAndroidProject() and Gradle; --release → assembleRelease.
# ic_launcher: QT_ANDROID_PACKAGE_SOURCE_DIR (e.g. src/gui/qt/android/res/drawable/ic_launcher.png).
# Do NOT use --no-build: with build=false, Qt skips copyAndroidTemplate/cleanAndroidFiles entirely
# (qtbase src/tools/androiddeployqt/main.cpp: options.build && !copyDependenciesOnly).
# https://doc.qt.io/qt-6/android-deploy-qt-tool.html
echo "Running androiddeployqt..."
"$ANDROIDDEPLOYQT" \
  --input "$DEPLOY_JSON" \
  --output "$ANDROID_BUILD_DIR" \
  --android-platform "android-${API_LEVEL}" \
  --jdk "$JAVA_HOME" \
  --release \
  --verbose

if [[ ! -f "$ANDROID_BUILD_DIR/gradle.properties" ]]; then
  echo "ERROR: gradle.properties not found under $ANDROID_BUILD_DIR after androiddeployqt."
  exit 8
fi

APK_FILE=$(find "$ANDROID_BUILD_DIR" -type f -name "*.apk" | grep -E "(release|debug)" | grep -v "unaligned" | head -n1)
if [[ -n "$APK_FILE" ]]; then
  echo "APK created: $APK_FILE"
else
  echo "WARNING: Could not find generated APK"
fi

echo "Done."
