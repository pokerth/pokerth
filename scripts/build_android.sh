#!/usr/bin/env bash
set -euo pipefail

# Android build: single-ABI (default) or fat APK when FAT_APK=yes (build_android_fat_apk in this file).
# Erwartet als Umgebungsvariablen:
#  ANDROID_SDK_ROOT, ANDROID_NDK_ROOT, JAVA_HOME, QT_ANDROID_DIR
# shellcheck source=/dev/null
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/functions.sh"

# Makefile sets TARGET_PLATFORM=android; default here so functions.sh applies when invoked directly.
export TARGET_PLATFORM="${TARGET_PLATFORM:-$DEFAULT_TARGET_PLATFORM_ANDROID}"

cd "$REPO_ROOT"

# Manifest paths: basename is MANIFEST_ENV from functions.sh (same as Makefile MANIFEST_NAME).
_android_manifest_rel="${REPO_BUILD_ROOT:-build_${TARGET_PLATFORM}}"
_android_repo_mf="${REPO_ROOT}/${_android_manifest_rel}/${MANIFEST_ENV}"
_android_cache_mf="${CACHE_ROOT}/${TARGET_PLATFORM}/${MANIFEST_ENV}"

# Source manifest written by setup_android.sh (ANDROID_SDK_ROOT, NDK, JAVA_HOME, QT_ANDROID_DIR, PATH, etc.).
# Repo-local first; in Docker, re-source cache copy if present (setup_android syncs there).
# Likely failure: no file here after a fresh clone — run setup-android or ensure (deps) first.
if [ -f "$_android_repo_mf" ]; then
  # shellcheck source=/dev/null
  . "$_android_repo_mf"
fi
if [[ "${IN_DOCKER:-}" == "1" ]] && [[ -f "$_android_cache_mf" ]]; then
  # shellcheck source=/dev/null
  . "$_android_cache_mf"
fi

ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-}"
ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-}"
JAVA_HOME="${JAVA_HOME:-}"
QT_ANDROID_DIR="${QT_ANDROID_DIR:-}"
QT_HOST_PATH="${QT_HOST_PATH:-}"

# Without a sourced setup manifest, ANDROID_SDK_ROOT may be unset. Avoid ${ANDROID_SDK_ROOT}/build-tools → "/build-tools".
if [[ -z "${ANDROID_SDK_ROOT}" ]]; then
  error "ANDROID_SDK_ROOT unset (setup manifest not loaded — expected ${_android_repo_mf}). Try: make setup-android (native) or ensure deps ran (Docker)."
fi

# Docker: reject a native-host SDK path if .manifest.env was copied or stale (must match CACHE_ROOT; see docs/building-developer.md).
if [[ "${IN_DOCKER:-}" == "1" ]] && [[ "${ANDROID_SDK_ROOT}" != "${CACHE_ROOT}/android"* ]]; then
  error "ANDROID_SDK_ROOT (${ANDROID_SDK_ROOT}) must be under ${CACHE_ROOT}/android when IN_DOCKER=1. Fix or remove ${_android_repo_mf} and re-run setup / ensure."
fi

# Incremental Gradle by default (fast back-to-back android-docker). Full Android/Java clean: CLEAN=yes (forwarded by run_devcontainer.py from host).
init_build_defaults "${TARGET_PLATFORM}"

# --- Shared helpers (single-ABI + FAT_APK; same file only — no separate common script) ---

# Sets BUILD_TOOLS_VERSION and ANDROID_SDK_BUILD_TOOLS_REVISION.
# Optional arg "verbose": long error when build-tools dir missing (single-ABI / Docker); else short error().
android_set_build_tools_version_from_sdk() {
  local verbose="${1:-}"
  if [[ ! -d "${ANDROID_SDK_ROOT}/build-tools" ]]; then
    if [[ "$verbose" == "verbose" ]]; then
      echo "ERROR: ${ANDROID_SDK_ROOT}/build-tools not found (no Android SDK at ANDROID_SDK_ROOT)."
      echo "Either the SDK was never installed in this container, or the manifest at ${_android_repo_mf}"
      echo "points at ${CACHE_ROOT}/${TARGET_PLATFORM}/... from a different environment."
      echo "The repo-root devcontainer (unified docker/Dockerfile) does not install the Android SDK under ${CACHE_ROOT}/${TARGET_PLATFORM} by default."
      echo "Use repo-root .devcontainer/devcontainer.json (image: docker/Dockerfile) for make android, or rebuild that image;"
      echo "or remove ${_android_repo_mf} and run setup inside an image that provisions the SDK."
      exit 5
    fi
    error "${ANDROID_SDK_ROOT}/build-tools not found"
  fi
  BUILD_TOOLS_VERSION=$(ls -1 "${ANDROID_SDK_ROOT}/build-tools" | sort -V | tail -n1)
  if [[ -z "$BUILD_TOOLS_VERSION" ]]; then
    echo "ERROR: No build-tools found"
    exit 5
  fi
  export ANDROID_SDK_BUILD_TOOLS_REVISION="$BUILD_TOOLS_VERSION"
  if [[ "$verbose" == "verbose" ]]; then
    echo "Using Android Build Tools version: $BUILD_TOOLS_VERSION"
  else
    echo "Build Tools Version: $BUILD_TOOLS_VERSION"
  fi
}

