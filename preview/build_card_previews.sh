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
# (preview.png) – kein Portrait.
#
# Gerastert wird mit rsvg-convert (Paket librsvg2-bin), zusammengesetzt mit
# ImageMagick (magick bzw. convert). ImageMagick darf die SVGs nicht selbst
# rastern: sein interner MSVG-Renderer ignoriert linearGradient und malt die
# Karten schwarz.
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
RSVG="$(command -v rsvg-convert || true)"
if [ -z "$RSVG" ]; then
    echo "FEHLER: rsvg-convert nicht gefunden (apt install librsvg2-bin)." >&2
    exit 1
fi

# SVG in ein transparentes PNG rastern, das unter Beibehaltung des Seiten-
# verhältnisses in $2 x $3 passt. $1=SVG, $4=Ausgabe-PNG.
rasterize() {
    "$RSVG" --keep-aspect-ratio -w "$2" -h "$3" "$1" -o "$4"
}

# Weicher Schlagschatten hinter einer (transparenten) Karten-Grafik. Hebt die
# Karten vom hellen Hintergrund ab und trennt überlappende Karten voneinander
# (die vordere wirft Schatten auf die hintere). $1=Eingabe-PNG, $2=Ausgabe-PNG.
SHADOW="55x8+3+8"   # Deckkraft x Weichzeichnung + Versatz(x,y)
drop_shadow() {
    "$MAGICK" "$1" \( +clone -background black -shadow "$SHADOW" \) \
        +swap -background none -layers merge +repage "$2"
}

# Kartenstapel: zwei Karten in der Hand – "Ace-King". K♥ (Engine-Index 24) hinten,
# A♠ (38) vorne – eine rote und eine schwarze Karte.
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

# Kartenrückseite: einzelne Karte mit Schlagschatten, mittig auf einer für alle
# Stile gleich großen, transparenten Leinwand – die Stil-Auswahl zeigt die
# Vorschauen nebeneinander, unterschiedlich große Bilder wirken dort schief.
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
