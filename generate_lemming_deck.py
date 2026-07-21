#!/usr/bin/env python3
"""
Generate the "Lemming" QML card deck (52 SVGs, viewBox 0 0 120 168) into
data/gfx/qml/cards/lemming/.

An SVG remake of lemming's classic PokerTH deck LEMMING-CARDS-01
(https://www.pokerth.net/download/styles/cards/LEMMING-CARDS-01.zip): a
4-colour deck (Karo=blau, Herz=rot, Pik=oliv, Kreuz=grün) whose card face is a
single soft colour gradient — no frame — with one big suit symbol drawn as a
pure OUTLINE (no fill) in the lower half.

Deviation from the original (as requested): in the corner index the RANK is on
top and the small filled suit pip below it — the original has it the other way
round.

Built like the other themed QML decks (template: star_trek / stardust):
  * rank glyphs (2-10, J, Q, K, A) are outline PATHS from a font
    (Montserrat Bold, OFL) — see data-copyright.txt.
  * suit symbols are reused paths from the cards/default set.

Engine index -> card:  suit = idx//13 (0 Karo,1 Herz,2 Pik,3 Kreuz),
                       rank = idx%13  (0=2 .. 8=10, 9=J,10=Q,11=K,12=A)

The Montserrat TTF is taken from $MONTSERRAT_TTF if set, otherwise downloaded
once from Google Fonts into a local cache. Only the resulting SVGs are needed
at runtime — the font itself is a build-time dependency only.
"""
import os, math, urllib.request
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.transformPen import TransformPen

HERE     = os.path.dirname(os.path.abspath(__file__))
OUT_DIR  = os.path.join(HERE, "data/gfx/qml/cards/lemming")
FONT_URL = ("https://raw.githubusercontent.com/google/fonts/main/ofl/"
            "montserrat/Montserrat%5Bwght%5D.ttf")

def font_path():
    p = os.environ.get("MONTSERRAT_TTF")
    if p and os.path.exists(p):
        return p
    cache = os.path.join(HERE, ".cache", "Montserrat.ttf")
    if not os.path.exists(cache):
        os.makedirs(os.path.dirname(cache), exist_ok=True)
        print(f"Downloading Montserrat -> {cache}")
        urllib.request.urlretrieve(FONT_URL, cache)
    return cache

RANKS = ["2","3","4","5","6","7","8","9","10","J","Q","K","A"]
GIDS  = ["diamond", "heart", "spade", "club"]   # suit index -> gradient id / symbol

# ── suit symbol paths (reused from cards/default, normalised around ~29,29.5) ──
SUIT_PATH = {
 "diamond": "M28.5 0l20.112 29L28.5 58 8.388 29z",
 "heart":   "M16.49.2c7.12-.476 10.832 5.638 11.557 10.455.432.766.668.55.834-.042.35-6.186 5.86-11.24 10.96-10.55 8.3.108 13.35 9.393 11.93 17.25-1.73 9.6-6.875 15.38-10.77 21.333C36.586 44.81 30.656 53.716 28.74 58c0 0-6.027-9.222-13.87-20.067-5.243-7.252-9.44-15.11-9.83-21.875-.447-7.73 2.71-14.66 11.45-15.86z",
 "spade":   "M26.406 40.187c.015-1.98-1.56-2.017-1.542-.573-.133 7.257-6.31 10.83-10.8 9.982-6.92-1.307-9.128-8.13-9.06-12.148C5.204 25.046 17.774 15.9 29.114 0c8.31 14.35 22.03 25.623 22.84 36.717 1.08 14.75-17.71 20.25-20.394 3.06-.172-1.214-1.612-1.76-1.5-.063.408 4.71-.17 6.674 5.18 18.287H21.25c3.1-5.42 4.943-12.09 5.154-17.81z",
 "club":    "M20.976 57.14c2.88-5.774 4.976-11.7 5.348-18.036.342-2.33-1.074-2.37-1.288-.73C20.96 53.97-.727 49.744.02 35.08.622 23.182 12.44 20.624 18.6 24.513c2.435 1.434 2.296.575.968-.91C10.416 13.39 15.958.24 28.33 0c13.38.793 16.854 14.87 8.647 23.147-1.02.994-3.283 4.19.123 1.956 8.287-5.888 19.787.443 19.553 9.52-.434 16.74-20.84 17.987-25.203 4.007-.42-1.55-1.63-2.328-1.295.305.604 4.767 1.855 11.953 5.438 18.205H20.976z",
}

# ── suit colours: (gradient top, gradient bottom, ink) ────────────────────────
# Face gradient and ink sampled from the original PNGs of LEMMING-CARDS-01.
SUITS = {
 "diamond": ("#b0cbdc", "#3d80b8", "#0e4c7a"),
 "heart":   ("#d3a8a5", "#b63e41", "#7e1717"),
 "spade":   ("#cbcfa4", "#b6b441", "#7a7b18"),
 "club":    ("#a0cfa4", "#42b742", "#1c871e"),
}

