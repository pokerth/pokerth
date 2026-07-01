#!/usr/bin/env bash
set -euo pipefail

# Minimaler Android-Build-Helper für ${TARGET} (Template)
# Erwartet als Umgebungsvariablen:
#  ANDROID_SDK_ROOT, ANDROID_NDK_ROOT, JAVA_HOME, QT_ANDROID_DIR

usage(){
  cat <<EOF
Usage: $0 [--arch arm64-v8a|armeabi-v7a|x86|x86_64] [--build-type Debug|Release] [--api-level 28]

Wichtig: Installiere Android SDK/NDK und eine Qt-for-Android-Build-Installation.
Setze mindestens ANDROID_SDK_ROOT, ANDROID_NDK_ROOT, JAVA_HOME und QT_ANDROID_DIR.
EOF
}
ARCH=${ANDROID_ARCH:-x64}
# if [[ $ARCH = "x64" ]]
# then
#   ARCH="x86_64"
# fi
BUILD_TYPE=Release
API_LEVEL=${ANDROID_API_LEVEL:-35}
# minSdkVersion (Geräte-Mindeststufe). Default 28 = Android 9.0.
# Überschreibbar via ANDROID_MIN_SDK (z.B. 26 für Android 8.0 / HUAWEI RNE-L21).
MIN_SDK=${ANDROID_MIN_SDK:-28}
# Nativer Compile-Level (ANDROID_NATIVE_API_LEVEL / ANDROID_PLATFORM). Default =
# API_LEVEL. Entkoppelt von compileSdk/targetSdk, weil ältere NDKs ein niedrigeres
# Maximum haben (NDK r26b: max android-34) bzw. man die nativen Libs gezielt für
# eine ältere Geräte-API bauen will (z.B. 26 für Android 8.0).
NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL:-$API_LEVEL}
TARGET=${TARGET:-pokerth_qml-client}
BUILD_TIMESTAMP=$(date +%Y%m%d_%H%M%S)

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
echo "arch=$ARCH build=$BUILD_TYPE api-level=$API_LEVEL native-api-level=$NATIVE_API_LEVEL min-sdk=$MIN_SDK"

# Validiere erlaubte ABIs
case "$ARCH" in
  arm64-v8a|armeabi-v7a|x86|x86_64) ;;
  *)
    echo "Unsupported arch: $ARCH"
    exit 1
    ;;
esac

: ${ANDROID_SDK_ROOT:?Please set ANDROID_SDK_ROOT}
: ${ANDROID_NDK_ROOT:?Please set ANDROID_NDK_ROOT}
: ${JAVA_HOME:?Please set JAVA_HOME}
: ${QT_ANDROID_DIR:?Please set QT_ANDROID_DIR (Qt installation for Android)}

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

BUILD_DIR=build-android-${ARCH}
mkdir -p "$BUILD_DIR"

# CMake Initial Cache
cat > "$BUILD_DIR/InitialCache.cmake" <<EOF
set(ANDROID_SDK_BUILD_TOOLS_REVISION "$BUILD_TOOLS_VERSION" CACHE STRING "")
set(QT_ANDROID_SDK_BUILD_TOOLS_REVISION "$BUILD_TOOLS_VERSION" CACHE STRING "")
EOF

echo "Configuring CMake..."
qt-cmake -S . -B "$BUILD_DIR" -G Ninja \
  -C "$BUILD_DIR/InitialCache.cmake" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
  "${VCPKG_CMAKE_ARGS[@]}" \
  -DANDROID_ABI="$ARCH" \
  -DANDROID_NATIVE_API_LEVEL="$NATIVE_API_LEVEL" \
  -DCMAKE_PREFIX_PATH="${QT_ANDROID_DIR}/lib/cmake" \
  -DCMAKE_FIND_ROOT_PATH=${QT_ANDROID_DIR} \
  -DQt6_DIR="${QT_ANDROID_DIR}/lib/cmake/Qt6" \
  ${QT_HOST_PATH:+-DQT_HOST_PATH="$QT_HOST_PATH"} \
  -DCMAKE_INSTALL_PREFIX="$(pwd)/$BUILD_DIR/install" \
  -DProtobuf_USE_STATIC_LIBS=ON

