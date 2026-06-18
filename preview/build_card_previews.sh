#!/usr/bin/env bash
#
# PokerTH QML-Client – Karten-Vorschauen aus den SVGs bauen (KEIN Client-Screenshot).
#
#   * Kartenstapel  (data/gfx/qml/cards/<name>):    zwei Karten wie in der Hand
#                                                   gehalten – leicht überlappt und
#                                                   gegeneinander gewinkelt.
#   * Kartenrückseite (data/gfx/qml/backside/<name>): einfach die Rückseiten-SVG.
#
# Für beide Kategorien gibt es bewusst NUR ein Querformat-Vorschaubild
# (preview.png) – kein Portrait. Benötigt ImageMagick (magick bzw. convert).
#
# Aufruf:  preview/build_card_previews.sh
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QML="$ROOT/data/gfx/qml"
MAGICK="$(command -v magick || command -v convert || true)"
if [ -z "$MAGICK" ]; then
    echo "FEHLER: ImageMagick (magick/convert) nicht gefunden." >&2
    exit 1
fi

# Kartenstapel: zwei Karten in der Hand – "Ace-King". K♥ (Engine-Index 24) hinten,
# A♠ (38) vorne – eine rote und eine schwarze Karte.
build_deck() {
    local dir="$1"
    local back="$dir/24.svg" front="$dir/38.svg"
    if [ ! -f "$back" ] || [ ! -f "$front" ]; then
        echo "  übersprungen (Karten fehlen): $dir"; return
    fi
    local tmp; tmp="$(mktemp -d)"
    "$MAGICK" -background none "$back"  -resize 300x420 -rotate -13 +repage "$tmp/b.png"
    "$MAGICK" -background none "$front" -resize 300x420 -rotate  13 +repage "$tmp/f.png"
    "$MAGICK" -size 760x500 xc:none \
        "$tmp/b.png" -gravity NorthWest -geometry +70+25 -composite \
        "$tmp/f.png" -gravity NorthWest -geometry +330+15 -composite \
        -trim +repage -bordercolor none -border 24 "$dir/preview.png"
    rm -rf "$tmp"
    echo "  -> $dir/preview.png"
}

# Kartenrückseite: nur die SVG, sauber zugeschnitten mit etwas Rand.
build_back() {
    local dir="$1"
    local svg="$dir/backside.svg"
    if [ ! -f "$svg" ]; then
        echo "  übersprungen (backside.svg fehlt): $dir"; return
    fi
    "$MAGICK" -background none "$svg" -resize 360x504 -trim +repage \
        -bordercolor none -border 16 "$dir/preview.png"
    echo "  -> $dir/preview.png"
}

echo "Kartenstapel-Vorschauen:"
for d in "$QML"/cards/*/; do [ -d "$d" ] && build_deck "$d"; done
echo "Kartenrückseiten-Vorschauen:"
for d in "$QML"/backside/*/; do [ -d "$d" ] && build_back "$d"; done
echo "Fertig."
