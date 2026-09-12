#!/usr/bin/env bash

###
# Build script for the PokerTH Android client (QML by default, or the classic
# Widgets client via BUILD_TARGET). Two output formats:
#   PACKAGE_FORMAT=aab (default) — builds every requested ABI and packs them into
#     ONE multi-ABI Android App Bundle (.aab) for the Google Play Store, plus an
#     optional universal .apk for side-load testing (an .aab cannot be installed
#     on a device directly).
#   PACKAGE_FORMAT=apk — builds ONE single-ABI, directly installable .apk
#     (ANDROID_ABIS must name exactly one ABI). Used by the per-variant matrix in
#     .github/workflows/android-apk.yml.
#
# Runs on x86_64 Linux with sudo available; the same script is used by
# .github/workflows/android.yml. Every install step is idempotent, so a cached
# ~/Qt, ~/vcpkg and Android SDK turn the setup into a no-op.
#
#   bash build_android_qml.sh
#
# Environment overrides:
#   BUILD_TARGET      pokerth_qml-client | pokerth_client (default qml client)
#   PACKAGE_FORMAT    aab | apk                           (default aab)
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
#   VERSION_CODE      versionCode, must grow per upload   (default 28)
#   BUILD_TYPE        Release | Debug                     (default Release)
#   UNIVERSAL_APK     1 = also build a universal APK      (default 0)
#   ANDROID_KEYSTORE        upload keystore; unsigned bundle if unset
#   ANDROID_KEYSTORE_PASS   keystore password
#   ANDROID_KEY_ALIAS       key alias
#   ANDROID_KEY_PASS        key password (default: keystore password)
###

set -euo pipefail

# Which client to package. The QML client lives in src/gui/qt6-qml, the classic
# Widgets client in src/gui/qt; each has its own package name, launcher
# orientation, short label and androiddeployqt package source directory.
BUILD_TARGET="${BUILD_TARGET:-pokerth_qml-client}"
case "$BUILD_TARGET" in
  pokerth_qml-client)
    # Permanent Google Play application id (chosen in the Play Console, can never
    # be changed there). Shared with the iOS bundle id and the F-Droid app id;
    # only macOS differs (net.pokerth.PokerTH). The ConnectionService Java class
    # stays org.pokerth.qml.ConnectionService (absolute name in the manifest, its
    # source lives under android/src/org/pokerth/qml/), so it keeps resolving
    # regardless of this id.
    PACKAGE_NAME="${ANDROID_PACKAGE_NAME:-net.pokerth.PokerTH_QML}"
    SCREEN_ORIENTATION="fullUser"
    GUI_SUBDIR="src/gui/qt6-qml"
    APP_LABEL="qml"
    ;;
  pokerth_client)
    PACKAGE_NAME="${ANDROID_PACKAGE_NAME:-org.pokerth.widget}"
    SCREEN_ORIENTATION="landscape"
    GUI_SUBDIR="src/gui/qt"
    APP_LABEL="widget"
    ;;
  *)
    echo "ERROR: unsupported BUILD_TARGET '$BUILD_TARGET' (use pokerth_qml-client or pokerth_client)" >&2
    exit 1
    ;;
esac

# aab = one multi-ABI Android App Bundle (Play Store); apk = one single-ABI APK
# (side-load / per-device testing). apk mode requires exactly one ABI.
PACKAGE_FORMAT="${PACKAGE_FORMAT:-aab}"

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
# Google's read-only mirror of Maven Central (full sync, unmetered), queried
# before Central itself — see patch_gradle_template_mirror().
MAVEN_CENTRAL_MIRROR="${MAVEN_CENTRAL_MIRROR:-https://maven-central.storage-download.googleapis.com/maven2/}"