echo "Building target '${TARGET}'..."
cmake --build "$BUILD_DIR" --target ${TARGET} -j $(nproc || echo 1)

echo "Build finished. Artefacts in: $BUILD_DIR"

# Bestimme das Android Source Directory basierend auf dem Target
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
  echo "Searching in entire build directory..."
  DEPLOY_JSON=$(find "$BUILD_DIR" -type f -name "*deployment-settings.json" 2>/dev/null | head -n1 || true)
fi

if [[ -z "$DEPLOY_JSON" ]]; then
  echo "ERROR: No deployment settings JSON found."
  echo "Searched in: $BUILD_DIR/$BUILD_SUBDIR and $BUILD_DIR"
  echo ""
  echo "Available JSON files in build directory:"
  find "$BUILD_DIR" -type f -name "*.json" 2>/dev/null || echo "No JSON files found"
  echo ""
  echo "This usually means qt_finalize_target() didn't generate the deployment settings."
  echo "Check if the CMakeLists.txt for $TARGET calls qt_finalize_target()."
  exit 10
fi

echo "Found deployment settings: $DEPLOY_JSON"

# Patche deployment-settings.json - WICHTIG: Ändere application-binary!
if command -v jq >/dev/null 2>&1; then
  echo "Patching deployment settings JSON..."
  TMP_JSON=$(mktemp)
  
  # Patche ALLE relevanten Felder UND setze application-binary auf den tatsächlichen Target-Namen
  jq --arg bt "$BUILD_TOOLS_VERSION" \
     --arg al "$API_LEVEL" \
     --arg min "$MIN_SDK" \
     --arg arch "$ARCH" \
     --arg target "$TARGET" \
     --arg android_src "$ANDROID_SOURCE_DIR" \
     --arg sdkroot "$ANDROID_SDK_ROOT" \
    '.["android-build-tools-revision"] = $bt |
     .["android-sdk-build-tools-revision"] = $bt |
     .["android-target-sdk-version"] = $al |
     .["android-min-sdk-version"] = $min |
     .["target-architecture"] = $arch |
     .["application-binary"] = $target |
     .["android-package-source-directory"] = $android_src |
     .sdk = $sdkroot |
     .sdkBuildToolsRevision = $bt' \
    "$DEPLOY_JSON" > "$TMP_JSON"
  mv "$TMP_JSON" "$DEPLOY_JSON"
  
  echo "Deployment settings after patch:"
  jq '.["application-binary"], .["android-package-source-directory"], .["target-architecture"]' "$DEPLOY_JSON"
else
  echo "WARNING: jq not found, cannot patch deployment settings"
fi

# Erstelle Android Build-Verzeichnisstruktur
mkdir -p "$ANDROID_BUILD_DIR/libs/$ARCH"

