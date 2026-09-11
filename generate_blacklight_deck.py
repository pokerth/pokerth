#!/usr/bin/env python3
"""Blacklight-4c-Deck aus dem default4c-Deck erzeugen.

Geometrie (Rangglyphen + Farbsymbole samt Transformationen) wird 1:1 aus
data/gfx/qml/cards/default4c uebernommen – damit bleiben Ranghoehen und Layout
exakt die des Vorbilds. Neu sind nur die Farben (invertiert -> Schwarzlicht)
und der Glow: QtSvg kennt keine Filter, das Leuchten entsteht deshalb aus
mehreren immer breiteren, immer schwaecheren Konturen unter der Fuellung.
"""
import re, os

SRC = "/opt/pokerth_env/repos/pokerth-test/data/gfx/qml/cards/default4c"
DST = "/opt/pokerth_env/repos/pokerth-test/data/gfx/qml/cards/blacklight_4c"

# Suit = idx//13: 0 diamonds, 1 hearts, 2 spades, 3 clubs.
# Blacklight = inverted default4c colours, pulled towards neon:
#   diamonds blue -> amber, hearts red -> cyan, spades black -> white,
#   clubs green -> magenta.
SUITS = [
    dict(name="Karo",  ink="#FFC400", rim="#FFC400"),
    dict(name="Herz",  ink="#00EFFF", rim="#00EFFF"),
    dict(name="Pik",   ink="#F2F0FF", rim="#C9C6FF"),
    dict(name="Kreuz", ink="#FF4DE8", rim="#FF4DE8"),
]

# Glow layers: (width in card coordinates, opacity). From the outside in,
# with the full fill on top.
GLOW = [(10.6, 0.038), (8.2, 0.05), (6.2, 0.07), (4.4, 0.095),
        (3.0, 0.13), (1.9, 0.18), (1.0, 0.28)]

# The rank glyph moves up by 7 units compared to default4c. There the rank
# (ink y 26..94) and the suit symbol (y 97..149) are only 3 units apart -
# close enough for the glow to close the gap. With the offset it is 10 units,
# and the set has 19 units of margin at the top as well as at the bottom.
RANK_LIFT = 7.0

PATH_RE = re.compile(
    r'<path\s+d="([^"]+)"\s+transform="matrix\(([\d.\- ]+)\)"', re.S)


def glow_group(d, transform, scale, ink):
    """Konturen-Stapel + Fuellung fuer einen Pfad."""
    out = []
    for w, op in GLOW:
        out.append(
            '    <path d="%s" transform="%s" fill="none" stroke="%s" '
            'stroke-width="%.3f" stroke-opacity="%.2f" stroke-linejoin="round" '
            'stroke-linecap="round"/>' % (d, transform, ink, w / scale, op))
    out.append('    <path d="%s" transform="%s" fill="%s"/>' % (d, transform, ink))
    return "\n".join(out)


def main():
    os.makedirs(DST, exist_ok=True)
    for idx in range(52):
        src = open(os.path.join(SRC, "%d.svg" % idx)).read()
        hits = PATH_RE.findall(src)
        assert len(hits) == 2, (idx, len(hits))
        suit = SUITS[idx // 13]
        parts = []
        for n, (d, m) in enumerate(hits):
            nums = [float(v) for v in m.split()]
            if n == 0:                       # Rank glyph: move up
                nums[5] -= RANK_LIFT
            scale = nums[0]
            matrix = " ".join("%g" % v for v in nums)
            parts.append(glow_group(d, "matrix(%s)" % matrix, scale, suit["ink"]))
        svg = '''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 120 168">
  <!-- Blacklight 4c: Geometrie aus dem default4c-Deck, Farben invertiert
       (%s). Das Leuchten ist aus Konturen gebaut - QtSvg kann keine Filter. -->
  <defs>
    <linearGradient id="face" x1="0" y1="0" x2="0.35" y2="1">
      <stop offset="0" stop-color="#100c20"/>
      <stop offset="0.55" stop-color="#0a0716"/>
      <stop offset="1" stop-color="#05040c"/>
    </linearGradient>
  </defs>
  <rect x="0" y="0" width="120" height="168" rx="8" fill="url(#face)"/>
  <!-- Kartenkante: schwacher Neonrand in der Farbe der Karte, damit sich die
       dunkle Karte vom dunklen Tisch abhebt. -->
  <rect x="1.1" y="1.1" width="117.8" height="165.8" rx="7" fill="none"
        stroke="%s" stroke-width="2.2" stroke-opacity="0.13"/>
  <rect x="1.1" y="1.1" width="117.8" height="165.8" rx="7" fill="none"
        stroke="%s" stroke-width="0.9" stroke-opacity="0.42"/>
  <g>
%s
  </g>
  <g>
%s
  </g>
</svg>
''' % (suit["name"], suit["rim"], suit["rim"], parts[0], parts[1])
        open(os.path.join(DST, "%d.svg" % idx), "w").write(svg)
    print("52 Karten geschrieben nach", DST)


main()