# Single source of truth for the release string (same #define the client reports
# to the server), so the store listing can never drift from the binary.
VERSION_NAME="${VERSION_NAME:-$(sed -n 's/^#define[[:space:]]*POKERTH_BETA_RELEASE_STRING[[:space:]]*"\([^"]*\)".*/\1/p' "$SCRIPT_DIR/src/game_defs.h")}"
# Bumped with every release, in step with docker/android/build_android.sh — a
# local run without VERSION_CODE must not produce an APK that Android refuses to
# install over the previous release (INSTALL_FAILED_VERSION_DOWNGRADE). The
# workflows always pass their own code and never reach this default.
VERSION_CODE="${VERSION_CODE:-28}"

ANDROID_KEYSTORE="${ANDROID_KEYSTORE:-}"
ANDROID_KEYSTORE_PASS="${ANDROID_KEYSTORE_PASS:-}"
ANDROID_KEY_ALIAS="${ANDROID_KEY_ALIAS:-}"
ANDROID_KEY_PASS="${ANDROID_KEY_PASS:-$ANDROID_KEYSTORE_PASS}"

read -r -a ABI_LIST <<< "$ANDROID_ABIS"
PRIMARY_ABI="${ABI_LIST[0]}"

if [ "$PACKAGE_FORMAT" = "apk" ] && [ "${#ABI_LIST[@]}" -ne 1 ]; then
  echo "ERROR: PACKAGE_FORMAT=apk builds a single-ABI APK — set ANDROID_ABIS to exactly one ABI (got: $ANDROID_ABIS)" >&2
  exit 1
fi

# The Gradle project that ends up as the bundle: every ABI deploys into this one
# directory, each into its own libs/<abi>/.
ANDROID_BUILD_DIR="$SCRIPT_DIR/build-android-bundle"
ANDROID_SOURCE_DIR="$SCRIPT_DIR/$GUI_SUBDIR/android"

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

# Puts MAVEN_CENTRAL_MIRROR in front of every mavenCentral() of a Qt kit's
# Gradle template. Idempotent — ~/Qt is cached between CI runs, so this sees
# already patched kits, and a kit whose template carries no mavenCentral() at all
# is reported instead of silently staying unpatched.
patch_gradle_template_mirror() {
  local qt_android_dir="$1" tpl patched=0
  for tpl in "$qt_android_dir"/src/android/templates/build.gradle \
             "$qt_android_dir"/src/android/templates/settings.gradle; do
    [ -f "$tpl" ] || continue
    if grep -q "$MAVEN_CENTRAL_MIRROR" "$tpl"; then
      patched=1
      continue
    fi
    grep -qE '^[[:space:]]*mavenCentral\(\)' "$tpl" || continue
    sed -i -E "s|^([[:space:]]*)mavenCentral\(\)|\1maven { url '$MAVEN_CENTRAL_MIRROR' }\n\1mavenCentral()|g" "$tpl"
    patched=1
  done
  [ "$patched" = 1 ] || echo "WARNING: no mavenCentral() in $qt_android_dir/src/android/templates — Maven Central is used unmirrored and may answer HTTP 429" >&2
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

if [ "$PACKAGE_FORMAT" = "apk" ]; then
  echo "=== PokerTH Android APK build ($APP_LABEL, $PRIMARY_ABI) ==="
else
  echo "=== PokerTH Android App Bundle build ($APP_LABEL) ==="
fi
echo "Target:      $BUILD_TARGET  ($PACKAGE_NAME)"
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

  # Maven Central rate-limits shared CI egress and answers HTTP 429 ("Too Many
  # Requests"), which Gradle treats as a fatal resolution error instead of a
  # reason to try the next repository — the build then dies while resolving AGP's
  # own classpath. Google's read-only Central mirror goes in front of every
  # repository list of the template, Central itself stays as the fallback.
  #
  # The template (not the generated project) is the only place this can be
  # patched: the first Gradle run of a build is the one androiddeployqt starts
  # itself, before we ever touch build-android-bundle/.
  patch_gradle_template_mirror "$QT_ANDROID_DIR"

  # Qt < 6.8's Gradle template hardcodes AGP 7.4.1, whose aapt2 cannot read
  # android-34+ and which conflicts with androidx.core:1.13.1 (needs
  # compileSdk >= 34). Raise it to AGP 8.2.2 — the newest AGP still compatible
  # with the Gradle 8.x these kits ship — and build such variants with
  # ANDROID_API_LEVEL=34. Idempotent: a no-op once patched (or on Qt >= 6.8).
  case "$QT_VERSION" in
    6.5.*|6.6.*|6.7.*)
      GRADLE_TEMPLATE="$QT_ANDROID_DIR/src/android/templates/build.gradle"
      [ -f "$GRADLE_TEMPLATE" ] && sed -i \
        's|com.android.tools.build:gradle:7.4.1|com.android.tools.build:gradle:8.2.2|' \
        "$GRADLE_TEMPLATE"
      ;;
  esac
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

# androiddeployqt is called ONCE for all ABIs, from one merged settings file.
# It is not a convenience: the Qt libraries an app loads at startup are listed
# in res/values/libs.xml, and only a full run writes that list. A run per ABI —
# the primary full, the rest with --copy-dependencies-only — copies every .so
# into place but leaves libs.xml with the primary ABI alone, so on an x86_64
# device QtLoader asks for "libQt6Core_arm64-v8a.so" and the app dies before it
# starts (UnsatisfiedLinkError). androiddeployqt handles the multi-ABI case
# itself: "qt" and "architectures" are maps of ABI -> Qt kit / toolchain triple,
# it loops over them and keeps each ABI's libraries apart, and "stdcpp-path" is
# a base directory it combines with each triple. The app binaries and the
# OpenSSL libraries only have to be in libs/<abi>/ before it runs.
# Collected while preparing each ABI, consumed by the single deploy run below.
QT_DIR_MAP="{}"
ARCH_MAP="{}"
PRIMARY_DEPLOY_JSON=""

for ABI in "${ABI_LIST[@]}"; do
  BUILD_DIR="$SCRIPT_DIR/build-android-$ABI"

  DEPLOY_JSON="$(find "$BUILD_DIR/$GUI_SUBDIR" -name "*deployment-settings.json" -print -quit 2>/dev/null || true)"
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

  # Every ABI has its own Qt kit and toolchain triple — those are the only two
  # fields that differ between the per-ABI settings files, and the merged file
  # carries them as maps. CMake writes "qt" as a plain string for a single-ABI
  # build and as a map for a multi-ABI one, so accept both.
  ABI_QT_DIR="$(jq -r 'if (.qt | type) == "object" then (.qt | to_entries[0].value) else .qt end' "$DEPLOY_JSON")"
  ABI_TRIPLE="$(jq -r '.architectures | to_entries[0].value' "$DEPLOY_JSON")"
  QT_DIR_MAP="$(jq -cn --argjson m "$QT_DIR_MAP" --arg a "$ABI" --arg v "$ABI_QT_DIR" '$m + {($a): $v}')"
  ARCH_MAP="$(jq -cn --argjson m "$ARCH_MAP" --arg a "$ABI" --arg v "$ABI_TRIPLE" '$m + {($a): $v}')"
  if [ "$ABI" = "$PRIMARY_ABI" ]; then
    PRIMARY_DEPLOY_JSON="$DEPLOY_JSON"
  fi

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

done

# One settings file for every ABI: all fields are identical across the per-ABI
# files except "qt" and "architectures", which androiddeployqt reads as maps of
# ABI -> Qt kit / toolchain triple and loops over. Nothing else may be merged in
# here — "android-deploy-plugins" in particular is a flat list of ABI-suffixed
# paths that androiddeployqt copies without checking their architecture, which
# is why CMake no longer writes one (see qt6-qml/CMakeLists.txt, Android branch).
#
# extraPrefixDirs/extraLibraryDirs are emptied for the same reason, one level
# deeper: CMake puts the Qt kit of *this* ABI in there ("Usually android
# deployment settings contain Qt install directory in extraPrefixDirs" —
# androiddeployqt's own comment), and those directories are searched BEFORE
# qtInstallDirectory, with the first hit for a file returned outright. In a
# single multi-ABI run that means every secondary ABI looks into the primary
# ABI's kit first, finds its dependency XML and plugins there, and drops them
# again on the ELF architecture check — silently, unless --verbose is on. The
# result is an ABI that ends up without its multimedia plugin. Emptied, the
# search uses qtInstallDirectory alone, which is correct for each ABI. Nothing
# is lost: the vcpkg libraries are linked statically, and the only shared ones
# (libssl/libcrypto) are copied into libs/<abi>/ by this script.
MERGED_JSON="$ANDROID_BUILD_DIR-deployment-settings.json"
jq --argjson qt "$QT_DIR_MAP" --argjson arch "$ARCH_MAP" \
   '.qt = $qt | .architectures = $arch
    | .extraPrefixDirs = [] | .extraLibraryDirs = []' \
   "$PRIMARY_DEPLOY_JSON" > "$MERGED_JSON"

# The fields that decide which kit each ABI is deployed from — printed because
# getting them wrong costs a full build to notice.
log "Merged deployment settings:"
jq -r '{qt, architectures, extraPrefixDirs, extraLibraryDirs}' "$MERGED_JSON" | sed 's/^/  /'

log "Deploying ${#ABI_LIST[@]} ABI(s) in one androiddeployqt run: $(IFS=,; echo "${ABI_LIST[*]}")…"
# The run ends with an "assembleRelease" whose APK we throw away — there is no
# mode that generates the Gradle project without building it. The real package
# is the bundleRelease (or the assembleRelease of section 8a) below.
"$ANDROIDDEPLOYQT" \
  --input "$MERGED_JSON" \
  --output "$ANDROID_BUILD_DIR" \
  --android-platform "android-${ANDROID_API_LEVEL}" \
  --jdk "$JAVA_HOME" \
  --release

# The check that would have caught the bug above: every ABI in the package must
# appear in Qt's load list, or its devices crash on startup with
# UnsatisfiedLinkError. libs.xml is generated text at this point.
LIBS_XML="$ANDROID_BUILD_DIR/res/values/libs.xml"
[ -f "$LIBS_XML" ] || { echo "ERROR: androiddeployqt wrote no $LIBS_XML" >&2; exit 1; }
for ABI in "${ABI_LIST[@]}"; do
  # Entries look like "<item>x86_64;libQt6Core_x86_64.so:…</item>"; anchoring on
  # "<item>$ABI;" keeps "x86" from matching inside "x86_64".
  if ! grep -q "<item>$ABI;" "$LIBS_XML"; then
    echo "ERROR: $ABI is missing from $LIBS_XML — Qt would look for another" >&2
    echo "       ABI's libraries on those devices and the app would not start." >&2
    exit 1
  fi
done
log "Qt load list covers: $(IFS=,; echo "${ABI_LIST[*]}")"

########################################
# 7a. Drop the FFmpeg multimedia backend
########################################

# androiddeployqt deploys every multimedia plugin the Qt kit offers, so each ABI
# gets both backends. FFmpeg has to go, and not for cosmetic reasons: the FFmpeg
# libraries in the Qt kit align their ELF segments to 4 KB, so Google Play
# rejects a bundle containing them as "does not support 16 KB memory pages".
# Nothing here needs them — on Android the audio player runs the software mixer
# (see qtaudioplayer.cpp), which decodes the WAVs itself and only needs a
# QAudioSink, which the Android backend provides. It also saves ~16 MB per ABI.
# This is done here rather than with qt_import_plugins() in CMake because that
# would write an "android-deploy-plugins" list, which has no per-ABI form and
# breaks the single multi-ABI deploy run (see qt6-qml/CMakeLists.txt).
LIBS_XML="$ANDROID_BUILD_DIR/res/values/libs.xml"
for ABI in "${ABI_LIST[@]}"; do
  LIB_DIR="$ANDROID_BUILD_DIR/libs/$ABI"
  [ -d "$LIB_DIR" ] || continue
  DROPPED=""
  for DEAD in "libplugins_multimedia_ffmpegmediaplugin_${ABI}.so" \
              libavcodec.so libavformat.so libavutil.so \
              libswresample.so libswscale.so; do
    if [ -f "$LIB_DIR/$DEAD" ]; then
      rm -f "$LIB_DIR/$DEAD"
      DROPPED="$DROPPED $DEAD"
    fi
  done
  [ -z "$DROPPED" ] || log "Removed the FFmpeg backend from libs/$ABI:$DROPPED"
  # Without any multimedia plugin the client starts but stays silent, and that
  # is exactly the kind of thing nobody notices until a player reports it.
  if [ ! -f "$LIB_DIR/libplugins_multimedia_androidmediaplugin_${ABI}.so" ]; then
    echo "ERROR: no Android multimedia plugin in libs/$ABI — the app would have" >&2
    echo "       no audio backend at all. Check the qtmultimedia module of the" >&2
    echo "       Qt kit for $ABI." >&2
    exit 1
  fi
done

# Qt loads every library listed in libs.xml at startup and refuses to start if
# one is missing, so the entries of the files just deleted have to go too. The
# lists are ":"-separated, hence the two passes (entry in the middle, entry at
# the end).
if [ -f "$LIBS_XML" ]; then
  for DEAD_RE in 'libplugins_multimedia_ffmpegmediaplugin_[^:<]*\.so' \
                 'libav[a-z]*\.so' 'libsw[a-z]*\.so'; do
    sed -i -e "s/${DEAD_RE}://g" -e "s/:${DEAD_RE}//g" "$LIBS_XML"
  done
  if grep -qE 'ffmpegmediaplugin|libav[a-z]*\.so|libsw[a-z]*\.so' "$LIBS_XML"; then
    echo "ERROR: FFmpeg entries survive in $LIBS_XML — Qt would try to load a" >&2
    echo "       library that is no longer in the package and refuse to start." >&2
    grep -nE 'ffmpegmediaplugin|libav[a-z]*\.so|libsw[a-z]*\.so' "$LIBS_XML" >&2
    exit 1
  fi
fi

########################################
# 8. Package: one multi-ABI .aab, or one single-ABI .apk
########################################

set_gradle_property() {
  local key="$1" value="$2" file="$ANDROID_BUILD_DIR/gradle.properties"
  if grep -q "^$key=" "$file"; then
    sed -i "s|^$key=.*|$key=$value|" "$file"
  else
    echo "$key=$value" >> "$file"
  fi
}

# build.gradle limits the package to ndk.abiFilters = qtTargetAbiList, which
# androiddeployqt writes from the "architectures" map of the merged settings
# file — every ABI of this build. Verify rather than set it: a mismatch here
# silently drops ABIs from the package.
ABI_CSV="$(IFS=,; echo "${ABI_LIST[*]}")"
GRADLE_ABIS="$(sed -n 's/^qtTargetAbiList=//p' "$ANDROID_BUILD_DIR/gradle.properties" | tr ',' '\n' | sort | tr '\n' ' ')"
EXPECTED_ABIS="$(printf '%s\n' "${ABI_LIST[@]}" | sort | tr '\n' ' ')"
if [ -z "$GRADLE_ABIS" ]; then
  # Not written at all (older androiddeployqt): set it rather than fail.
  log "qtTargetAbiList missing from gradle.properties — setting it to $ABI_CSV"
  set_gradle_property qtTargetAbiList "$ABI_CSV"
elif [ "$GRADLE_ABIS" != "$EXPECTED_ABIS" ]; then
  echo "ERROR: gradle.properties has qtTargetAbiList=[$GRADLE_ABIS], expected [$EXPECTED_ABIS]." >&2
  echo "       Gradle would drop the missing ABIs from the package." >&2
  exit 1
fi

# AGP aborts on a compileSdk it does not know unless the check is waived
# explicitly: Qt 6.9 pins AGP 8.8 (max SDK 35), the Qt 6.7 kit is raised to AGP
# 8.2 (max SDK 34) above.
if [ "$ANDROID_API_LEVEL" -gt 35 ]; then
  set_gradle_property android.suppressUnsupportedCompileSdk "$ANDROID_API_LEVEL"
fi

# Native debug symbols: keep the symbol tables, drop the debug info. AGP strips
# every .so it ships either way; 'SYMBOL_TABLE' additionally files the symbol
# tables under BUNDLE-METADATA/, from where Play takes them to symbolicate
# native crashes and ANRs — without them the Play Console shows bare addresses
# and warns on every upload. 'FULL' would add the DWARF debug info on top, which
# a Release build does not even produce (no -g), so it would cost size for
# nothing. Qt's own libraries arrive stripped and contribute next to nothing;
# what this is really about is libpokerth_qml-client_<abi>.so. None of it is
# ever downloaded by a device — it only travels inside the bundle, where Play
# caps the symbols at 800 MB (section 11 prints what we actually use).
BUILD_GRADLE="$ANDROID_BUILD_DIR/build.gradle"
if [ -f "$BUILD_GRADLE" ] && ! grep -q "debugSymbolLevel" "$BUILD_GRADLE"; then
  log "Keeping native symbol tables in the bundle (debugSymbolLevel 'SYMBOL_TABLE')…"
  cat >> "$BUILD_GRADLE" <<'EOF'

// Added by build_android_qml.sh: keep the native symbol tables so Play can
// symbolicate crashes and ANRs. They live in BUNDLE-METADATA/ and are never
// delivered to a device. 'FULL' would add DWARF a Release build does not have.
android {
    buildTypes {
        release {
            ndk {
                debugSymbolLevel 'SYMBOL_TABLE'
            }
        }
    }
}
EOF
fi

chmod +x "$ANDROID_BUILD_DIR/gradlew"

########################################
# 8. 16 KB page compatibility check
########################################

# Google Play refuses an upload that is not 16 KB compatible ("Your app does not
# support 16 KB memory pages" / "Deine App unterstützt keine Speicherseiten mit
# 16 KB"). Two independent things decide that, and both are checked here on the
# FINISHED package — the answer belongs in this build log, not in a Play Console
# message after an hour-long build:
#
#   1. ELF alignment: every 64-bit .so must align its LOAD segments to 16 KB
#      (0x4000), otherwise it does not even load on a 16 KB-page device. Our own
#      libraries get there through the linker flags in CMakeLists.txt
#      (add_link_options, Android branch), Qt's own libraries since 6.9.3
#      (QTBUG-131514) and the KDAB OpenSSL prebuilts since their 16 KB rebuild.
#      Qt's bundled FFmpeg libraries are still 4 KB aligned — section 7a drops
#      them, they are not needed here. 32-bit ABIs are exempt (no 32-bit device
#      runs 16 KB pages) and are only listed, so an unaligned armeabi-v7a
#      library cannot be mistaken for the real thing.
#   2. Packaging: the libraries have to sit UNCOMPRESSED on 16 KB ZIP boundaries
#      in the installed package. That is what android:extractNativeLibs="false"
#      (AndroidManifest.xml.template) plus AGP >= 8.5.1 produce. A package with
#      compressed libraries is rejected even when every single .so is aligned —
#      which is exactly what happened to the first upload.
check_16k_alignment() {
  local pkg="$1"
  local readelf tmp so rel abi aligns a v min total bad64 bad32
  readelf="$(ls "$ANDROID_NDK_ROOT"/toolchains/llvm/prebuilt/*/bin/llvm-readelf 2>/dev/null | head -1)"
  echo ""
  echo "16 KB page compatibility ($(basename "$pkg")):"
  if [ -z "$readelf" ]; then
    echo "  SKIPPED — llvm-readelf not found in $ANDROID_NDK_ROOT"
    return 0
  fi
  tmp="$(mktemp -d)"
  # APK and AAB are both ZIPs: libraries live in lib/<abi>/ (AAB: base/lib/<abi>/).
  unzip -q -o "$pkg" 'lib/*/*.so' 'base/lib/*/*.so' -d "$tmp" 2>/dev/null || true
  total=0; bad64=0; bad32=0
  while IFS= read -r so; do
    total=$((total + 1))
    abi="$(basename "$(dirname "$so")")"
    # Program headers: the last column of every LOAD line is its alignment.
    aligns="$("$readelf" -lW "$so" 2>/dev/null | awk '$1 == "LOAD" { print $NF }')"
    min=""
    for a in $aligns; do
      v=$((a))
      if [ -z "$min" ] || [ "$v" -lt "$min" ]; then min=$v; fi
    done
    [ -n "$min" ] || min=0
    [ "$min" -lt 16384 ] || continue
    rel="${so#"$tmp"/}"
    case "$abi" in
      arm64-v8a|x86_64|riscv64)
        bad64=$((bad64 + 1))
        printf '  NOT ALIGNED (%s bytes): %s\n' "$min" "$rel" ;;
      *)
        bad32=$((bad32 + 1))
        printf '  %s bytes, 32-bit ABI — exempt, not checked by Play: %s\n' "$min" "$rel" ;;
    esac
  done < <(find "$tmp" -name '*.so' | sort)
  rm -rf "$tmp"

  if [ "$total" -eq 0 ]; then
    echo "  no shared libraries found in the package"
  elif [ "$bad64" -eq 0 ]; then
    echo "  OK — all 64-bit libraries of $total are aligned to 16 KB or more."
  else
    echo ""
    echo "  WARNING: $bad64 of $total shared libraries are NOT 16 KB aligned."
    echo "           Play rejects this, and the app fails to start on devices with"
    echo "           16 KB memory pages. Own libraries: check the linker flags in"
    echo "           CMakeLists.txt. Qt/NDK/OpenSSL libraries: only a newer kit"
    echo "           helps — they ship prebuilt and cannot be relinked here."
  fi

  check_lib_packaging "$pkg"
  return 0
}

