#!/bin/bash
set -e

# Baut die PokerTH-Widget-APK (arm64-v8a) in einem Docker-Container.
#
# Das devcontainer-Dockerfile wird als Build-Umgebung verwendet – Qt, NDK
# und vcpkg sind bereits im Image enthalten. Die lokalen Quellen werden per
# Volume eingebunden, sodass kein Einsteigen in den Container nötig ist.
#
# Aufruf:
#   cd <projekt-root>
#   bash docker/android/build_android_widget_arm64_docker.sh
#   # oder ohne Image-Cache:
#   bash docker/android/build_android_widget_arm64_docker.sh --no-cache

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEVCONTAINER_DIR="$SCRIPT_DIR/.devcontainer"
IMAGE_NAME="pokerth-android-builder:arm64"
ARCH="arm64-v8a"
NO_CACHE="${1:-}"
BUILD_TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=== PokerTH Android APK – Docker-Build Widget (arm64-v8a) ==="
echo "Projekt-Root:  $PROJECT_ROOT"
echo "Docker-Image:  $IMAGE_NAME"
echo ""

# Branch-Prüfung
CURRENT_BRANCH=$(git -C "$PROJECT_ROOT" rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unbekannt")
if [ "$CURRENT_BRANCH" != "qt6-qml" ]; then
    echo "WARNUNG: Aktueller Branch ist '$CURRENT_BRANCH', erwartet 'qt6-qml'."
    echo "Bitte erst: git checkout qt6-qml"
    read -r -p "Trotzdem fortfahren? [j/N] " REPLY
    [[ "$REPLY" =~ ^[jJyY]$ ]] || exit 1
fi
echo "Branch: $CURRENT_BRANCH  (wird via Volume in den Container eingebunden)"
echo ""

# Laufende/gestoppte Container dieses Images bereinigen
RUNNING=$(docker ps -q --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
if [ -n "$RUNNING" ]; then
    echo "=== Stoppe laufende Container ==="
    docker stop $RUNNING
    docker rm $RUNNING 2>/dev/null || true
fi
STOPPED=$(docker ps -aq --filter "ancestor=$IMAGE_NAME" 2>/dev/null)
[ -n "$STOPPED" ] && docker rm $STOPPED 2>/dev/null || true

# Docker-Image bauen
echo "=== Baue Docker-Image ==="
echo "    Erster Aufruf: Qt, NDK und vcpkg werden installiert – ca. 1 Stunde."
echo "    Folgeaufrufe starten dank Cache in Sekunden."
echo ""
docker build \
    ${NO_CACHE:+--no-cache} \
    -f "$DEVCONTAINER_DIR/Dockerfile" \
    -t "$IMAGE_NAME" \
    "$DEVCONTAINER_DIR"

# PokerTH Widget-Client im Container bauen – lokale Quellen via Volume eingebunden
echo ""
echo "=== Starte PokerTH Android Widget Build ==="
echo "    Lokale Quellen: $PROJECT_ROOT"
echo "    Container-Pfad: /opt/pokerth-android/pokerth"
echo "    Build-Target:   pokerth_client"
echo ""
docker run --rm \
    -e TARGET=pokerth_client \
    -v "$PROJECT_ROOT:/opt/pokerth-android/pokerth" \
    -w /opt/pokerth-android/pokerth \
    "$IMAGE_NAME" \
    bash docker/android/build_android.sh

# APK in docker/android/ kopieren
echo ""
echo "=== Suche und kopiere APK ==="
APK_SEARCH_DIR="$PROJECT_ROOT/build-android-${ARCH}/android-build/build/outputs/apk"
APK_FILE=$(find "$APK_SEARCH_DIR" -type f -name "*.apk" ! -name "*unaligned*" 2>/dev/null | head -1)

if [ -z "$APK_FILE" ]; then
    APK_FILE=$(find "$APK_SEARCH_DIR" -type f -name "*.apk" 2>/dev/null | head -1)
fi

if [ -z "$APK_FILE" ]; then
    echo "FEHLER: Keine APK in $APK_SEARCH_DIR gefunden!"
    exit 1
fi

DEST_APK="$SCRIPT_DIR/pokerth-widget_${ARCH}_${BUILD_TIMESTAMP}.apk"
cp -v "$APK_FILE" "$DEST_APK"

echo ""
echo "=== Fertig! ==="
ls -lh "$DEST_APK"
echo ""
echo "Nächster Schritt – APK signieren (außerhalb Docker):"
echo "  cd docker/android/"
echo "  apksigner sign --ks my.keystore --ks-key-alias app $(basename "$DEST_APK")"
