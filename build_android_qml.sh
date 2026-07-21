#!/usr/bin/env bash

###
# Build script for the PokerTH QML client for Android. Builds every requested ABI
# and packs them into ONE multi-ABI Android App Bundle (.aab) ready for upload to
# the Google Play Store, plus an optional universal .apk for side-load testing
# (an .aab cannot be installed on a device directly).
#
# Runs on x86_64 Linux with sudo available; the same script is used by
# .github/workflows/android.yml. Every install step is idempotent, so a cached
# ~/Qt, ~/vcpkg and Android SDK turn the setup into a no-op.
#
#   bash build_android_qml.sh
#
# Environment overrides:
#   QT_VERSION        Qt version                          (default 6.9.3)
#   QT_OUTPUT_DIR     Qt install prefix                   (default $HOME/Qt)
#   VCPKG_DIR         vcpkg checkout                      (default $HOME/vcpkg)
#   ANDROID_SDK_ROOT  Android SDK                         (default $HOME/android-sdk)
#   ANDROID_ABIS      ABIs to build, space separated      (default: all four)
#   ANDROID_API_LEVEL compileSdk/targetSdk                (default 35)
#   ANDROID_MIN_SDK   minSdkVersion, also the API the
#                     native code is compiled against     (default 28 = Qt 6.8+ floor)
#   ANDROID_NDK_VERSION                                   (default 28.0.13004108)
#   VERSION_NAME      versionName                         (default: from src/game_defs.h)
#   VERSION_CODE      versionCode, must grow per upload   (default 23)
#   BUILD_TYPE        Release | Debug                     (default Release)
#   UNIVERSAL_APK     1 = also build a universal APK      (default 0)
#   ANDROID_KEYSTORE        upload keystore; unsigned bundle if unset
#   ANDROID_KEYSTORE_PASS   keystore password
#   ANDROID_KEY_ALIAS       key alias
#   ANDROID_KEY_PASS        key password (default: keystore password)
###

set -euo pipefail

BUILD_TARGET="pokerth_qml-client"
PACKAGE_NAME="${ANDROID_PACKAGE_NAME:-org.pokerth.qml}"
SCREEN_ORIENTATION="fullUser"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

QT_VERSION="${QT_VERSION:-6.9.3}"
QT_OUTPUT_DIR="${QT_OUTPUT_DIR:-$HOME/Qt}"
QT_HOST_PATH="${QT_HOST_PATH:-$QT_OUTPUT_DIR/$QT_VERSION/gcc_64}"

VCPKG_DIR="${VCPKG_DIR:-$HOME/vcpkg}"
VCPKG_HOST_TRIPLET="x64-linux"

ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/android-sdk}}"
ANDROID_NDK_VERSION="${ANDROID_NDK_VERSION:-28.0.13004108}"
ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/$ANDROID_NDK_VERSION"
ANDROID_API_LEVEL="${ANDROID_API_LEVEL:-35}"
ANDROID_MIN_SDK="${ANDROID_MIN_SDK:-28}"
ANDROID_BUILD_TOOLS_VERSION="${ANDROID_BUILD_TOOLS_VERSION:-${ANDROID_API_LEVEL}.0.0}"
# The first entry is the primary ABI: its androiddeployqt run is the one that
# completes the Gradle project every other ABI plugs into.
ANDROID_ABIS="${ANDROID_ABIS:-arm64-v8a armeabi-v7a x86_64 x86}"

BUILD_TYPE="${BUILD_TYPE:-Release}"
UNIVERSAL_APK="${UNIVERSAL_APK:-0}"
BUNDLETOOL_VERSION="${BUNDLETOOL_VERSION:-1.18.3}"