# Second half of the requirement: how the libraries are stored in the package.
check_lib_packaging() {
  local pkg="$1" zipalign compressed flag
  case "$pkg" in
    *.apk)
      # Only an APK carries the layout a device sees. "Stored" = uncompressed;
      # zipalign -c -P 16 then verifies the 16 KB boundaries themselves.
      compressed="$(unzip -lv "$pkg" | awk '$NF ~ /^lib\/.*\.so$/ && $2 != "Stored" { print "    " $NF }')"
      zipalign="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION/zipalign"
      if [ -n "$compressed" ]; then
        echo "  WARNING: native libraries are COMPRESSED — Play rejects the package"
        echo "           as not 16 KB compatible even when every .so is aligned."
        echo "           Fix: android:extractNativeLibs=\"false\" in"
        echo "           docker/android/AndroidManifest.xml.template."
        echo "$compressed"
      elif [ ! -x "$zipalign" ]; then
        echo "  packaging: libraries are uncompressed (zipalign not found, boundaries unchecked)"
      elif "$zipalign" -c -P 16 4 "$pkg" > /dev/null 2>&1; then
        echo "  OK — libraries are uncompressed and on 16 KB ZIP boundaries."
      else
        echo "  WARNING: libraries are uncompressed but NOT on 16 KB ZIP boundaries"
        echo "           (zipalign -c -P 16 4 failed) — that needs AGP >= 8.5.1."
      fi
      ;;
    *)
      # Inside an .aab every entry is deflated by definition; the split APKs Play
      # generates from it take their packaging from the manifest flag, so that
      # flag — read back from the manifest this build actually generated — is
      # the only thing worth reporting here.
      flag="$(sed -n 's/.*android:extractNativeLibs="\([^"]*\)".*/\1/p' \
              "$ANDROID_SOURCE_DIR/AndroidManifest.xml" 2>/dev/null | head -1)"
      case "$flag" in
        false)
          echo "  OK — extractNativeLibs=\"false\": Play ships the libraries uncompressed" ;;
        "")
          echo "  OK — extractNativeLibs unset: AGP defaults to uncompressed libraries" ;;
        *)
          echo "  WARNING: extractNativeLibs=\"$flag\" — Play rejects the bundle as not"
          echo "           16 KB compatible. Fix it in"
          echo "           docker/android/AndroidManifest.xml.template." ;;
      esac
      ;;
  esac
}