# Kopiere Android-Manifest und Ressourcen BEVOR androiddeployqt läuft
if [[ -d "$ANDROID_SOURCE_DIR" ]]; then
  echo "Copying Android source files from: $ANDROID_SOURCE_DIR"
  cp -rv "$ANDROID_SOURCE_DIR"/* "$ANDROID_BUILD_DIR/" || true
fi

# Erstelle Verzeichnisse für Ressourcen
mkdir -p "$ANDROID_BUILD_DIR/res/drawable"
mkdir -p "$ANDROID_BUILD_DIR/res/values"

# Package-Name, Version und Orientierung pro Target.
VERSION_CODE="20"
if [[ $TARGET == "pokerth_qml-client" ]]; then
  PACKAGE_NAME="org.pokerth.qml"
  VERSION_NAME="2.1.0"
  SCREEN_ORIENTATION="fullUser"
else
  PACKAGE_NAME="org.pokerth.widget"
  VERSION_NAME="2.1.0"
  SCREEN_ORIENTATION="landscape"
fi

# Generiere AndroidManifest.xml aus Template und schreibe es direkt in
# ANDROID_SOURCE_DIR, damit androiddeployqt immer die aktuelle Version liest.
# Sicherstellen, dass das Verzeichnis existiert (z.B. fehlt src/gui/qt/android/
# beim Widget-Client initial).
mkdir -p "$ANDROID_SOURCE_DIR"
MANIFEST_TEMPLATE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/AndroidManifest.xml.template"
if [[ ! -f "$MANIFEST_TEMPLATE" ]]; then
  echo "ERROR: Manifest-Template nicht gefunden: $MANIFEST_TEMPLATE"
  exit 1
fi
export PACKAGE_NAME VERSION_NAME VERSION_CODE API_LEVEL MIN_SDK TARGET SCREEN_ORIENTATION
envsubst '${PACKAGE_NAME} ${VERSION_NAME} ${VERSION_CODE} ${API_LEVEL} ${MIN_SDK} ${TARGET} ${SCREEN_ORIENTATION}' \
  < "$MANIFEST_TEMPLATE" > "$ANDROID_SOURCE_DIR/AndroidManifest.xml"
echo "AndroidManifest.xml generiert: package=$PACKAGE_NAME version=$VERSION_NAME/$VERSION_CODE lib=$TARGET minSdk=$MIN_SDK"

# Wenn das Package im Gradle-Cache noch unter einem anderen Namen gespeichert ist
# (z.B. nach einem QML-Build), bricht AAPT mit "resource mipmap/ic_launcher not found"
# ab. Gradle-Build-Cache leeren, damit ein sauberer Neubau erzwungen wird.
CACHED_PKG_FILE="$ANDROID_BUILD_DIR/.last_package_name"
if [[ -f "$CACHED_PKG_FILE" ]] && [[ "$(cat "$CACHED_PKG_FILE")" != "$PACKAGE_NAME" ]]; then
  echo "Package-Name geändert ($(cat "$CACHED_PKG_FILE") → $PACKAGE_NAME) – lösche Gradle-Build-Cache."
  rm -rf "$ANDROID_BUILD_DIR/build"
fi
echo "$PACKAGE_NAME" > "$CACHED_PKG_FILE"

# Finde .so-Datei - suche sowohl nach lib${TARGET}.so als auch nach Varianten
SO_FILE=$(find "$BUILD_DIR" -type f \( -name "lib${TARGET}.so" -o -name "lib${TARGET}_*.so" \) | head -n1)

if [[ -z "$SO_FILE" ]]; then
  echo "ERROR: Could not find lib${TARGET}*.so"
  echo "Searching for any .so files in build directory:"
  find "$BUILD_DIR" -type f -name "*.so" | head -20
  exit 6
fi

echo "Found library: $SO_FILE"

# Kopiere mit dem Namen, den androiddeployqt erwartet
EXPECTED_SO_NAME="lib${TARGET}_${ARCH}.so"
echo "Copying library as: $EXPECTED_SO_NAME"
cp -v "$SO_FILE" "$ANDROID_BUILD_DIR/libs/$ARCH/$EXPECTED_SO_NAME"

# Erstelle auch einen Symlink mit dem ursprünglichen Namen
ln -sf "$EXPECTED_SO_NAME" "$ANDROID_BUILD_DIR/libs/$ARCH/lib${TARGET}.so"

# Überprüfe, ob die Datei kopiert wurde
if [[ ! -f "$ANDROID_BUILD_DIR/libs/$ARCH/$EXPECTED_SO_NAME" ]]; then
  echo "ERROR: Failed to copy library to $ANDROID_BUILD_DIR/libs/$ARCH/"
  exit 9
fi

echo "Library successfully copied to: $ANDROID_BUILD_DIR/libs/$ARCH/$EXPECTED_SO_NAME"

# Download OpenSSL 3.x libraries for Android (required for Qt Network HTTPS)
echo ""
echo "Downloading OpenSSL 3.x libraries for Android..."
OPENSSL_DIR="$ANDROID_BUILD_DIR/libs/$ARCH"
mkdir -p "$OPENSSL_DIR"

OPENSSL_BASE_URL="https://github.com/KDAB/android_openssl/raw/master/ssl_3"
if [[ "$ARCH" == "arm64-v8a" ]]; then
  OPENSSL_ARCH="arm64-v8a"
elif [[ "$ARCH" == "armeabi-v7a" ]]; then
  OPENSSL_ARCH="armeabi-v7a"
elif [[ "$ARCH" == "x86_64" ]]; then
  OPENSSL_ARCH="x86_64"
elif [[ "$ARCH" == "x86" ]]; then
  OPENSSL_ARCH="x86"
else
  echo "WARNING: Unknown architecture $ARCH for OpenSSL download"
  OPENSSL_ARCH="$ARCH"
fi

echo "Downloading OpenSSL for architecture: $OPENSSL_ARCH"
wget -q -O "$OPENSSL_DIR/libssl_3.so" "$OPENSSL_BASE_URL/$OPENSSL_ARCH/libssl_3.so" || echo "WARNING: Failed to download libssl_3.so"
wget -q -O "$OPENSSL_DIR/libcrypto_3.so" "$OPENSSL_BASE_URL/$OPENSSL_ARCH/libcrypto_3.so" || echo "WARNING: Failed to download libcrypto_3.so"

if [[ -f "$OPENSSL_DIR/libssl_3.so" && -f "$OPENSSL_DIR/libcrypto_3.so" ]]; then
  echo "OpenSSL libraries downloaded successfully"
  ls -lh "$OPENSSL_DIR"/lib{ssl,crypto}_3.so
else
  echo "WARNING: OpenSSL download incomplete - HTTPS may not work"
fi

# Verwende androiddeployqt
ANDROIDDEPLOYQT="${QT_HOST_PATH}/bin/androiddeployqt"

if [[ ! -x "$ANDROIDDEPLOYQT" ]]; then
  echo "ERROR: androiddeployqt not found at $ANDROIDDEPLOYQT"
  exit 7
fi

echo ""
echo "Running androiddeployqt..."
set +e
"$ANDROIDDEPLOYQT" \
  --input "$DEPLOY_JSON" \
  --output "$ANDROID_BUILD_DIR" \
  --android-platform "android-${API_LEVEL}" \
  --jdk "$JAVA_HOME" \
  --verbose
DEPLOYQT_EXIT=$?
set -e

echo ""
echo "androiddeployqt exit code: $DEPLOYQT_EXIT"

# Generiere App-Icon aus pokerth.svg in alle Mipmap-Dichten (NACH androiddeployqt,
# da androiddeployqt res/ neu anlegt und eigene Icons überschreiben würde).
echo ""
echo "Generating PokerTH mipmap icons from pokerth.svg..."
ICON_SVG="${PWD}/src/gui/qt6-qml/resources/pokerth.svg"
if [[ ! -f "$ICON_SVG" ]]; then
  echo "WARNING: $ICON_SVG not found – skipping icon generation"
elif ! command -v rsvg-convert &>/dev/null; then
  echo "WARNING: rsvg-convert not found – skipping icon generation (install librsvg2-bin)"
else
  declare -A MIPMAP_SIZES=(
    [mipmap-mdpi]=48
    [mipmap-hdpi]=72
    [mipmap-xhdpi]=96
    [mipmap-xxhdpi]=144
    [mipmap-xxxhdpi]=192
  )
  for MIPMAP in "${!MIPMAP_SIZES[@]}"; do
    SIZE="${MIPMAP_SIZES[$MIPMAP]}"
    DEST_DIR="$ANDROID_BUILD_DIR/res/$MIPMAP"
    mkdir -p "$DEST_DIR"
    rsvg-convert -w "$SIZE" -h "$SIZE" "$ICON_SVG" -o "$DEST_DIR/ic_launcher.png"
    echo "  $MIPMAP/ic_launcher.png  (${SIZE}×${SIZE})"
  done
  echo "Icon generation complete."
fi

# Prüfe und patche gradle.properties (nicht build.gradle!)
if [[ -f "$ANDROID_BUILD_DIR/gradle.properties" ]]; then
  echo ""
  echo "Checking and patching gradle.properties..."
  
  echo "Current gradle.properties content:"
  cat "$ANDROID_BUILD_DIR/gradle.properties"
  
  echo ""
  echo "Applying patch..."
  
  # Setze oder aktualisiere androidBuildToolsVersion in gradle.properties
  if grep -q "^androidBuildToolsVersion=" "$ANDROID_BUILD_DIR/gradle.properties"; then
    # Ersetze existierende Zeile
    sed -i "s/^androidBuildToolsVersion=.*/androidBuildToolsVersion=$BUILD_TOOLS_VERSION/" "$ANDROID_BUILD_DIR/gradle.properties"
  else
    # Füge neue Zeile hinzu
    echo "androidBuildToolsVersion=$BUILD_TOOLS_VERSION" >> "$ANDROID_BUILD_DIR/gradle.properties"
  fi
  
  # Setze auch compileSdkVersion falls nötig
  if ! grep -q "^androidCompileSdkVersion=" "$ANDROID_BUILD_DIR/gradle.properties"; then
    echo "androidCompileSdkVersion=$API_LEVEL" >> "$ANDROID_BUILD_DIR/gradle.properties"
  fi
  
  echo ""
  echo "After patch:"
  cat "$ANDROID_BUILD_DIR/gradle.properties"
  
  # Führe Gradle Build manuell aus
  echo ""
  echo "Running Gradle build manually..."
  cd "$ANDROID_BUILD_DIR"
  
  if [[ ! -f "gradlew" ]]; then
    echo "ERROR: gradlew not found in $ANDROID_BUILD_DIR"
    exit 8
  fi
  
  chmod +x gradlew
  ./gradlew assembleRelease --stacktrace
  
  cd -