# Single source of truth for the release string (same #define the client reports
# to the server), so the store listing can never drift from the binary.
VERSION_NAME="${VERSION_NAME:-$(sed -n 's/^#define[[:space:]]*POKERTH_BETA_RELEASE_STRING[[:space:]]*"\([^"]*\)".*/\1/p' "$SCRIPT_DIR/src/game_defs.h")}"
VERSION_CODE="${VERSION_CODE:-23}"

ANDROID_KEYSTORE="${ANDROID_KEYSTORE:-}"
ANDROID_KEYSTORE_PASS="${ANDROID_KEYSTORE_PASS:-}"
ANDROID_KEY_ALIAS="${ANDROID_KEY_ALIAS:-}"
ANDROID_KEY_PASS="${ANDROID_KEY_PASS:-$ANDROID_KEYSTORE_PASS}"

read -r -a ABI_LIST <<< "$ANDROID_ABIS"
PRIMARY_ABI="${ABI_LIST[0]}"

# The Gradle project that ends up as the bundle: every ABI deploys into this one
# directory, each into its own libs/<abi>/.
ANDROID_BUILD_DIR="$SCRIPT_DIR/build-android-bundle"
ANDROID_SOURCE_DIR="$SCRIPT_DIR/src/gui/qt6-qml/android"

log() {
  echo ""
  echo "▶ $1"
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

# aqt / vcpkg / NDK names for one Android ABI.
qt_arch_for_abi() {
  case "$1" in
    arm64-v8a)   echo "android_arm64_v8a" ;;
    armeabi-v7a) echo "android_armv7" ;;
    x86)         echo "android_x86" ;;
    x86_64)      echo "android_x86_64" ;;
    *) echo "ERROR: unsupported ABI '$1'" >&2; exit 1 ;;
  esac
}

vcpkg_triplet_for_abi() {
  case "$1" in
    arm64-v8a)   echo "arm64-android" ;;
    armeabi-v7a) echo "arm-android" ;;
    x86)         echo "x86-android" ;;
    x86_64)      echo "x64-android" ;;
    *) echo "ERROR: unsupported ABI '$1'" >&2; exit 1 ;;
  esac
}

echo "=== PokerTH Android App Bundle build ==="
echo "ABIs:        ${ABI_LIST[*]}"
echo "Version:     $VERSION_NAME ($VERSION_CODE)"
echo "SDK:         compile/target $ANDROID_API_LEVEL, min $ANDROID_MIN_SDK"
echo "Qt:          $QT_VERSION"
if [ -n "$ANDROID_KEYSTORE" ]; then
  echo "Signing:     $ANDROID_KEYSTORE (alias: $ANDROID_KEY_ALIAS)"
else
  echo "Signing:     none — the bundle will be unsigned"
fi

########################################
# 1. Base tools
########################################

declare -a MISSING_PKGS=()
command_exists cmake        || MISSING_PKGS+=(cmake)
command_exists ninja        || MISSING_PKGS+=(ninja-build)
command_exists jq           || MISSING_PKGS+=(jq)
command_exists envsubst     || MISSING_PKGS+=(gettext-base)
command_exists rsvg-convert || MISSING_PKGS+=(librsvg2-bin)
command_exists unzip        || MISSING_PKGS+=(unzip)
command_exists zip          || MISSING_PKGS+=(zip)
command_exists curl         || MISSING_PKGS+=(curl)
command_exists g++          || MISSING_PKGS+=(build-essential)
command_exists pkg-config   || MISSING_PKGS+=(pkg-config)

