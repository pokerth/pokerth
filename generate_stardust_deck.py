#!/usr/bin/env python3
"""
Generate the "Stardust" QML card decks (52 SVGs each, viewBox 0 0 120 168)
into data/gfx/qml/cards/stardust_dark/ and .../stardust_light/.

Built exactly like the other themed QML decks (template: star_trek):
  * rank glyphs (2-10, J, Q, K, A) are outline PATHS from a font
    (Orbitron, OFL) — a squared sci-fi typeface that reproduces the
    Eurostile-like look of the classic PokerTH "Stardust" deck.
  * suit symbols are reused paths from the cards/default set.

Two 1:1 variants sharing one layout and one card back (backside/stardust/):
  * dark  — dark "space" card face (deep-blue gradient + starfield)
  * light — light card face (soft white gradient + faint starfield)
Both are 4-colour decks like nobus_4c_classic (Karo/Herz/Pik/Kreuz), with the
rank glyph, pips and bevel frame all in the suit colour, lifted off the ground
by a thin contour and/or a soft drop shadow. Pure paths, no SVG filters -> QtSvg.

Engine index -> card:  suit = idx//13 (0 Karo,1 Herz,2 Pik,3 Kreuz),
                       rank = idx%13  (0=2 .. 8=10, 9=J,10=Q,11=K,12=A)

The Orbitron TTF is taken from $ORBITRON_TTF if set, otherwise downloaded once
from Google Fonts into a local cache. Only the resulting SVGs are needed at
runtime — the font itself is a build-time dependency only.
"""
import os, math, random, urllib.request
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.transformPen import TransformPen

HERE      = os.path.dirname(os.path.abspath(__file__))
CARDS_DIR = os.path.join(HERE, "data/gfx/qml/cards")
FONT_URL  = ("https://raw.githubusercontent.com/google/fonts/main/ofl/"
             "orbitron/Orbitron%5Bwght%5D.ttf")

def font_path():
    p = os.environ.get("ORBITRON_TTF")
    if p and os.path.exists(p):
        return p
    cache = os.path.join(HERE, ".cache", "Orbitron.ttf")
    if not os.path.exists(cache):
        os.makedirs(os.path.dirname(cache), exist_ok=True)
        print(f"Downloading Orbitron -> {cache}")
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

# ── layout (shared by both variants) ──────────────────────────────────────────
BASELINE_Y  = 49.0              # baseline of the corner rank
SMALL_PIP_Y = 72.0             # centre y of the small pip under the rank
INDEX_CX    = 31.0             # centre x of single-glyph rank + the small pip
LEFT_MIN    = 10.0             # min left edge of the rank (keeps "10" from clipping)
RANK_SCALE  = 0.052            # CONSTANT for every rank -> all ranks share one height

# ── the two themes ────────────────────────────────────────────────────────────
# 4-colour decks like nobus_4c_classic (Karo=blue, Herz=red, Pik=black, Kreuz=
# green). "dark" brightens the inks for the dark face; "light" uses the classic
# saturated inks (Pik pure black) on the light face. Each suit: (ink, accent),
# accent = frame outer glow (+ glyph contour when the theme uses one).
THEMES = {
 "stardust_dark": {
   "face_top": "#12234f", "face_bot": "#050a1e", "bg_stroke": "#0a1330",
   "glow_w": 3, "glow_op": 0.22,
   "dark_edge": "#04070f", "dark_edge_op": 0.55,
   "frame_w": 2.2,
   "light_edge": "#ffffff", "light_edge_w": 0.8, "light_edge_op": 0.45,
   "shadow": "#01030c", "shadow_op": 0.55,
   "star": "#dff6ff", "star_lo": 0.35, "star_hi": 0.9,
   "contour": True,
   "suits": [("#45b4ff", "#bfe6ff"), ("#ff3b57", "#ffd0d8"),
             ("#eef5ff", "#a9d8ff"), ("#2ec46e", "#bdf1d2")],
 },
 "stardust_light": {
   "face_top": "#ffffff", "face_bot": "#dbe3f0", "bg_stroke": "#aeb9cf",
   "glow_w": 3, "glow_op": 0.28,
   "dark_edge": "#8b98b2", "dark_edge_op": 0.5,
   "frame_w": 2.2,
   "light_edge": "#ffffff", "light_edge_w": 0.8, "light_edge_op": 0.7,
   "shadow": "#9aa7be", "shadow_op": 0.55,
   "star": "#a9b7d0", "star_lo": 0.25, "star_hi": 0.55,
   "contour": False,
   "suits": [("#1565d8", "#8fb8ee"), ("#d81f33", "#f0a0ab"),
             ("#14171d", "#9aa4b3"), ("#0f8a3e", "#8fd6a8")],
 },
}

