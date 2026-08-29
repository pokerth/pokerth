#!/usr/bin/env python3
"""Kartenrückseite "Disco": Spiegelkugel im Schwarzlicht.

Schreibt data/gfx/qml/backside/disco/backside.svg (viewBox 580x800 wie die
anderen Rückseiten). Die Kugel ist eine echte Facettenkugel: Breiten-/Längen-
gitter, orthographisch projiziert, Rückseite verworfen, jede Facette diffus
beleuchtet mit hartem Glanzpunkt. Ein Teil der Spiegel fängt die Klubfarben
(die Schwarzlichtfarben des blacklight_4c-Decks). Alles aus Pfaden und
Verläufen – QtSvg kennt weder Filter noch <use>-Sicherheiten.
"""
import math, random, os

OUT = "/opt/pokerth_env/repos/pokerth-test/data/gfx/qml/backside/disco"

CX, CY, R = 290.0, 400.0, 143.0
NLAT, NLON = 15, 30
LIGHT = (-0.42, 0.60, 0.68)

# Klubfarben = Palette des blacklight_4c-Decks (Magenta, Cyan, Bernstein,
# dazu Violett als Übergang).
CLUB = [(1.00, 0.30, 0.91), (0.00, 0.94, 1.00), (1.00, 0.77, 0.00),
        (0.62, 0.36, 1.00), (1.00, 0.45, 0.55)]
TINT = (0.80, 0.82, 0.95)          # Grundton der Spiegel: kühles Silber


def clamp(v, a=0.0, b=1.0):
    return max(a, min(b, v))


def hexc(r, g, b):
    return "#%02X%02X%02X" % (int(clamp(r) * 255), int(clamp(g) * 255), int(clamp(b) * 255))


def facets():
    random.seed(20260829)
    ln = math.sqrt(sum(c * c for c in LIGHT))
    L = tuple(c / ln for c in LIGHT)
    out = []
    for i in range(NLAT):
        lat0 = -math.pi / 2 + math.pi * i / NLAT
        lat1 = -math.pi / 2 + math.pi * (i + 1) / NLAT
        for j in range(NLON):
            lon0 = -math.pi + 2 * math.pi * j / NLON
            lon1 = -math.pi + 2 * math.pi * (j + 1) / NLON
            latm, lonm = (lat0 + lat1) / 2, (lon0 + lon1) / 2
            n = (math.cos(latm) * math.sin(lonm), math.sin(latm), math.cos(latm) * math.cos(lonm))
            if n[2] <= 0.06:                       # Rückseite / Silhouettenrand
                continue
            pts = []
            for (la, lo) in ((lat0, lon0), (lat0, lon1), (lat1, lon1), (lat1, lon0)):
                pts.append((CX + R * math.cos(la) * math.sin(lo), CY - R * math.sin(la)))
            mx = sum(p[0] for p in pts) / 4
            my = sum(p[1] for p in pts) / 4
            k = 0.87                                # dunkle Fugen zwischen den Spiegeln
            pts = [(mx + (x - mx) * k, my + (y - my) * k) for (x, y) in pts]
            d = max(0.0, sum(n[i2] * L[i2] for i2 in range(3)))
            spec = d ** 20
            base = random.choice(CLUB) if random.random() < 0.45 else TINT
            base = [base[i2] * 0.86 + 0.10 for i2 in range(3)]
            f = (0.08 + 1.12 * d) * (0.30 + 0.70 * n[2])
            f *= 0.78 + 0.44 * random.random()
            col = [clamp(base[i2] * f + spec) for i2 in range(3)]
            out.append((n[2], pts, hexc(*col)))
    out.sort(key=lambda t: t[0])
    return out