else
  echo "WARNING: gradle.properties not found at $ANDROID_BUILD_DIR/gradle.properties"
  
  if [[ $DEPLOYQT_EXIT -ne 0 ]]; then
    echo "ERROR: androiddeployqt failed and no gradle.properties to fix"
    echo ""
    echo "Listing android-build directory contents:"
    ls -la "$ANDROID_BUILD_DIR" || true
    exit $DEPLOYQT_EXIT
  fi
fi

echo ""
echo "Looking for generated APK..."
APK_FILE=$(find "$ANDROID_BUILD_DIR" -type f -name "*.apk" | grep -E "(release|debug)" | grep -v "unaligned" | head -n1)

if [[ -n "$APK_FILE" ]]; then
  FINAL_APK="${TARGET}_${VERSION_NAME}_${ARCH}_${BUILD_TIMESTAMP}.apk"
  cp -v "$APK_FILE" "$FINAL_APK"

  echo ""
  echo "======================================"
  echo "APK created successfully!"
  echo "Output: $FINAL_APK"

  if command -v aapt >/dev/null 2>&1; then
    echo ""
    echo "APK Info:"
    aapt dump badging "$FINAL_APK" | grep -E "package|sdkVersion|targetSdkVersion"
  fi

  echo "======================================"
else
  echo "WARNING: Could not find generated APK"
  echo "APK files in build directory:"
  find "$ANDROID_BUILD_DIR" -type f -name "*.apk" || echo "No APK files found"
fi

echo "Done."