if ((${#MISSING_PKGS[@]})); then
  log "Installing base packages: ${MISSING_PKGS[*]}"
  sudo apt-get update
  sudo apt-get install -y --no-install-recommends "${MISSING_PKGS[@]}"
fi

if ! command_exists aqt; then
  log "Installing aqtinstall…"
  python3 -m venv "$HOME/.aqt-venv"
  "$HOME/.aqt-venv/bin/pip" install --upgrade pip aqtinstall
fi
if [ -d "$HOME/.aqt-venv/bin" ]; then
  export PATH="$HOME/.aqt-venv/bin:$PATH"
fi

if [ -z "${JAVA_HOME:-}" ]; then
  JAVA_HOME="$(dirname "$(dirname "$(readlink -f "$(command -v javac)")")")"
  export JAVA_HOME
fi
log "JAVA_HOME: $JAVA_HOME"

########################################
# 2. Android SDK, build tools and NDK
########################################

SDKMANAGER="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
if [ ! -x "$SDKMANAGER" ]; then
  log "Installing Android command line tools…"
  mkdir -p "$ANDROID_SDK_ROOT/cmdline-tools"
  curl -fSL -o /tmp/cmdline-tools.zip \
    https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
  unzip -q -o /tmp/cmdline-tools.zip -d /tmp/cmdline-tools-extract
  rm -rf "$ANDROID_SDK_ROOT/cmdline-tools/latest"
  mv /tmp/cmdline-tools-extract/cmdline-tools "$ANDROID_SDK_ROOT/cmdline-tools/latest"
  rm -rf /tmp/cmdline-tools.zip /tmp/cmdline-tools-extract
fi

log "Installing SDK platform $ANDROID_API_LEVEL, build-tools $ANDROID_BUILD_TOOLS_VERSION, NDK $ANDROID_NDK_VERSION…"
yes | "$SDKMANAGER" --sdk_root="$ANDROID_SDK_ROOT" --licenses >/dev/null 2>&1 || true
"$SDKMANAGER" --sdk_root="$ANDROID_SDK_ROOT" \
  "platform-tools" \
  "platforms;android-${ANDROID_API_LEVEL}" \
  "build-tools;${ANDROID_BUILD_TOOLS_VERSION}" \
  "ndk;${ANDROID_NDK_VERSION}" > /dev/null

# vcpkg's *-android triplets locate the toolchain through ANDROID_NDK_HOME.
export ANDROID_NDK_HOME="$ANDROID_NDK_ROOT"
export ANDROID_SDK_ROOT

########################################
# 3. Qt for Android (one kit per ABI) + host Qt
########################################

# Everything else the client needs (Core, Gui, Widgets, Quick, QuickControls2,
# Qml, Sql, Xml, Svg, Network, LinguistTools) ships with the Qt base package.
QT_MODULES=(
  qt5compat
  qtimageformats
  qtmultimedia
  qtshadertools
  qtwebsockets
)

if [ -x "$QT_HOST_PATH/bin/qmlimportscanner" ]; then
  log "Host Qt $QT_VERSION already installed at: $QT_HOST_PATH"
else
  log "Installing host Qt $QT_VERSION (moc/rcc/qmlimportscanner/androiddeployqt)…"
  aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 \
    --outputdir "$QT_OUTPUT_DIR" \
    --modules "${QT_MODULES[@]}"
fi

for ABI in "${ABI_LIST[@]}"; do
  QT_ARCH="$(qt_arch_for_abi "$ABI")"
  QT_ANDROID_DIR="$QT_OUTPUT_DIR/$QT_VERSION/$QT_ARCH"
  if [ -f "$QT_ANDROID_DIR/lib/cmake/Qt6/qt.toolchain.cmake" ]; then
    log "Qt $QT_VERSION for $ABI already installed at: $QT_ANDROID_DIR"
  else
    log "Installing Qt $QT_VERSION for $ABI ($QT_ARCH)…"
    aqt install-qt all_os android "$QT_VERSION" "$QT_ARCH" \
      --outputdir "$QT_OUTPUT_DIR" \
      --modules "${QT_MODULES[@]}"
  fi
done

########################################
# 4. vcpkg dependencies (one triplet per ABI)
########################################

# Boost, OpenSSL and protobuf are not shipped by Qt; vcpkg's *-android triplets
# cross-compile them against the NDK. protoc is built for the host triplet
# automatically (vcpkg host dependency).
declare -a VCPKG_PORTS=(
  boost-any
  boost-asio
  boost-atomic
  boost-chrono
  boost-container
  boost-date-time
  boost-filesystem
  boost-foreach
  boost-interprocess
  boost-iostreams
  boost-lambda
  boost-program-options
  boost-random
  boost-regex
  boost-serialization
  boost-smart-ptr
  boost-system
  boost-thread
  boost-uuid
  openssl
  protobuf
)

if [ ! -d "$VCPKG_DIR" ]; then
  log "Cloning vcpkg…"
  git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi
if [ ! -x "$VCPKG_DIR/vcpkg" ]; then
  log "Bootstrapping vcpkg…"
  "$VCPKG_DIR/bootstrap-vcpkg.sh" -disableMetrics
fi

# protobuf's Android build trips over the NDK linker's executable-stack warning
# (emulated TLS objects carry a .note.GNU-stack that lld flags). An overlay port
# re-adds the upstream port with the warning turned off — same workaround the
# Android dev container uses.
PROTOBUF_OVERLAY_DIR="$VCPKG_DIR/pokerth-overlay-ports"
if [ ! -f "$PROTOBUF_OVERLAY_DIR/protobuf/portfile.cmake" ]; then
  log "Creating protobuf overlay port (execstack workaround)…"
  mkdir -p "$PROTOBUF_OVERLAY_DIR/protobuf"
  cp -r "$VCPKG_DIR/ports/protobuf/." "$PROTOBUF_OVERLAY_DIR/protobuf/"
  sed -i '1i\
# Workaround: NDK lld warns about executable stacks in protobuf'"'"'s emulated-TLS objects\
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-warn-execstack")\
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-warn-execstack")\
' "$PROTOBUF_OVERLAY_DIR/protobuf/portfile.cmake"
fi

# vcpkg's stock arm-android triplet still configures ports with
# -DANDROID_ARM_NEON=OFF. Since NDK r26 the toolchain hard-fails on that
# ("Disabling Neon is no longer supported"), so vcpkg cannot even probe the
# compiler and no port ever builds. This overlay is that triplet minus the dead
# flag — NEON is mandatory on every armeabi-v7a device Google Play still serves,
# so there is nothing to preserve. The name matches the built-in triplet, which
# keeps the install path (and with it the deployment settings) unchanged.
TRIPLET_OVERLAY_DIR="$VCPKG_DIR/pokerth-overlay-triplets"
mkdir -p "$TRIPLET_OVERLAY_DIR"
cat > "$TRIPLET_OVERLAY_DIR/arm-android.cmake" <<'EOF'
set(VCPKG_TARGET_ARCHITECTURE arm)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Android)
set(VCPKG_CMAKE_SYSTEM_VERSION 28)
set(VCPKG_MAKE_BUILD_TRIPLET "--host=armv7a-linux-androideabi")
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DANDROID_ABI=armeabi-v7a)
EOF

for ABI in "${ABI_LIST[@]}"; do
  TRIPLET="$(vcpkg_triplet_for_abi "$ABI")"
  log "Installing vcpkg dependencies for $ABI ($TRIPLET)…"
  # --clean-after-build drops buildtrees/packages of each port once installed;
  # without it the vcpkg tree grows to several GB per triplet (boost) and no
  # longer fits into a GitHub Actions cache entry.
  "$VCPKG_DIR/vcpkg" install \
    --triplet="$TRIPLET" \
    --host-triplet="$VCPKG_HOST_TRIPLET" \
    --overlay-ports="$PROTOBUF_OVERLAY_DIR" \
    --overlay-triplets="$TRIPLET_OVERLAY_DIR" \
    --clean-after-build \
    "${VCPKG_PORTS[@]}"
done

########################################
# 5. Compile the client for every ABI
########################################

for ABI in "${ABI_LIST[@]}"; do
  QT_ANDROID_DIR="$QT_OUTPUT_DIR/$QT_VERSION/$(qt_arch_for_abi "$ABI")"
  TRIPLET="$(vcpkg_triplet_for_abi "$ABI")"
  BUILD_DIR="$SCRIPT_DIR/build-android-$ABI"

  log "Configuring CMake for $ABI…"
  # Toolchain layering mirrors the iOS build: vcpkg's toolchain is the entry
  # point and chain-loads the NDK's, Qt is pulled in through CMAKE_PREFIX_PATH.
  # ANDROID_PLATFORM is pinned to minSdk (not to compileSdk): the native code
  # must only bind symbols that exist on the oldest device we ship to.
  "$QT_HOST_PATH/bin/qt-cmake" -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
    -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
    -DVCPKG_HOST_TRIPLET="$VCPKG_HOST_TRIPLET" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM="android-$ANDROID_MIN_SDK" \
    -DANDROID_NATIVE_API_LEVEL="$ANDROID_MIN_SDK" \
    -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
    -DQT_HOST_PATH="$QT_HOST_PATH" \
    -DQt6_DIR="$QT_ANDROID_DIR/lib/cmake/Qt6" \
    -DCMAKE_PREFIX_PATH="$QT_ANDROID_DIR/lib/cmake" \
    -DCMAKE_FIND_ROOT_PATH="$QT_ANDROID_DIR" \
    -DProtobuf_USE_STATIC_LIBS=ON \
    -DQT_ANDROID_SDK_BUILD_TOOLS_REVISION="$ANDROID_BUILD_TOOLS_VERSION"

  log "Building $BUILD_TARGET for $ABI ($BUILD_TYPE)…"
  cmake --build "$BUILD_DIR" --target "$BUILD_TARGET" -j"$(nproc)"
done

########################################
# 6. AndroidManifest.xml and launcher icons
########################################

# Both are generated into the *package source directory*, which androiddeployqt
# copies over its own template — that is the only place where they survive the
# res/ rebuild androiddeployqt does on every run.
log "Generating AndroidManifest.xml (package=$PACKAGE_NAME, $VERSION_NAME/$VERSION_CODE)…"
export PACKAGE_NAME VERSION_NAME VERSION_CODE SCREEN_ORIENTATION
export API_LEVEL="$ANDROID_API_LEVEL" MIN_SDK="$ANDROID_MIN_SDK" TARGET="$BUILD_TARGET"
envsubst '${PACKAGE_NAME} ${VERSION_NAME} ${VERSION_CODE} ${API_LEVEL} ${MIN_SDK} ${TARGET} ${SCREEN_ORIENTATION}' \
  < "$SCRIPT_DIR/docker/android/AndroidManifest.xml.template" \
  > "$ANDROID_SOURCE_DIR/AndroidManifest.xml"

log "Generating launcher icons from pokerth.svg…"
ICON_SVG="$SCRIPT_DIR/src/gui/qt6-qml/resources/pokerth.svg"
declare -A MIPMAP_SIZES=(
  [mipmap-mdpi]=48
  [mipmap-hdpi]=72
  [mipmap-xhdpi]=96
  [mipmap-xxhdpi]=144
  [mipmap-xxxhdpi]=192
)
for MIPMAP in "${!MIPMAP_SIZES[@]}"; do
  SIZE="${MIPMAP_SIZES[$MIPMAP]}"
  mkdir -p "$ANDROID_SOURCE_DIR/res/$MIPMAP"
  rsvg-convert -w "$SIZE" -h "$SIZE" "$ICON_SVG" -o "$ANDROID_SOURCE_DIR/res/$MIPMAP/ic_launcher.png"
done
# Play requires a 512x512 icon for the store listing; generate it alongside.
rsvg-convert -w 512 -h 512 "$ICON_SVG" -o "$SCRIPT_DIR/pokerth-play-icon-512.png"

########################################
# 7. androiddeployqt: collect every ABI into one Gradle project
########################################

ANDROIDDEPLOYQT="$QT_HOST_PATH/bin/androiddeployqt"
[ -x "$ANDROIDDEPLOYQT" ] || { echo "ERROR: androiddeployqt not found at $ANDROIDDEPLOYQT" >&2; exit 1; }

rm -rf "$ANDROID_BUILD_DIR"

# Deploy order matters. Every run copies Qt's QML modules into
# assets/android_rcc_bundle/, but only a *full* run packs that directory into
# android_rcc_bundle.rcc and deletes it again — a --copy-dependencies-only run
# stops before that step. So the secondary ABIs go first and the primary's full
# run closes the bundle; the other way round each secondary ABI would leave a
# raw copy of Qt's QML tree in the assets.
declare -a DEPLOY_ORDER=()
for ABI in "${ABI_LIST[@]}"; do
  [ "$ABI" = "$PRIMARY_ABI" ] || DEPLOY_ORDER+=("$ABI")
done
DEPLOY_ORDER+=("$PRIMARY_ABI")

for ABI in "${DEPLOY_ORDER[@]}"; do
  BUILD_DIR="$SCRIPT_DIR/build-android-$ABI"

  DEPLOY_JSON="$(find "$BUILD_DIR/src/gui/qt6-qml" -name "*deployment-settings.json" -print -quit 2>/dev/null || true)"
  [ -n "$DEPLOY_JSON" ] || { echo "ERROR: no deployment-settings.json for $ABI — did qt_finalize_target() run?" >&2; exit 1; }

  # CMake leaves "sdk"/"sdkBuildToolsRevision" empty and takes the SDK versions
  # from the target properties; patch in what this build actually targets.
  TMP_JSON="$(mktemp)"
  jq --arg sdk "$ANDROID_SDK_ROOT" \
     --arg bt "$ANDROID_BUILD_TOOLS_VERSION" \
     --arg api "$ANDROID_API_LEVEL" \
     --arg min "$ANDROID_MIN_SDK" \
     '.sdk = $sdk |
      .sdkBuildToolsRevision = $bt |
      .["android-target-sdk-version"] = $api |
      .["android-min-sdk-version"] = $min' \
     "$DEPLOY_JSON" > "$TMP_JSON"
  mv "$TMP_JSON" "$DEPLOY_JSON"

  # androiddeployqt expects the app binary under the name lib<target>_<abi>.so
  # and refuses to package an ABI whose binary is missing.
  SO_FILE="$(find "$BUILD_DIR" -type f -name "lib${BUILD_TARGET}*.so" -print -quit 2>/dev/null || true)"
  [ -n "$SO_FILE" ] || { echo "ERROR: lib${BUILD_TARGET}.so not found for $ABI" >&2; exit 1; }
  mkdir -p "$ANDROID_BUILD_DIR/libs/$ABI"
  cp "$SO_FILE" "$ANDROID_BUILD_DIR/libs/$ABI/lib${BUILD_TARGET}_${ABI}.so"

  # Qt's TLS backend dlopen()s libssl_3/libcrypto_3 at runtime; vcpkg's OpenSSL
  # is linked statically into the port consumers and does not provide them.
  # They have to sit in libs/<abi>/ *before* androiddeployqt runs, otherwise it
  # drops the openssl TLS plugin as having unmet dependencies (= no HTTPS).
  OPENSSL_BASE_URL="https://github.com/KDAB/android_openssl/raw/master/ssl_3"
  for SSL_LIB in libssl_3.so libcrypto_3.so; do
    curl -fsSL -o "$ANDROID_BUILD_DIR/libs/$ABI/$SSL_LIB" "$OPENSSL_BASE_URL/$ABI/$SSL_LIB"
  done

  if [ "$ABI" = "$PRIMARY_ABI" ]; then
    # Full run: copies Qt's Gradle template and the package source directory
    # (manifest, icons, ConnectionService.java), packs the rcc bundle and writes
    # gradle.properties. It ends with a single-ABI "assembleRelease" whose APK we
    # throw away — androiddeployqt has no mode that stops after generating the
    # project but still copies the package sources. The real package is the
    # bundleRelease below, once every ABI is in place.
    log "Deploying $ABI (primary — completes the Gradle project)…"
    "$ANDROIDDEPLOYQT" \
      --input "$DEPLOY_JSON" \
      --output "$ANDROID_BUILD_DIR" \
      --android-platform "android-${ANDROID_API_LEVEL}" \
      --jdk "$JAVA_HOME" \
      --release
  else
    # Secondary ABIs only contribute their native libraries and Qt assets. Each
    # is deployed from its *own* settings file: Qt kit, vcpkg prefix and plugin
    # list differ per ABI, and androiddeployqt resolves those flat lists against
    # the first match it finds — one merged multi-ABI file would hand every ABI
    # the same (wrong) libraries.
    log "Deploying $ABI (adds libs/$ABI to the Gradle project)…"
    "$ANDROIDDEPLOYQT" \
      --input "$DEPLOY_JSON" \
      --output "$ANDROID_BUILD_DIR" \
      --android-platform "android-${ANDROID_API_LEVEL}" \
      --jdk "$JAVA_HOME" \
      --copy-dependencies-only
  fi
done

########################################
# 8. Bundle every ABI into one .aab
########################################

set_gradle_property() {
  local key="$1" value="$2" file="$ANDROID_BUILD_DIR/gradle.properties"
  if grep -q "^$key=" "$file"; then
    sed -i "s|^$key=.*|$key=$value|" "$file"
  else
    echo "$key=$value" >> "$file"
  fi
}

# androiddeployqt only knows about the primary ABI, so it limits the build to
# it (build.gradle: ndk.abiFilters = qtTargetAbiList). Widen it to everything
# that is now sitting in libs/.
ABI_CSV="$(IFS=,; echo "${ABI_LIST[*]}")"
log "Bundling ABIs: $ABI_CSV"
set_gradle_property qtTargetAbiList "$ABI_CSV"

# Qt 6.9 pins AGP 8.8, which only knows SDKs up to 35 and aborts on anything
# newer unless the check is waived explicitly.
if [ "$ANDROID_API_LEVEL" -gt 35 ]; then
  set_gradle_property android.suppressUnsupportedCompileSdk "$ANDROID_API_LEVEL"
fi

log "Running Gradle bundleRelease…"
chmod +x "$ANDROID_BUILD_DIR/gradlew"
(cd "$ANDROID_BUILD_DIR" && ./gradlew bundleRelease --no-daemon --stacktrace)

AAB_FILE="$(find "$ANDROID_BUILD_DIR/build/outputs/bundle" -name "*.aab" -print -quit)"
[ -n "$AAB_FILE" ] || { echo "ERROR: Gradle produced no .aab" >&2; exit 1; }

FINAL_AAB="$SCRIPT_DIR/PokerTH-${VERSION_NAME}-${VERSION_CODE}.aab"
cp "$AAB_FILE" "$FINAL_AAB"

########################################
# 9. Sign the bundle with the upload key
########################################

if [ -n "$ANDROID_KEYSTORE" ]; then
  log "Signing the bundle with the upload key…"
  jarsigner -keystore "$ANDROID_KEYSTORE" \
    -storepass "$ANDROID_KEYSTORE_PASS" \
    -keypass "$ANDROID_KEY_PASS" \
    -sigalg SHA256withRSA -digestalg SHA-256 \
    "$FINAL_AAB" "$ANDROID_KEY_ALIAS"
  jarsigner -verify "$FINAL_AAB" > /dev/null
  echo "Bundle signed and verified."
else
  echo ""
  echo "WARNING: no ANDROID_KEYSTORE set — the bundle is UNSIGNED and Google Play"
  echo "         will reject it. It is only good for local inspection."
fi

########################################
# 10. Universal APK for side-load testing
########################################

if [ "$UNIVERSAL_APK" = "1" ]; then
  log "Building a universal APK from the bundle (bundletool $BUNDLETOOL_VERSION)…"
  BUNDLETOOL_JAR="$HOME/.cache/bundletool-all-${BUNDLETOOL_VERSION}.jar"
  if [ ! -f "$BUNDLETOOL_JAR" ]; then
    mkdir -p "$(dirname "$BUNDLETOOL_JAR")"
    curl -fsSL -o "$BUNDLETOOL_JAR" \
      "https://github.com/google/bundletool/releases/download/${BUNDLETOOL_VERSION}/bundletool-all-${BUNDLETOOL_VERSION}.jar"
  fi

  APKS_FILE="$SCRIPT_DIR/PokerTH-${VERSION_NAME}-${VERSION_CODE}-universal.apks"
  declare -a BUNDLETOOL_SIGN_ARGS=()
  if [ -n "$ANDROID_KEYSTORE" ]; then
    BUNDLETOOL_SIGN_ARGS=(
      --ks="$ANDROID_KEYSTORE"
      --ks-pass="pass:$ANDROID_KEYSTORE_PASS"
      --ks-key-alias="$ANDROID_KEY_ALIAS"
      --key-pass="pass:$ANDROID_KEY_PASS"
    )
  fi

  rm -f "$APKS_FILE"
  java -jar "$BUNDLETOOL_JAR" build-apks \
    --bundle="$FINAL_AAB" \
    --output="$APKS_FILE" \
    --mode=universal \
    "${BUNDLETOOL_SIGN_ARGS[@]}"

  # A .apks archive is a ZIP; the universal mode puts exactly one APK inside.
  unzip -p "$APKS_FILE" universal.apk > "$SCRIPT_DIR/PokerTH-${VERSION_NAME}-${VERSION_CODE}-universal.apk"
  rm -f "$APKS_FILE"
fi

########################################
# 11. Summary
########################################

echo ""
echo "======================================"
echo "Android App Bundle created:"
ls -lh "$FINAL_AAB"
echo ""
echo "ABIs in the bundle:"
unzip -l "$FINAL_AAB" | sed -n 's|.*base/lib/\([^/]*\)/.*|  \1|p' | sort -u
if [ "$UNIVERSAL_APK" = "1" ]; then
  echo ""
  echo "Universal APK for side-load testing:"
  ls -lh "$SCRIPT_DIR/PokerTH-${VERSION_NAME}-${VERSION_CODE}-universal.apk"
fi
echo ""
echo "Store icon: $SCRIPT_DIR/pokerth-play-icon-512.png"
echo "======================================"