def load_font():
    f = TTFont(font_path())
    if "fvar" in f:             # Orbitron is variable -> instance a semi-bold
        instantiateVariableFont(f, {"wght": 600}, inplace=True)
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
    xmin = math.inf; xmax = -math.inf
    for ch in rank:
        d, adv, b = _glyph(font, gs, ch, x)
        parts.append(d)
        if b:
            xmin = min(xmin, b[0]); xmax = max(xmax, b[2])
        x += adv
    return " ".join(parts), xmin, xmax

def stars(seed, col, lo, hi):
    rnd = random.Random(seed)
    out = []
    for _ in range(14):
        cx = round(rnd.uniform(9, 111), 1); cy = round(rnd.uniform(9, 159), 1)
        r  = round(rnd.uniform(0.4, 1.1), 2); op = round(rnd.uniform(lo, hi), 2)
        out.append(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{col}" opacity="{op}"/>')
    return "".join(out)

def inked(d, transform, ink, contour, sw, shadow, shadow_op):
    """Shape with a soft drop shadow, plus (optionally) a thin crisp contour on
    the solid fill, so it lifts off the background without looking blurry."""
    s = (f'<path d="{d}" transform="translate(0.8 1.1) {transform}" '
         f'fill="{shadow}" opacity="{shadow_op}"/>')
    if contour:
        s += (f'<path d="{d}" transform="{transform}" fill="{ink}" '
              f'stroke="{contour}" stroke-width="{sw}" stroke-linejoin="round"/>')
    else:
        s += f'<path d="{d}" transform="{transform}" fill="{ink}"/>'
    return s

def make_card(idx, font, T):
    gid = GIDS[idx // 13]
    ink, accent = T["suits"][idx // 13]
    contour = accent if T["contour"] else None
    rank = RANKS[idx % 13]

    # constant scale (uniform rank height); centre single glyphs at INDEX_CX,
    # clamp wide glyphs like "10" to a fixed left margin instead of clipping.
    d, xmin, xmax = rank_glyph(font, rank)
    s = RANK_SCALE
    X = INDEX_CX - (xmin + xmax) / 2.0 * s
    if xmin * s + X < LEFT_MIN:
        X = LEFT_MIN - xmin * s
    X = round(X, 2)
    rank_tf  = f"translate({X} {BASELINE_Y}) scale({s} -{s})"
    small_tf = f"translate({INDEX_CX:.2f} {SMALL_PIP_Y}) scale(0.4237) translate(-29.0 -29.5)"
    big_tf   = "translate(81.00 125.0) scale(0.9153) translate(-29.0 -29.5)"
    suit_d = SUIT_PATH[gid]
    sh, sh_op = T["shadow"], T["shadow_op"]

    p = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 120 168">',
         f'<defs><linearGradient id="g_{gid}" x1="0" y1="0" x2="0" y2="1">'
         f'<stop offset="0" stop-color="{T["face_top"]}"/>'
         f'<stop offset="1" stop-color="{T["face_bot"]}"/></linearGradient></defs>',
         f'<rect x="0.75" y="0.75" width="118.5" height="166.5" rx="9" '
         f'fill="url(#g_{gid})" stroke="{T["bg_stroke"]}" stroke-width="1.5"/>',
         f'<g>{stars(idx * 97 + 13, T["star"], T["star_lo"], T["star_hi"])}</g>',
         # bevel frame in the suit colour: outer glow, dark shadow edge,
         # solid suit-coloured frame, light inner edge
         f'<rect x="4.6" y="4.6" width="110.8" height="158.8" rx="6" fill="none" '
         f'stroke="{accent}" stroke-width="{T["glow_w"]}" opacity="{T["glow_op"]}"/>',
         f'<rect x="4.6" y="4.6" width="110.8" height="158.8" rx="6" fill="none" '
         f'stroke="{T["dark_edge"]}" stroke-width="1" opacity="{T["dark_edge_op"]}" '
         f'transform="translate(0 1.5)"/>',
         f'<rect x="4.6" y="4.6" width="110.8" height="158.8" rx="6" fill="none" '
         f'stroke="{ink}" stroke-width="{T["frame_w"]}"/>',
         f'<rect x="4.6" y="4.6" width="110.8" height="158.8" rx="6" fill="none" '
         f'stroke="{T["light_edge"]}" stroke-width="{T["light_edge_w"]}" '
         f'opacity="{T["light_edge_op"]}" transform="translate(0 -1.1)"/>',
         # rank + pips
         inked(d, rank_tf, ink, contour, round(0.45 / s, 2), sh, sh_op),
         inked(suit_d, small_tf, ink, contour, round(0.45 / 0.4237, 2), sh, sh_op),
         inked(suit_d, big_tf, ink, contour, round(0.45 / 0.9153, 2), sh, sh_op),
         '</svg>']
    return "".join(p)

def main():
    font = load_font()
    for name, T in THEMES.items():
        out = os.path.join(CARDS_DIR, name)
        os.makedirs(out, exist_ok=True)
        for idx in range(52):
            with open(os.path.join(out, f"{idx}.svg"), "w") as fh:
                fh.write(make_card(idx, font, T))
        print(f"Generated 52 card SVGs -> {out}/")

if __name__ == "__main__":
    main()
