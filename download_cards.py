#!/usr/bin/env python3
"""Download SVGs from MattCain/svg-playing-cards and map to PokerTH card indices."""

import urllib.request
import os

BASE = "https://raw.githubusercontent.com/MattCain/svg-playing-cards/master/imgs"
OUT  = "src/gui/qt6-qml/resources/cards"

RANKS = ["2", "3", "4", "5", "6", "7", "8", "9", "10",
         "jack", "queen", "king", "ace"]
# PokerTH: 0-12=Diamonds, 13-25=Hearts, 26-38=Spades, 39-51=Clubs
SUITS = ["diamonds", "hearts", "spades", "clubs"]

os.makedirs(OUT, exist_ok=True)

for suit_idx, suit in enumerate(SUITS):
    for rank_idx, rank in enumerate(RANKS):
        card_idx = suit_idx * 13 + rank_idx
        fname    = f"{rank}_of_{suit}.svg"
        url      = f"{BASE}/{fname}"
        dest     = os.path.join(OUT, f"{card_idx}.svg")
        print(f"  {card_idx:2d}  {fname}  →  {dest}")
        try:
            urllib.request.urlretrieve(url, dest)
        except Exception as e:
            print(f"     ERROR: {e}")

# Card back → cardBackground.svg
back_url  = f"{BASE}/back.svg"
back_dest = "src/gui/qt6-qml/resources/cardBackground.svg"
print(f"\n  back.svg  →  {back_dest}")
try:
    urllib.request.urlretrieve(back_url, back_dest)
    print("  Done.")
except Exception as e:
    print(f"  ERROR: {e}")

print("\nAll done.")
