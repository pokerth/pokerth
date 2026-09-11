#!/usr/bin/env bash
#
# PokerTH QML client – build the card previews from the SVGs (NO client screenshot).
#
#   * card deck     (data/gfx/qml/cards/<name>):    two cards as held in the
#                                                   hand – slightly overlapping and
#                                                   angled against each other.
#   * card back   (data/gfx/qml/backside/<name>): simply the backside SVG.
#
# For both categories there is deliberately ONLY a landscape preview image
# (preview.png) – no portrait one.
#
# Rasterizing is done with rsvg-convert (package librsvg2-bin), compositing with
# ImageMagick (magick or convert). ImageMagick must not rasterize the SVGs
# itself: its internal MSVG renderer ignores linearGradient and paints the
# cards black.
#
# Usage:  preview/build_card_previews.sh
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QML="$ROOT/data/gfx/qml"
MAGICK="$(command -v magick || command -v convert || true)"
if [ -z "$MAGICK" ]; then
    echo "FEHLER: ImageMagick (magick/convert) nicht gefunden." >&2
    exit 1
fi
RSVG="$(command -v rsvg-convert || true)"
if [ -z "$RSVG" ]; then
    echo "FEHLER: rsvg-convert nicht gefunden (apt install librsvg2-bin)." >&2
    exit 1
fi

# Rasterize an SVG into a transparent PNG that fits into $2 x $3 while keeping
# the aspect ratio. $1=SVG, $4=output PNG.
rasterize() {
    "$RSVG" --keep-aspect-ratio -w "$2" -h "$3" "$1" -o "$4"
}

# Soft drop shadow behind a (transparent) card graphic. Sets the cards apart
# from the light background and separates overlapping cards from each other
# (the front one casts a shadow onto the back one). $1=input PNG, $2=output PNG.
SHADOW="55x8+3+8"   # opacity x blur + offset(x,y)
drop_shadow() {
    "$MAGICK" "$1" \( +clone -background black -shadow "$SHADOW" \) \
        +swap -background none -layers merge +repage "$2"
}

# Card deck: two cards in the hand – "ace-king". K♥ (engine index 24) at the back,
# A♠ (38) in front – one red and one black card.
build_deck() {
    local dir="$1"
    local back="$dir/24.svg" front="$dir/38.svg"
    if [ ! -f "$back" ] || [ ! -f "$front" ]; then
        echo "  übersprungen (Karten fehlen): $dir"; return
    fi
    local tmp; tmp="$(mktemp -d)"
    rasterize "$back"  300 420 "$tmp/b1.png"
    rasterize "$front" 300 420 "$tmp/f1.png"
    "$MAGICK" -background none "$tmp/b1.png" -rotate -13 +repage "$tmp/b0.png"
    "$MAGICK" -background none "$tmp/f1.png" -rotate  13 +repage "$tmp/f0.png"
    drop_shadow "$tmp/b0.png" "$tmp/b.png"
    drop_shadow "$tmp/f0.png" "$tmp/f.png"
    "$MAGICK" -size 820x560 xc:none \
        "$tmp/b.png" -gravity NorthWest -geometry +70+25 -composite \
        "$tmp/f.png" -gravity NorthWest -geometry +330+15 -composite \
        -trim +repage -bordercolor none -border 24 "$dir/preview.png"
    rm -rf "$tmp"
    echo "  -> $dir/preview.png"
}

# Card back: a single card with a drop shadow, centred on a transparent canvas
# of the same size for all styles – the style selection shows the previews
# next to each other, differently sized images look skewed there.
BACK_CANVAS="424x561"
build_back() {
    local dir="$1"
    local svg="$dir/backside.svg"
    if [ ! -f "$svg" ]; then
        echo "  übersprungen (backside.svg fehlt): $dir"; return
    fi
    local tmp; tmp="$(mktemp -d)"
    rasterize "$svg" 360 504 "$tmp/bk.png"
    drop_shadow "$tmp/bk.png" "$tmp/bks.png"
    "$MAGICK" "$tmp/bks.png" -background none -gravity center \
        -extent "$BACK_CANVAS" "$dir/preview.png"
    rm -rf "$tmp"
    echo "  -> $dir/preview.png"
}

echo "Kartenstapel-Vorschauen:"
for d in "$QML"/cards/*/; do [ -d "$d" ] && build_deck "$d"; done
echo "Kartenrückseiten-Vorschauen:"
for d in "$QML"/backside/*/; do [ -d "$d" ] && build_back "$d"; done
echo "Fertig."