def sparkles():
    """Lichtpunkte, die die Kugel in den Raum wirft: kleine gedrehte Quadrate."""
    random.seed(4711)
    out = []
    for _ in range(150):
        x = random.uniform(52, 528)
        y = random.uniform(52, 748)
        if math.hypot(x - CX, y - CY) < R + 26:     # nicht auf der Kugel
            continue
        s = random.uniform(3.0, 9.5)
        rot = random.uniform(0, 90)
        col = hexc(*random.choice(CLUB))
        # nah an der Kugel hell, zum Rand hin ausfadend
        dist = math.hypot(x - CX, y - CY)
        op = (0.85 - 0.55 * min(1.0, (dist - R) / 300.0)) * random.uniform(0.45, 1.0)
        out.append('  <rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" fill="%s" '
                   'opacity="%.2f" transform="rotate(%.1f %.1f %.1f)"/>'
                   % (x, y, s, s, col, max(0.10, min(0.85, op)), rot, x + s / 2, y + s / 2))
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    body = "\n".join('    <path d="M%s Z" fill="%s"/>'
                     % (" L".join("%.1f,%.1f" % p for p in pts), col)
                     for _, pts, col in facets())
    svg = '''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 580 800">
  <!-- Disco: Spiegelkugel im Schwarzlicht. Facetten sind echte Pfade (Kugel-
       gitter, orthographisch projiziert), das Leuchten kommt aus Verläufen -
       QtSvg kann keine Filter. Erzeugt mit generate_disco_backside.py. -->
  <defs>
    <linearGradient id="edge" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#241a33"/><stop offset="1" stop-color="#0d0817"/>
    </linearGradient>
    <linearGradient id="deep" x1="0" y1="0" x2="0.3" y2="1">
      <stop offset="0" stop-color="#150e26"/><stop offset="0.55" stop-color="#0a0618"/>
      <stop offset="1" stop-color="#05030e"/>
    </linearGradient>
    <radialGradient id="halo" cx="0.5" cy="0.5" r="0.55">
      <stop offset="0" stop-color="#FF4DE8" stop-opacity="0.30"/>
      <stop offset="0.45" stop-color="#8A2BE2" stop-opacity="0.16"/>
      <stop offset="1" stop-color="#05030e" stop-opacity="0"/>
    </radialGradient>
    <radialGradient id="bloom" cx="0.5" cy="0.5" r="0.5">
      <stop offset="0.52" stop-color="#FF6BEE" stop-opacity="0.34"/>
      <stop offset="0.74" stop-color="#A64BFF" stop-opacity="0.14"/>
      <stop offset="1" stop-color="#05030e" stop-opacity="0"/>
    </radialGradient>
    <radialGradient id="vig" cx="0.5" cy="0.5" r="0.62">
      <stop offset="0.55" stop-color="#05030e" stop-opacity="0"/>
      <stop offset="1" stop-color="#000000" stop-opacity="0.55"/>
    </radialGradient>
  </defs>

  <rect x="9" y="9" width="562" height="782" rx="42" fill="url(#edge)" stroke="#3a2450" stroke-width="2"/>
  <rect x="40" y="40" width="500" height="720" rx="30" fill="url(#deep)"/>
  <rect x="40" y="40" width="500" height="720" rx="30" fill="url(#halo)"/>

  <!-- Lichtpunkte der Kugel -->
%s

  <!-- Spiegelkugel: erst der Lichthof, dann der Korpus -->
  <circle cx="%.0f" cy="%.0f" r="%.0f" fill="url(#bloom)"/>
  <circle cx="%.0f" cy="%.0f" r="%.1f" fill="#0b0714"/>
  <g>
%s
  </g>

  <rect x="40" y="40" width="500" height="720" rx="30" fill="url(#vig)"/>
  <rect x="40" y="40" width="500" height="720" rx="30" fill="none" stroke="#FF4DE8" stroke-width="14" opacity="0.12"/>
  <rect x="40" y="40" width="500" height="720" rx="30" fill="none" stroke="#FF4DE8" stroke-width="5" opacity="0.75"/>
  <rect x="40" y="40" width="500" height="720" rx="30" fill="none" stroke="#FFD9F6" stroke-width="1.3" opacity="0.6" transform="translate(0 -2)"/>
</svg>
''' % ("\n".join(sparkles()), CX, CY, R * 2.05, CX, CY, R + 1.5, body)
    open(os.path.join(OUT, "backside.svg"), "w").write(svg)
    print("geschrieben:", os.path.join(OUT, "backside.svg"))


main()