########################################
# 8a. Single-ABI APK path (PACKAGE_FORMAT=apk)
########################################

if [ "$PACKAGE_FORMAT" = "apk" ]; then
  log "Running Gradle assembleRelease (single-ABI APK: $PRIMARY_ABI)…"
  (cd "$ANDROID_BUILD_DIR" && ./gradlew assembleRelease --no-daemon --stacktrace)

  RAW_APK="$(find "$ANDROID_BUILD_DIR/build/outputs/apk" -name "*.apk" ! -name "*unaligned*" -print -quit)"
  [ -n "$RAW_APK" ] || { echo "ERROR: Gradle produced no .apk" >&2; exit 1; }

  FINAL_APK="$SCRIPT_DIR/PokerTH-${APP_LABEL}-${VERSION_NAME}-${VERSION_CODE}-${PRIMARY_ABI}.apk"
  BT_DIR="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION"

  if [ -n "$ANDROID_KEYSTORE" ]; then
    log "Signing the APK with the upload key (zipalign + apksigner)…"
    "$BT_DIR/zipalign" -f -p 4 "$RAW_APK" "$FINAL_APK"
    "$BT_DIR/apksigner" sign \
      --ks "$ANDROID_KEYSTORE" \
      --ks-pass "pass:$ANDROID_KEYSTORE_PASS" \
      --ks-key-alias "$ANDROID_KEY_ALIAS" \
      --key-pass "pass:$ANDROID_KEY_PASS" \
      "$FINAL_APK"
    "$BT_DIR/apksigner" verify "$FINAL_APK" > /dev/null
    echo "APK signed and verified."
  else
    cp "$RAW_APK" "$FINAL_APK"
    echo ""
    echo "WARNING: no ANDROID_KEYSTORE set — the APK is UNSIGNED. Sign it before"
    echo "         installing:  apksigner sign --ks <keystore> '$FINAL_APK'"
  fi

  # Everything a failing side-load install comes down to — package name,
  # versionCode, minSdk, packaged ABI and the signing certificate — printed from
  # the finished file, so a "cannot install" report can be answered from the
  # build log instead of from the device. In particular: a versionCode below the
  # one already installed, or a certificate other than the one that signed the
  # previous install, both fail with the same bare "App not installed".
  check_16k_alignment "$FINAL_APK"

  log "APK contents (this is what the device checks on install):"
  "$BT_DIR/aapt2" dump badging "$FINAL_APK" |
    grep -E "^(package|sdkVersion|targetSdkVersion|native-code)" || true
  if [ -n "$ANDROID_KEYSTORE" ]; then
    "$BT_DIR/apksigner" verify --print-certs "$FINAL_APK" |
      grep -E "Signer #1 certificate (DN|SHA-256)" || true
  fi

  echo ""
  echo "======================================"
  echo "Android APK created:"
  ls -lh "$FINAL_APK"
  echo "  target: $BUILD_TARGET   ABI: $PRIMARY_ABI   Qt: $QT_VERSION"
  echo "  SDK:    compile/target $ANDROID_API_LEVEL, min $ANDROID_MIN_SDK"
  echo "  package: $PACKAGE_NAME   version: $VERSION_NAME ($VERSION_CODE)"
  echo ""
  echo "  An earlier side-load of the same package must be uninstalled first if"
  echo "  it has a higher versionCode or was signed with a different key."
  echo "======================================"
  exit 0