write_android_sdk_initial_cache_cmake() {
  local build_dir="$1"
  mkdir -p "$build_dir"
  cat > "$build_dir/InitialCache.cmake" <<EOF
set(ANDROID_SDK_BUILD_TOOLS_REVISION "$BUILD_TOOLS_VERSION" CACHE STRING "")
set(QT_ANDROID_SDK_BUILD_TOOLS_REVISION "$BUILD_TOOLS_VERSION" CACHE STRING "")
EOF
}

# Widgets client manifest (org.pokerth.widget); uses TARGET, ANDROID_MIN_SDK_VERSION, API_LEVEL.
write_pokerth_widgets_android_manifest() {
  local out="$1"
  cat > "$out" <<MANIFEST
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
}

find_android_deployment_settings_json() {
  local build_dir="$1" build_subdir="$2"
  local dj
  dj=$(find "$build_dir/$build_subdir" -type f -name "*deployment-settings.json" 2>/dev/null | head -n1 || true)
  if [[ -n "$dj" ]]; then
    echo "$dj"
    return 0
  fi
  find "$build_dir" -type f -name "*deployment-settings.json" 2>/dev/null | head -n1 || true
}

# Globals: QT_CMAKE_CMD, TOOLCHAIN_FILE, BUILD_TYPE, ANDROID_MIN_SDK_VERSION, ANDROID_TARGET_SDK_VERSION,
# VCPKG_CMAKE_ARGS, API_LEVEL, QT_HOST_PATH.
run_android_qt_cmake_configure() {
  local build_dir="$1" abi="$2" qt_android_dir="$3" install_prefix="$4"
  local config_label="${5:-Configuring CMake for ${abi}...}"
  write_android_sdk_initial_cache_cmake "$build_dir"
  echo "$config_label"
  "$QT_CMAKE_CMD" -S . -B "$build_dir" -G Ninja \
    -C "$build_dir/InitialCache.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DANDROID_MIN_SDK_VERSION="$ANDROID_MIN_SDK_VERSION" \
    -DANDROID_TARGET_SDK_VERSION="$ANDROID_TARGET_SDK_VERSION" \
    "${VCPKG_CMAKE_ARGS[@]}" \
    -DANDROID_ABI="$abi" \
    -DANDROID_NATIVE_API_LEVEL="$API_LEVEL" \
    -DCMAKE_PREFIX_PATH="${qt_android_dir}/lib/cmake" \
    -DCMAKE_FIND_ROOT_PATH="${qt_android_dir}" \
    -DQt6_DIR="${qt_android_dir}/lib/cmake/Qt6" \
    ${QT_HOST_PATH:+-DQT_HOST_PATH="$QT_HOST_PATH" -DQT_HOST_PATH_CMAKE_DIR="${QT_HOST_PATH}/lib/cmake"} \
    -DCMAKE_INSTALL_PREFIX="$install_prefix" \
    -DProtobuf_USE_STATIC_LIBS=ON
}

resolve_androiddeployqt_or_exit() {
  ANDROIDDEPLOYQT="${QT_HOST_PATH}/bin/androiddeployqt"
  if [[ ! -x "$ANDROIDDEPLOYQT" ]]; then
    echo "ERROR: androiddeployqt not found: $ANDROIDDEPLOYQT"
    exit 7
  fi
}

# Doc anchor for errors / usage (see docs/building-developer.md).
_DOC_ANDROID_FAT_REF="docs/building-developer.md#android-fat-apk-reference"

require_fat_apk_pokerth_client_target() {
  if [[ "${TARGET:-}" != "pokerth_client" ]]; then
    error "FAT_APK requires pokerth_client (TARGET was '${TARGET:-}'). Multi-ABI fat + QML is not supported; see ${_DOC_ANDROID_FAT_REF}."
  fi
}

# Echo vcpkg triplet for ANDROID_ABI. arm-neon-android matches NDK r28+ (not community arm-android NEON=OFF).
vcpkg_triplet_for_android_abi() {
  case "$1" in
    arm64-v8a) echo "arm64-android";;
    armeabi-v7a) echo "arm-neon-android";;
    x86) echo "x86-android";;
    x86_64) echo "x64-android";;
    *) echo "";;
  esac
}