# ── layout ────────────────────────────────────────────────────────────────────
INDEX_CX     = 26.0     # centre x of the corner index (rank + small pip)
LEFT_MIN     = 7.0      # min left edge of the rank (keeps "10" from clipping)
BASELINE_Y   = 44.0     # baseline of the corner rank
CAP_HEIGHT   = 36.0     # cap/figure height of every rank -> one common size
SMALL_PIP    = (26.0, 64.0, 0.52)    # cx, cy, scale of the pip under the rank
# big outline symbol: cx, cy, x-scale, y-scale — squat and wide, so it stays
# clear of the (large) corner index without losing presence.
BIG_SUIT     = (60.0, 118.0, 1.42, 1.10)
BIG_STROKE   = 5.0      # stroke width of the big outline symbol, in card units

def load_font():
    f = TTFont(font_path())
    if "fvar" in f:                 # Montserrat is variable -> instance the Bold
        instantiateVariableFont(f, {"wght": 700}, inplace=True)
    return f

def _glyph(font, gs, ch, x_off):
    gname = font.getBestCmap()[ord(ch)]
    pen = SVGPathPen(gs)
    gs[gname].draw(TransformPen(pen, (1, 0, 0, 1, x_off, 0)))
    bpen = BoundsPen(gs)
    gs[gname].draw(TransformPen(bpen, (1, 0, 0, 1, x_off, 0)))
    return pen.getCommands(), font["hmtx"][gname][0], bpen.bounds

def rank_glyph(font, rank):
    gs = font.getGlyphSet()
    parts, x = [], 0.0
    xmin, xmax = math.inf, -math.inf
    for ch in rank:
        d, adv, b = _glyph(font, gs, ch, x)
        parts.append(d)
        if b:
            xmin = min(xmin, b[0]); xmax = max(xmax, b[2])
        x += adv
    return " ".join(parts), xmin, xmax

def make_card(idx, font, scale):
    gid = GIDS[idx // 13]
    top, bot, ink = SUITS[gid]
    rank = RANKS[idx % 13]

    # constant scale (uniform rank height); centre single glyphs at INDEX_CX,
    # clamp wide glyphs like "10" to a fixed left margin instead of clipping.
    d, xmin, xmax = rank_glyph(font, rank)
    X = INDEX_CX - (xmin + xmax) / 2.0 * scale
    if xmin * scale + X < LEFT_MIN:
        X = LEFT_MIN - xmin * scale
    X = round(X, 2)

    scx, scy, ss = SMALL_PIP
    bcx, bcy, bsx, bsy = BIG_SUIT
    suit_d = SUIT_PATH[gid]
    # the symbol is scaled anisotropically -> compensate the stroke width with the
    # geometric mean, so the outline keeps an even weight all around.
    bsw = round(BIG_STROKE / math.sqrt(bsx * bsy), 2)

    p = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 120 168">',
         f'<defs><linearGradient id="g_{gid}" x1="0" y1="0" x2="0.18" y2="1">'
         f'<stop offset="0" stop-color="{top}"/>'
         f'<stop offset="0.5" stop-color="{top}" stop-opacity="0.85"/>'
         f'<stop offset="1" stop-color="{bot}"/></linearGradient></defs>',
         # card face: one soft colour gradient, rounded corners, faint suit-tinted rim
         f'<rect x="0" y="0" width="120" height="168" rx="9" fill="url(#g_{gid})"/>',
         f'<rect x="0.5" y="0.5" width="119" height="167" rx="8.5" fill="none" '
         f'stroke="{ink}" stroke-width="1" opacity="0.3"/>',
         # corner index: rank on top, small filled pip below it
         f'<path d="{d}" transform="translate({X} {BASELINE_Y}) '
         f'scale({scale} -{scale})" fill="{ink}"/>',
         f'<path d="{suit_d}" transform="translate({scx} {scy}) scale({ss}) '
         f'translate(-29.0 -29.5)" fill="{ink}"/>',
         # big suit symbol: outline only, no fill
         f'<path d="{suit_d}" transform="translate({bcx} {bcy}) scale({bsx} {bsy}) '
         f'translate(-29.0 -29.5)" fill="none" stroke="{ink}" '
         f'stroke-width="{bsw}" stroke-linejoin="round"/>',
         '</svg>']
    return "".join(p)

def main():
    font = load_font()
    # cap height of the figures -> one constant glyph scale for every rank
    scale = CAP_HEIGHT / font["OS/2"].sCapHeight
    os.makedirs(OUT_DIR, exist_ok=True)
    for idx in range(52):
        with open(os.path.join(OUT_DIR, f"{idx}.svg"), "w") as fh:
            fh.write(make_card(idx, font, scale))
    print(f"Generated 52 card SVGs -> {OUT_DIR}/")

if __name__ == "__main__":
    main()
