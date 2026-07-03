#!/usr/bin/env python3
"""
Generate 52 playing card SVGs as pixel-accurate reproductions of the
default_800x480 PNG cards (48×76 → viewBox 120×190, scale=2.5).

Strategy: trace every ink pixel from the source PNG as a coloured SVG rect.
This gives an exact 1:1 match to the original bitmap design.
"""

import os, zlib, struct

PNG_DIR    = "data/gfx/cards/default_800x480"
OUTPUT_DIR = "src/gui/qt6-qml/resources/cards"
SCALE      = 2.5   # 48×76 PNG → 120×190 SVG


# ── PNG decoder ───────────────────────────────────────────────────────────────
def _read_png(path):
    with open(path, 'rb') as f:
        data = f.read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    pos = 8; chunks = {}
    while pos < len(data):
        length = struct.unpack('>I', data[pos:pos+4])[0]
        ctype  = data[pos+4:pos+8].decode()
        chunks.setdefault(ctype, []).append(data[pos+8:pos+8+length])
        pos += 12 + length
    ihdr = chunks['IHDR'][0]
    w, h = struct.unpack('>II', ihdr[:8])
    raw  = zlib.decompress(b''.join(chunks['IDAT']))
    bpp  = 4; stride = w * bpp
    rows = []; prev = bytes(stride); i = 0
    for y in range(h):
        filt = raw[i]; i += 1
        row  = bytearray(raw[i:i+stride]); i += stride
        if filt == 1:
            for x in range(bpp, len(row)): row[x] = (row[x] + row[x-bpp]) & 0xff
        elif filt == 2:
            for x in range(len(row)):     row[x] = (row[x] + prev[x]) & 0xff
        elif filt == 3:
            for x in range(len(row)):
                a = row[x-bpp] if x >= bpp else 0
                row[x] = (row[x] + (a + prev[x]) // 2) & 0xff
        elif filt == 4:
            def paeth(a, b, c):
                p = a+b-c; pa = abs(p-a); pb = abs(p-b); pc = abs(p-c)
                return a if pa <= pb and pa <= pc else (b if pb <= pc else c)
            for x in range(len(row)):
                a = row[x-bpp] if x >= bpp else 0
                b = prev[x]; c = prev[x-bpp] if x >= bpp else 0
                row[x] = (row[x] + paeth(a, b, c)) & 0xff
        prev = bytes(row)
        rows.append([(row[x*4], row[x*4+1], row[x*4+2], row[x*4+3])
                     for x in range(w)])
    return rows, w, h


def card_to_svg_rects(card_idx, target_color):
    """
    Read the PNG for card_idx, extract ink pixels, and return a list of
    SVG <rect> strings using target_color.

    Ink is separated from the #F0F0F0 background (rgb 240,240,240) by computing:
      • red cards  (idx < 26): ink_alpha = 1 − g/240   (green ≈0 for red ink)
      • black cards (idx ≥ 26): ink_alpha = 1 − r/240   (all channels ≈0 for black ink)
    """
    rows, w, h = _read_png(f"{PNG_DIR}/{card_idx}.png")
    is_red = (card_idx < 26)
    BG = 240.0
    rects = []

    for y, row in enumerate(rows):
        x = 0
        while x < w:
            r, g, b, a = row[x]
            if a < 200:          # transparent or edge anti-alias → skip
                x += 1; continue
            ink = (1.0 - g / BG) if is_red else (1.0 - r / BG)
            if ink < 0.04:       # background pixel → skip
                x += 1; continue

            # Extend run while colour stays within ±0.05 ink-alpha
            run = 1
            while x + run < w:
                nr, ng, nb, na = row[x + run]
                if na < 200: break
                ni = (1.0 - ng / BG) if is_red else (1.0 - nr / BG)
                if abs(ni - ink) > 0.05 or ni < 0.04: break
                run += 1

            sx  = round(x   * SCALE, 1)
            sy  = round(y   * SCALE, 1)
            sw  = round(run * SCALE, 1)
            sh  = SCALE
            ink = round(ink, 2)

            if ink >= 0.95:
                rects.append(
                    f'  <rect fill="{target_color}" x="{sx}" y="{sy}"'
                    f' width="{sw}" height="{sh}"/>')
            else:
                rects.append(
                    f'  <rect fill="{target_color}" x="{sx}" y="{sy}"'
                    f' width="{sw}" height="{sh}" opacity="{ink}"/>')
            x += run

    return rects


# ── Deck definition ───────────────────────────────────────────────────────────
# 0-12 = Diamonds (#FF0000), 13-25 = Hearts (#FF0000),
# 26-38 = Spades (#000000),  39-51 = Clubs  (#000000)
SUITS = [
    ("diamonds", "#FF0000"),
    ("hearts",   "#FF0000"),
    ("spades",   "#000000"),
    ("clubs",    "#000000"),
]
RANKS = ["2","3","4","5","6","7","8","9","10","J","Q","K","A"]


def generate_svg(card_idx, color):
    lines = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 120 190">',
             '  <rect fill="#F0F0F0" x="0" y="0" width="120" height="190" rx="6"/>']
    lines += card_to_svg_rects(card_idx, color)
    lines.append('</svg>')
    return "\n".join(lines)


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    for suit_idx, (suit_name, color) in enumerate(SUITS):
        for rank_idx, rank in enumerate(RANKS):
            card_idx = suit_idx * 13 + rank_idx
            svg = generate_svg(card_idx, color)
            out = os.path.join(OUTPUT_DIR, f"{card_idx}.svg")
            with open(out, 'w') as f:
                f.write(svg)
    print(f"Generated 52 card SVGs → {OUTPUT_DIR}/")


if __name__ == "__main__":
    main()