# KDAB OpenSSL 3 prebuilts. mode: always | if_missing
download_kdab_openssl3_libs() {
  local dest_dir="$1" arch="$2" mode="${3:-always}"
  local base="https://github.com/KDAB/android_openssl/raw/master/ssl_3"
  mkdir -p "$dest_dir"
  if [[ "$mode" == "if_missing" ]]; then
    if [[ ! -s "$dest_dir/libssl_3.so" ]]; then
      $CURL_CMD -o "$dest_dir/libssl_3.so" "$base/$arch/libssl_3.so" \
        || echo "WARNING: Failed to download libssl_3.so"
    fi
    if [[ ! -s "$dest_dir/libcrypto_3.so" ]]; then
      $CURL_CMD -o "$dest_dir/libcrypto_3.so" "$base/$arch/libcrypto_3.so" \
        || echo "WARNING: Failed to download libcrypto_3.so"
    fi
    return 0
  fi
  $CURL_CMD -o "$dest_dir/libssl_3.so" "$base/$arch/libssl_3.so" \
    || echo "WARNING: libssl_3.so Download fehlgeschlagen für $arch"
  $CURL_CMD -o "$dest_dir/libcrypto_3.so" "$base/$arch/libcrypto_3.so" \
    || echo "WARNING: libcrypto_3.so Download fehlgeschlagen für $arch"
}

# release/debug APK under androiddeployqt output (excludes unaligned).
find_built_android_apk() {
  find "${1:?}" -type f -name "*.apk" 2>/dev/null | grep -E "(release|debug)" | grep -v "unaligned" | head -n1
}

init_qt_android_dirs_for_fat() {
  QT_ANDROID_DIR_ARM64="${QT_ANDROID_DIR_ARM64:-${QT_ANDROID_DIR:-}}"
  QT_ANDROID_DIR_ARMV7="${QT_ANDROID_DIR_ARMV7:-}"
  if [[ -z "$QT_ANDROID_DIR_ARM64" ]]; then
    error "Qt Android arm64 kit unset (QT_ANDROID_DIR or QT_ANDROID_DIR_ARM64). Expected ${MANIFEST_ENV} at ${_android_repo_mf}."
  fi
  if [[ -z "$QT_ANDROID_DIR_ARMV7" ]]; then
    local _parent _leaf
    _parent="${QT_ANDROID_DIR_ARM64%/*}"
    _leaf="${QT_ANDROID_DIR_ARM64##*/}"
    if [[ "$_leaf" == "android_arm64_v8a" ]]; then
      QT_ANDROID_DIR_ARMV7="${_parent}/android_armv7"
    else
      error "Cannot derive Qt android_armv7 path from QT_ANDROID_DIR=${QT_ANDROID_DIR_ARM64} (expected .../android_arm64_v8a). Set QT_ANDROID_DIR_ARMV7."
    fi
  fi
  export QT_ANDROID_DIR_ARM64 QT_ANDROID_DIR_ARMV7
  export QT_ANDROID_DIR="$QT_ANDROID_DIR_ARM64"
}