fi

########################################
# 8b. Multi-ABI App Bundle path (PACKAGE_FORMAT=aab)
########################################

log "Bundling ABIs: $ABI_CSV"
log "Running Gradle bundleRelease…"
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

check_16k_alignment "$FINAL_AAB"

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
  UNIVERSAL_APK_FILE="$SCRIPT_DIR/PokerTH-${VERSION_NAME}-${VERSION_CODE}-universal.apk"
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
  unzip -p "$APKS_FILE" universal.apk > "$UNIVERSAL_APK_FILE"
  rm -f "$APKS_FILE"

  # The only 16 KB evidence this build can produce end to end: bundletool
  # applies the manifest's extractNativeLibs here, so this APK is packed the way
  # the split APKs Play generates from the same bundle will be. On the .aab
  # itself only the flag can be reported — every entry in a bundle is deflated.
  check_16k_alignment "$UNIVERSAL_APK_FILE"

  # Which certificate ended up on the APK decides whether a device that already
  # has PokerTH installed accepts it as an update. Without --ks bundletool does
  # not leave the APK unsigned — it silently falls back to its own debug key,
  # which installs fine on a clean device but is rejected as an update over any
  # previously side-loaded build. Print the certificate either way, so the two
  # cases are told apart from the log instead of from a tester's phone.
  BT_DIR="$ANDROID_SDK_ROOT/build-tools/$ANDROID_BUILD_TOOLS_VERSION"
  log "Universal APK signature:"
  if [ -n "$ANDROID_KEYSTORE" ]; then
    "$BT_DIR/apksigner" verify --print-certs "$UNIVERSAL_APK_FILE" |
      grep -E "Signer #1 certificate (DN|SHA-256)" || true
  else
    "$BT_DIR/apksigner" verify --print-certs "$UNIVERSAL_APK_FILE" |
      grep -E "Signer #1 certificate DN" || true
    echo ""
    echo "WARNING: no ANDROID_KEYSTORE set — bundletool signed the universal APK"
    echo "         with its DEBUG key. It installs on a device that has no"
    echo "         PokerTH yet, but every existing side-load install refuses it"
    echo "         as an update (INSTALL_FAILED_UPDATE_INCOMPATIBLE, shown as a"
    echo "         bare 'App not installed')."
  fi
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

# Native debug symbols ride along in BUNDLE-METADATA/ (no device downloads
# them). Play refuses them above 800 MB, so print what this build ships.
SYMBOL_BYTES="$(unzip -l "$FINAL_AAB" |
  awk '$NF ~ /BUNDLE-METADATA\/com\.android\.tools\.build\.debugsymbols/ { n += $1 } END { print n + 0 }')"
if [ "$SYMBOL_BYTES" -gt 0 ]; then
  echo ""
  printf 'Native debug symbols for Play: %d MB uncompressed (Play rejects above 800 MB)\n' \
    "$((SYMBOL_BYTES / 1048576))"
fi
if [ "$UNIVERSAL_APK" = "1" ]; then
  echo ""
  echo "Universal APK for side-load testing:"
  ls -lh "$SCRIPT_DIR/PokerTH-${VERSION_NAME}-${VERSION_CODE}-universal.apk"
fi
echo ""
echo "Store icon: $SCRIPT_DIR/pokerth-play-icon-512.png"
echo "======================================"