# Armv7-Qt fehlt oft beim ersten Fat-Build — optional AUTO_INSTALL_QT_ARMV7=yes (default): deps mit aqt nachziehen.
ensure_qt_android_kits_for_fat() {
  local arm64_cfg="${QT_ANDROID_DIR_ARM64}/lib/cmake/Qt6/Qt6Config.cmake"
  local armv7_cfg="${QT_ANDROID_DIR_ARMV7}/lib/cmake/Qt6/Qt6Config.cmake"
  if [[ ! -f "$arm64_cfg" ]]; then
    error "Qt6 Android arm64 kit missing: ${QT_ANDROID_DIR_ARM64} (expected ${arm64_cfg}). Run: TARGET_PLATFORM=android BUILD_DIR=${_android_manifest_rel} ${REPO_ROOT}/scripts/setup.sh deps"
  fi
  if [[ -f "$armv7_cfg" ]]; then
    return 0
  fi
  if ! is_yes "${AUTO_INSTALL_QT_ARMV7:-yes}"; then
    error "Qt6 Android kit missing: ${QT_ANDROID_DIR_ARMV7} (Fat APK needs android_armv7). Install: INSTALL_QT_ARMV7=yes TARGET_PLATFORM=android BUILD_DIR=${_android_manifest_rel} ${REPO_ROOT}/scripts/setup.sh deps"
  fi
  log "Fat APK: android_armv7 Qt not found — running INSTALL_QT_ARMV7=yes … setup.sh deps (set AUTO_INSTALL_QT_ARMV7=no to disable)."
  INSTALL_QT_ARMV7=yes TARGET_PLATFORM=android BUILD_DIR="${_android_manifest_rel}" "${REPO_ROOT}/scripts/setup.sh" deps
  if [[ -f "$_android_repo_mf" ]]; then
    set -a
    # shellcheck source=/dev/null
    . "$_android_repo_mf"
    set +a
  fi
  init_qt_android_dirs_for_fat
  if [[ ! -f "${QT_ANDROID_DIR_ARMV7}/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
    error "Qt6 android_armv7 still missing after deps: ${QT_ANDROID_DIR_ARMV7}. Retry: INSTALL_QT_ARMV7=yes TARGET_PLATFORM=android BUILD_DIR=${_android_manifest_rel} ${REPO_ROOT}/scripts/setup.sh deps"
  fi
}

build_android_fat_apk() {
  ABIS=("arm64-v8a" "armeabi-v7a")
  echo "=== PokerTH Universal Android Build ==="
  echo "ABIs: ${ABIS[*]}"
  echo "build=$BUILD_TYPE  api-level=$API_LEVEL  target=$TARGET"
  echo ""
  init_qt_android_dirs_for_fat
  ensure_qt_android_kits_for_fat

  command -v cmake >/dev/null || error "cmake not found"

  resolve_qt_cmake_cmd "qt-cmake not found. Set QT_HOST_PATH (Qt linux_gcc_64) or put qt-cmake on PATH."

  # Toolchain-Validierung wird pro ABI in der Build-Schleife gemacht

  android_set_build_tools_version_from_sdk

  # Android-Quellverzeichnis (Widgets-Client; Fat v1 kein pokerth_qml-client)
  ANDROID_SOURCE_DIR="${REPO_ROOT}/src/gui/qt/android"
  BUILD_SUBDIR="src/gui/qt"

  if [ -n "${REPO_BUILD_ROOT:-}" ]; then
    _android_build_root_rel="$REPO_BUILD_ROOT"
  else
    _android_build_root_rel="build_${TARGET_PLATFORM}"
  fi
  FAT_BUILD_ROOT="${REPO_ROOT}/${_android_build_root_rel}"
  mkdir -p "$FAT_BUILD_ROOT"

  # vcpkg-Argumente vorbereiten (Triplet pro ABI in der Schleife; siehe vcpkg_triplet_for_abi)
  VCPKG_CMAKE_FILE=""
  if [[ -n "${VCPKG_ROOT:-}" ]]; then
    VCPKG_CMAKE_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    [[ -f "$VCPKG_CMAKE_FILE" ]] || error "VCPKG_ROOT set but $VCPKG_CMAKE_FILE not found"
  fi

  # Fat APK: Boost/vcpkg für arm64-android und arm-neon-android (deps siehe setup_android.sh)
  if [[ -n "${VCPKG_ROOT:-}" ]]; then
    for _triplet in arm64-android arm-neon-android; do
      _inc="${VCPKG_ROOT}/installed/${_triplet}/include/boost/version.hpp"
      if [[ ! -f "$_inc" ]]; then
        error "vcpkg triplet '${_triplet}' is missing or incomplete under ${VCPKG_ROOT}/installed/ (expected Boost headers). Fat APK needs both arm64-android and arm-neon-android. Run deps once: TARGET_PLATFORM=android BUILD_DIR=${_android_build_root_rel} ${REPO_ROOT}/scripts/setup.sh deps"
      fi
    done
  fi

  # ─── Hilfsfunktionen (Fat-Schleife) ─────────────────────────────────────────

  # Liefert den Qt-Android-Pfad für die ABI (separate aqt-Installs)
  qt_dir_for_abi() {
    case "$1" in
      arm64-v8a)  echo "$QT_ANDROID_DIR_ARM64";;
      armeabi-v7a) echo "$QT_ANDROID_DIR_ARMV7";;
    esac
  }

  # Liefert den NDK-Root-Pfad (identisch für beide ABIs: ANDROID_NDK_ROOT)
  ndk_root_for_abi() {
    echo "$ANDROID_NDK_ROOT"
  }

  # ─── Phase 1: Beide Architekturen bauen ─────────────────────────────────────

  for ABI in "${ABIS[@]}"; do
    echo ""
    echo "================================================================"
    echo "  Baue für ABI: $ABI"
    echo "================================================================"
    echo ""

    QT_ANDROID_DIR=$(qt_dir_for_abi "$ABI")
    VCPKG_TRIPLET=$(vcpkg_triplet_for_android_abi "$ABI")
    NDK_ROOT=$(ndk_root_for_abi "$ABI")
    TOOLCHAIN_FILE="$NDK_ROOT/build/cmake/android.toolchain.cmake"
    BUILD_DIR="${FAT_BUILD_ROOT}/fat-cmake-${ABI}"

    [[ -f "$TOOLCHAIN_FILE" ]] || { echo "Android toolchain nicht gefunden: $TOOLCHAIN_FILE"; exit 3; }
    echo "NDK: $NDK_ROOT"

    VCPKG_CMAKE_ARGS=()
    if [[ -n "$VCPKG_CMAKE_FILE" ]]; then
      VCPKG_CMAKE_ARGS+=(
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_CMAKE_FILE"
        -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
        -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET"
      )
    fi

    run_android_qt_cmake_configure "$BUILD_DIR" "$ABI" "$QT_ANDROID_DIR" "${REPO_ROOT}/$BUILD_DIR/install" "Konfiguriere CMake für $ABI ..."

    echo "Baue Target '${TARGET}' für $ABI ..."
    cmake --build "$BUILD_DIR" --target "${TARGET}" -j "$(nproc || echo 1)"

    echo "Build für $ABI abgeschlossen."
  done

# ─── Phase 2: Universal-APK zusammenstellen ─────────────────────────────────

echo ""
echo "================================================================"
echo "  Erstelle Universal-APK"
echo "================================================================"
echo ""

UNIVERSAL_DIR="${FAT_BUILD_ROOT}/fat-universal"
ANDROID_BUILD_DIR="$UNIVERSAL_DIR/android-build"

# Sauberer Start: alte libs/ löschen (verhindert Überbleibsel aus früheren Läufen)
rm -rf "$ANDROID_BUILD_DIR/libs"
mkdir -p "$ANDROID_BUILD_DIR"

# Kopiere Android-Manifest und Ressourcen
if [[ -d "$ANDROID_SOURCE_DIR" ]]; then
  echo "Kopiere Android-Quelldateien aus: $ANDROID_SOURCE_DIR"
  cp -rv "$ANDROID_SOURCE_DIR"/* "$ANDROID_BUILD_DIR/" || true
fi

mkdir -p "$ANDROID_BUILD_DIR/res/drawable"
mkdir -p "$ANDROID_BUILD_DIR/res/values"

# Dynamisches AndroidManifest.xml
write_pokerth_widgets_android_manifest "$ANDROID_BUILD_DIR/AndroidManifest.xml"
echo "AndroidManifest.xml erstellt mit lib_name=$TARGET"


# ─── androiddeployqt mit Multi-ABI deployment-settings.json ────────────────
#
# Qt 6 unterstützt Multi-ABI nativ in deployment-settings.json: die per-ABI-
# Felder (qt, architectures, qtLibsDirectory, etc.) sind als Objekte mit
# ABI-Keys gespeichert. Ein EINZIGER androiddeployqt-Aufruf mit merged JSON
# erzeugt das komplette Gradle-Projekt inkl. libs.xml für ALLE ABIs.
#
# NDK: Ein ANDROID_NDK_ROOT (Manifest / versions.env) für beide ABIs und merged JSON.
# Früher: teils zweites NDK (r27) nur für v7a/androiddeployqt — entfällt mit aktuellem Tooling.

resolve_androiddeployqt_or_exit

# ─── Schritt 1: Deployment-Settings-JSONs finden und zusammenführen ─────────

echo ""
echo "--- Erstelle merged Multi-ABI deployment-settings.json ---"

# Finde die von CMake generierten deployment-settings.json für jede ABI
declare -A DEPLOY_JSONS
for ABI in "${ABIS[@]}"; do
  ABI_BUILD_DIR="${FAT_BUILD_ROOT}/fat-cmake-${ABI}"
  DJ=$(find_android_deployment_settings_json "$ABI_BUILD_DIR" "$BUILD_SUBDIR")
  if [[ -z "$DJ" ]]; then
    echo "ERROR: Keine deployment-settings.json gefunden für $ABI in $ABI_BUILD_DIR"
    exit 10
  fi
  DEPLOY_JSONS[$ABI]="$DJ"
  echo "  $ABI: $DJ"
done

PRIMARY_ABI="${ABIS[0]}"
SECONDARY_ABI="${ABIS[1]}"
MERGED_JSON="$UNIVERSAL_DIR/deployment-settings-universal.json"

# Merged JSON erzeugen: per-ABI-Objekte zusammenführen; einheitlicher NDK-Pfad (.ndk / stdcpp-path)
if ! command -v jq >/dev/null 2>&1; then
  echo "ERROR: jq wird für Multi-ABI-Merge benötigt"
  exit 11
fi

jq -n \
  --slurpfile primary "${DEPLOY_JSONS[$PRIMARY_ABI]}" \
  --slurpfile secondary "${DEPLOY_JSONS[$SECONDARY_ABI]}" \
  --arg ndk "$ANDROID_NDK_ROOT" \
  --arg sdk "$ANDROID_SDK_ROOT" \
  --arg bt "$BUILD_TOOLS_VERSION" \
  --arg al "$API_LEVEL" \
  --arg min_sdk "$ANDROID_MIN_SDK_VERSION" \
  --arg android_src "$ANDROID_SOURCE_DIR" \
  --arg target "$TARGET" \
'
  ($primary[0]) as $a | ($secondary[0]) as $b |
  $a |

  # Per-ABI-Objekte zusammenführen
  .qt = ($a.qt + $b.qt) |
  .qtDataDirectory = ($a.qtDataDirectory + $b.qtDataDirectory) |
  .qtLibExecsDirectory = ($a.qtLibExecsDirectory + $b.qtLibExecsDirectory) |
  .qtLibsDirectory = ($a.qtLibsDirectory + $b.qtLibsDirectory) |
  .qtPluginsDirectory = ($a.qtPluginsDirectory + $b.qtPluginsDirectory) |
  .qtQmlDirectory = ($a.qtQmlDirectory + $b.qtQmlDirectory) |
  .architectures = ($a.architectures + $b.architectures) |

  # Deploy-Plugins beider ABIs zusammenführen (semikolon-getrennt)
  .["android-deploy-plugins"] = (
    [$a["android-deploy-plugins"], $b["android-deploy-plugins"]]
    | map(select(. != null and . != ""))
    | join(";")
  ) |

  # Extra-Prefix-Dirs zusammenführen
  .extraPrefixDirs = (
    (($a.extraPrefixDirs // []) + ($b.extraPrefixDirs // [])) | unique
  ) |

  # NDK für readelf + libc++_shared (gleiches NDK für beide ABIs)
  .ndk = $ndk |
  .["stdcpp-path"] = ($ndk + "/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/") |

  # SDK + Build-Metadaten
  .sdk = $sdk |
  .sdkBuildToolsRevision = $bt |
  .["android-build-tools-revision"] = $bt |
  .["android-sdk-build-tools-revision"] = $bt |
  .["android-target-sdk-version"] = $al |
  .["android-min-sdk-version"] = $min_sdk |
  .["application-binary"] = $target |
  .["android-package-source-directory"] = $android_src
' > "$MERGED_JSON"

echo ""
echo "=== Merged deployment-settings.json ==="
cat "$MERGED_JSON"
echo ""
echo "=== Ende deployment-settings.json ==="

# ─── Schritt 2: App-.so + OpenSSL für beide ABIs kopieren ──────────────────

for ABI in "${ABIS[@]}"; do
  echo ""
  echo "--- Kopiere App-Library + OpenSSL für $ABI ---"

  SRC_BUILD_DIR="${FAT_BUILD_ROOT}/fat-cmake-${ABI}"
  mkdir -p "$ANDROID_BUILD_DIR/libs/$ABI"

  # App-Library
  SO_FILE=$(find "$SRC_BUILD_DIR" -type f \( -name "lib${TARGET}.so" -o -name "lib${TARGET}_*.so" \) | head -n1)
  if [[ -z "$SO_FILE" ]]; then
    echo "ERROR: lib${TARGET}*.so nicht gefunden in $SRC_BUILD_DIR"
    exit 6
  fi

  EXPECTED_SO_NAME="lib${TARGET}_${ABI}.so"
  cp -v "$SO_FILE" "$ANDROID_BUILD_DIR/libs/$ABI/$EXPECTED_SO_NAME"
  ln -sf "$EXPECTED_SO_NAME" "$ANDROID_BUILD_DIR/libs/$ABI/lib${TARGET}.so"

  download_kdab_openssl3_libs "$ANDROID_BUILD_DIR/libs/$ABI" "$ABI" always

  echo "  libs/$ABI: $(ls -1 "$ANDROID_BUILD_DIR/libs/$ABI/"*.so 2>/dev/null | wc -l) .so-Dateien (vor androiddeployqt)"
done

# Icon liegt in Package-Source (android-package-source-directory), z. B. res/drawable/ic_launcher.png unter src/gui/qt/android/.

# ─── Schritt 3: androiddeployqt mit merged JSON aufrufen ───────────────────
#
# KEIN --no-build! In Qt 6.9 generiert androiddeployqt die Gradle-Projektdateien
# (libs.xml, build.gradle, gradlew, gradle.properties) nur als Teil des
# vollständigen Laufs. --no-build überspringt die Projekt-Generierung komplett.
# Da sdk + sdkBuildToolsRevision jetzt korrekt im JSON stehen, läuft der
# interne Gradle-Build durch (mit --release: assembleRelease statt Debug).
# Danach bauen wir nochmal manuell als assembleRelease (nach Cross-Arch-Cleanup; Multi-ABI-Qt-Bug).

echo ""
echo "--- androiddeployqt mit Multi-ABI deployment-settings.json ---"
echo ""

set +e
"$ANDROIDDEPLOYQT" \
  --input "$MERGED_JSON" \
  --output "$ANDROID_BUILD_DIR" \
  --android-platform "android-${API_LEVEL}" \
  --jdk "$JAVA_HOME" \
  --release \
  --verbose
DEPLOYQT_EXIT=$?
set -e
echo ""
echo "androiddeployqt Exit-Code: $DEPLOYQT_EXIT"
if [[ $DEPLOYQT_EXIT -ne 0 ]]; then
  echo "ERROR: androiddeployqt fehlgeschlagen (exit $DEPLOYQT_EXIT)"
  exit "$DEPLOYQT_EXIT"
fi

# ─── Schritt 3b: Cross-Arch-Cleanup ────────────────────────────────────────
#
# androiddeployqt hat einen Bug im Multi-ABI-Modus: beim Scannen der
# armeabi-v7a-Dependencies werden auch arm64-v8a-Plugins erkannt und
# fälschlicherweise nach libs/armeabi-v7a/ kopiert. Außerdem werden
# unsuffixed Libs (libavcodec.so etc.) aus dem falschen Qt-Verzeichnis geholt.
# → Beide Probleme hier bereinigen.

echo ""
echo "--- Cross-Arch-Cleanup ---"

for ABI in "${ABIS[@]}"; do
  ABI_DIR="$ANDROID_BUILD_DIR/libs/$ABI"
  [[ -d "$ABI_DIR" ]] || continue

  # Entferne suffixed .so-Dateien die zur FALSCHEN Architektur gehören
  REMOVED=0
  for OTHER_ABI in "${ABIS[@]}"; do
    [[ "$OTHER_ABI" == "$ABI" ]] && continue
    # Lösche z.B. *_arm64-v8a.so aus libs/armeabi-v7a/
    for f in "$ABI_DIR"/*_"${OTHER_ABI}".so; do
      [[ -e "$f" ]] || continue
      rm -v "$f"
      ((REMOVED++)) || true
    done
  done
  echo "  $ABI: $REMOVED fremd-ABI .so-Dateien entfernt"

  # Ersetze unsuffixed Libs (av*, sw*) mit der richtigen Architektur-Version.
  # androiddeployqt kopiert diese evtl. aus dem falschen Qt-ABI-Verzeichnis.
  QT_ABI_DIR=$(qt_dir_for_abi "$ABI")
  REPLACED=0
  for UNSUFFIXED in libavcodec.so libavformat.so libavutil.so libswresample.so libswscale.so; do
    TARGET_FILE="$ABI_DIR/$UNSUFFIXED"
    SOURCE_FILE="$QT_ABI_DIR/lib/$UNSUFFIXED"
    if [[ -f "$TARGET_FILE" && -f "$SOURCE_FILE" ]]; then
      cp -v "$SOURCE_FILE" "$TARGET_FILE"
      ((REPLACED++)) || true
    fi
  done
  echo "  $ABI: $REPLACED unsuffixed Libs durch korrekte Arch-Version ersetzt"
done

echo ""

# ─── Schritt 4: Validierung ────────────────────────────────────────────────

echo ""
echo "=== Dateien nach androiddeployqt ==="
for ABI in "${ABIS[@]}"; do
  COUNT=$(ls -1 "$ANDROID_BUILD_DIR/libs/$ABI/"*.so 2>/dev/null | wc -l)
  echo "  libs/$ABI: $COUNT .so-Dateien"
done

LIBS_XML=$(find "$ANDROID_BUILD_DIR" -name "libs.xml" 2>/dev/null | head -n1)
if [[ -n "$LIBS_XML" ]]; then
  echo "  libs.xml: $LIBS_XML"
  echo ""
  echo "=== libs.xml ==="
  cat "$LIBS_XML"
  echo ""
  echo "=== Ende libs.xml ==="
else
  echo "  libs.xml: NICHT GEFUNDEN"
  echo "  Suche nach allen XML-Dateien:"
  find "$ANDROID_BUILD_DIR" -name "*.xml" -path "*/values/*" 2>/dev/null || echo "  (keine gefunden)"
fi

echo "  gradle.properties: $([[ -f "$ANDROID_BUILD_DIR/gradle.properties" ]] && echo 'VORHANDEN' || echo 'NICHT GEFUNDEN')"
echo "  build.gradle: $([[ -f "$ANDROID_BUILD_DIR/build.gradle" ]] && echo 'VORHANDEN' || echo 'NICHT GEFUNDEN')"
echo "  gradlew: $([[ -f "$ANDROID_BUILD_DIR/gradlew" ]] && echo 'VORHANDEN' || echo 'NICHT GEFUNDEN')"

# ─── Gradle Build ───────────────────────────────────────────────────────────

if [[ -f "$ANDROID_BUILD_DIR/gradle.properties" ]]; then
  echo ""
  echo "Starte Gradle assembleRelease (nach Cross-Arch-Cleanup) ..."
  cd "$ANDROID_BUILD_DIR"

  if [[ ! -f "gradlew" ]]; then
    echo "ERROR: gradlew nicht gefunden in $ANDROID_BUILD_DIR"
    exit 8
  fi

  chmod +x gradlew
  ./gradlew assembleRelease --stacktrace

  cd -
else
  echo "WARNING: gradle.properties nicht gefunden in $ANDROID_BUILD_DIR"
  if [[ $DEPLOYQT_EXIT -ne 0 ]]; then
    echo "ERROR: androiddeployqt fehlgeschlagen und keine gradle.properties zum Patchen"
    exit $DEPLOYQT_EXIT
  fi
fi

# ─── Ergebnis ───────────────────────────────────────────────────────────────

echo ""
echo "Suche generiertes APK ..."
APK_FILE=$(find_built_android_apk "$ANDROID_BUILD_DIR")

if [[ -n "$APK_FILE" ]]; then
  echo ""
  echo "======================================"
  echo "  Universal-APK erfolgreich erstellt!"
  echo "  ABIs: ${ABIS[*]}"
  echo "  Pfad: $APK_FILE"

  if command -v aapt >/dev/null 2>&1; then
    echo ""
    echo "APK Info:"
    aapt dump badging "$APK_FILE" | grep -E "package|sdkVersion|targetSdkVersion|native-code"
  fi

  # Prüfe, ob beide ABIs im APK enthalten sind
  if command -v unzip >/dev/null 2>&1; then
    echo ""
    echo "Enthaltene native Libraries:"
    unzip -l "$APK_FILE" | grep "lib/.*\.so" || echo "(keine .so-Dateien gefunden)"
  fi

  echo "======================================"
else
  echo "WARNING: Kein generiertes APK gefunden"
  find "$ANDROID_BUILD_DIR" -type f -name "*.apk" || echo "Keine APK-Dateien"
fi

echo ""
echo "Fertig."
}

usage(){
  cat <<EOF
Usage: $0 [--arch arm64-v8a|armeabi-v7a|x86|x86_64] [--build-type Debug|Release] [--api-level N]

Install Android SDK/NDK and a Qt-for-Android build. Set ANDROID_SDK_ROOT, ANDROID_NDK_ROOT, JAVA_HOME, QT_ANDROID_DIR.

Fat APK (arm64 + armeabi-v7a): FAT_APK=yes with TARGET=pokerth_client; ignores --arch. See ${_DOC_ANDROID_FAT_REF}.
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

# Default build target; BUILD_TARGET wins (CI / scripts)
TARGET="${TARGET:-${BUILD_TARGET:-$DEFAULT_BUILD_TARGET}}"

# Fat APK (multi-ABI): pokerth_client only — docs/building-developer.md#android-fat-apk-reference
if is_yes "${FAT_APK:-}"; then
  require_fat_apk_pokerth_client_target
  build_android_fat_apk
  exit 0
fi

# Prüfe Android-Plattform
if [[ ! -d "${ANDROID_SDK_ROOT}/platforms/android-${API_LEVEL}" ]]; then
  echo "WARNING: Android platform android-${API_LEVEL} not found"
fi

android_set_build_tools_version_from_sdk verbose

command -v cmake >/dev/null || { echo "cmake not found"; exit 2; }

# Error text: manifest path depends on REPO_BUILD_ROOT (build_android/ native vs docker/<kind>/build/ for IN_DOCKER=1).
resolve_qt_cmake_cmd "qt-cmake not found. Expected ${MANIFEST_ENV} at ${_android_repo_mf} (toolchain cache: ${_android_cache_mf}) from setup deps, plus Qt host gcc_64 (QT_ANDROID_DIR/QT_HOST_PATH). See docs/building-developer.md."

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

  VCPKG_TRIPLET=$(vcpkg_triplet_for_android_abi "$ARCH")

  VCPKG_CMAKE_ARGS+=(
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_CMAKE_FILE"
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
    -DVCPKG_TARGET_TRIPLET="$VCPKG_TRIPLET"
  )
fi

# Same pattern as build_linux.sh / build.sh: REPO_BUILD_ROOT separates host (build_<platform>) vs Docker (docker/android/build)
if [ -n "${REPO_BUILD_ROOT:-}" ]; then
  BUILD_DIR_REL="$REPO_BUILD_ROOT"
else
  BUILD_DIR_REL="build_${TARGET_PLATFORM}"
fi
BUILD_DIR="$REPO_ROOT/$BUILD_DIR_REL"
mkdir -p "$BUILD_DIR"

invalidate_cmake_vcpkg "$BUILD_DIR"
invalidate_cmake_ndk "$BUILD_DIR"

# QT_CMAKE_CMD from resolve_qt_cmake_cmd (functions.sh); manifest PATH may omit ${QT_HOST_PATH}/bin.
run_android_qt_cmake_configure "$BUILD_DIR" "$ARCH" "$QT_ANDROID_DIR" "$(pwd)/$BUILD_DIR/install" "Configuring CMake..."

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
DEPLOY_JSON=$(find_android_deployment_settings_json "$BUILD_DIR" "$BUILD_SUBDIR")

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
write_pokerth_widgets_android_manifest "$ANDROID_BUILD_DIR/AndroidManifest.xml"

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
case "$ARCH" in
  arm64-v8a) OPENSSL_ARCH="arm64-v8a";;
  armeabi-v7a) OPENSSL_ARCH="armeabi-v7a";;
  x86_64) OPENSSL_ARCH="x86_64";;
  x86) OPENSSL_ARCH="x86";;
  *) OPENSSL_ARCH="$ARCH";;
esac
download_kdab_openssl3_libs "$OPENSSL_DIR" "$OPENSSL_ARCH" if_missing

resolve_androiddeployqt_or_exit

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

APK_FILE=$(find_built_android_apk "$ANDROID_BUILD_DIR")
if [[ -n "$APK_FILE" ]]; then
  echo "APK created: $APK_FILE"
else
  echo "WARNING: Could not find generated APK"
fi

echo "Done."
