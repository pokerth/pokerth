.pragma library

// ─────────────────────────────────────────────────────────────────────────────
// Sitzplatz-Layout des Spieltischs: Box-Skala und Slot-Positionen.
//
// Reine Geometrie, ohne Zugriff auf QML-Objekte - deshalb liegt sie hier und
// nicht mehr in GamePage.qml. Alle Eingaben kommen im `env`-Objekt (siehe
// tableZone.layoutEnv): Zonenmaße, Sitzzahlen, die Basis-Maße der Boxen und
// die relevanten Config-Schalter.
//
// Die zentrale Eigenschaft: boxScale() sucht per Bisektion die GRÖSSTE Skala,
// bei der alle Boxen überlappungsfrei liegen. Die Machbarkeitsprobe bewertet
// dazu exakt die Slots, die bei dieser Skala auch gezeichnet würden - Probe
// und Zeichnen teilen sich buildLandscapeSlots() bzw. buildPortraitSlots().
// ─────────────────────────────────────────────────────────────────────────────
// Desktop-Hochformat: fest verdrahtete Slot-Brüche.
var slotPosPortraitFixed = ({
    "L_bottom": [0.15, 0.785],
    "L_lower":  [0.15, 0.65],
    "L_upper":  [0.15, 0.345],
    "TL":       [0.15, 0.21],
    "TC":       [0.50, 0.075],
    "TR":       [0.85, 0.21],
    "R_upper":  [0.85, 0.345],
    "R_lower":  [0.85, 0.65],
    "R_bottom": [0.85, 0.785],
    "BC":       [0.50, 0.90]
})

// Slot-Reihenfolge je nach Gegnerzahl M – symmetrisch links/rechts verteilt,
// damit unabhängig von der Spielerzahl Kreis-Symmetrie entsteht.
var slotSeqPortrait = ({
    1: ["TC"],
    2: ["TL", "TR"],
    3: ["TL", "TC", "TR"],
    4: ["L_upper", "TL", "TR", "R_upper"],
    5: ["L_upper", "TL", "TC", "TR", "R_upper"],
    6: ["L_lower", "L_upper", "TL", "TR", "R_upper", "R_lower"],
    7: ["L_lower", "L_upper", "TL", "TC", "TR", "R_upper", "R_lower"],
    8: ["L_bottom", "L_lower", "L_upper", "TL", "TR", "R_upper", "R_lower", "R_bottom"],
    9: ["L_bottom", "L_lower", "L_upper", "TL", "TC", "TR", "R_upper", "R_lower", "R_bottom"]
})

var slotSeqLandscape = (function() {
    var dict = {}
    for (var n = 1; n <= 9; n++) {
        var seq = []
        for (var i = 1; i <= n; i++) seq.push("opp" + i)
        dict[n] = seq
    }
    return dict
})()

// Hängt von der Orientierung ab (Slot-Namen unterscheiden sich), daher
// Funktion statt Tabelle.
function slotSeqSpectate(wide) {
    var base  = wide ? slotSeqLandscape : slotSeqPortrait
    var first = wide ? "opp0" : "BC"
    var dict  = {}
    dict[1] = [first]
    for (var n = 2; n <= 10; n++) {
        var rest = base[n - 1]
        if (rest) dict[n] = [first].concat(rest)
    }
    return dict
}

// Slot-Reihenfolge der aktuellen Verteilung.
function slotSeq(env) {
    return env.spectating ? slotSeqSpectate(env.wide)
                          : (env.wide ? slotSeqLandscape : slotSeqPortrait)
}

function portraitUpperSlot(name) {
    return name === "TC" || name === "TL" || name === "TR"
        || name === "L_upper" || name === "R_upper"
}

function portraitBandAt(env, s, pos, seq) {
    var width = env.width, height = env.height, wide = env.wide,
        spectating = env.spectating, seatCount = env.seatCount,
        ringCount = env.ringCount, _peakSeatCount = env.peakSeatCount,
        oppBaseWidth = env.oppBaseWidth, oppBaseHeight = env.oppBaseHeight,
        selfBaseWidth = env.selfBaseWidth, selfBaseHeight = env.selfBaseHeight,
        opponentGapBase = env.opponentGapBase,
        opponentHorizontalGapBase = env.opponentHorizontalGapBase,
        selfGapBase = env.selfGapBase, selfBadgeGapBase = env.selfBadgeGapBase,
        sideBadgeGapBase = env.sideBadgeGapBase,
        landscapeRowCount = env.landscapeRowCount,
        Config = env.Config
    var vH = oppBaseHeight * s
    var top = 4
    var bottom = spectating ? height - 4 : height - 4 - selfBaseHeight * s
    for (var i = 0; i < seq.length; ++i) {
        var p = pos[seq[i]]
        if (!p) continue
        var cy = p[1] * height
        if (portraitUpperSlot(seq[i])) {
            if (cy + vH / 2 > top) top = cy + vH / 2
        } else if (cy - vH / 2 < bottom) {
            bottom = cy - vH / 2
        }
    }
    return [top, bottom]
}

function portraitCommunityNeed(env, s) {
    var width = env.width, height = env.height, wide = env.wide,
        spectating = env.spectating, seatCount = env.seatCount,
        ringCount = env.ringCount, _peakSeatCount = env.peakSeatCount,
        oppBaseWidth = env.oppBaseWidth, oppBaseHeight = env.oppBaseHeight,
        selfBaseWidth = env.selfBaseWidth, selfBaseHeight = env.selfBaseHeight,
        opponentGapBase = env.opponentGapBase,
        opponentHorizontalGapBase = env.opponentHorizontalGapBase,
        selfGapBase = env.selfGapBase, selfBadgeGapBase = env.selfBadgeGapBase,
        sideBadgeGapBase = env.sideBadgeGapBase,
        landscapeRowCount = env.landscapeRowCount,
        Config = env.Config
    var cs = Math.max(0.55, Math.min(0.8 * s, 1.8, Math.max(0, width - 16) / 264))
    return 124 * cs + 20
}

function buildPortraitSlots(env, s, ringCnt) {
    var width = env.width, height = env.height, wide = env.wide,
        spectating = env.spectating, seatCount = env.seatCount,
        ringCount = env.ringCount, _peakSeatCount = env.peakSeatCount,
        oppBaseWidth = env.oppBaseWidth, oppBaseHeight = env.oppBaseHeight,
        selfBaseWidth = env.selfBaseWidth, selfBaseHeight = env.selfBaseHeight,
        opponentGapBase = env.opponentGapBase,
        opponentHorizontalGapBase = env.opponentHorizontalGapBase,
        selfGapBase = env.selfGapBase, selfBadgeGapBase = env.selfBadgeGapBase,
        sideBadgeGapBase = env.sideBadgeGapBase,
        landscapeRowCount = env.landscapeRowCount,
        Config = env.Config
    var W = Math.max(width, 1)
    var H = Math.max(height, 1)
    var vW = oppBaseWidth  * s
    var vH = oppBaseHeight * s
    var selfVH = spectating ? 0 : selfBaseHeight * s
    // Reihenabstand bewusst GRÖSSER als der Prüfabstand gapP (8) der
    // Bisektion: sonst liegen konstruierter und geforderter Abstand
    // exakt aufeinander und die Probe kippt am Rundungsfehler des
    // Umwegs über die 0..1-Slot-Brüche (Boxen fielen dann auf den
    // Skalen-Boden 0.55 zurück).
    var gapY = Math.max(opponentGapBase, opponentGapBase * s)
    // Spalten wandern mit der Box nach außen (4 px Rand), statt bei
    // festen 15 %/85 % gegen die Bildschirmkante zu laufen.
    var colX = Math.max(0.15, (4 + vW / 2) / W)
    // Reihen von oben: die oberste Box klebt (samt Sockel) 4 px unter
    // der Zonenkante, jede weitere Reihe eine Boxhöhe + Luft tiefer.
    var step = vH + gapY
    var yTC = 4 + vH / 2
    var yTop = yTC + step
    var yUpper = yTop + step
    // Unterste Reihe: knapp über der Self-Box – bzw. als Zuschauer
    // über dem Boden-Sitz BC, der dort die Self-Box ersetzt.
    var yBC = H - 4 - vH / 2
    var yBottom = spectating
        ? yBC - step
        : H - 4 - selfVH - Math.max(8, selfBadgeGapBase * s) - vH / 2
    // Die Bottom-Reihe wird erst ab 8 Ring-Sitzen besetzt; solange sie
    // leer bleibt, rückt die Lower-Reihe selbst an den unteren Anschlag.
    var rc = (ringCnt === undefined) ? ringCount : ringCnt
    var yLower = ((spectating ? rc - 1 : rc) >= 8) ? yBottom - step : yBottom
    // Notbremse, wenn die Höhe selbst am Skalen-Boden (0.55) nicht für
    // alle Reihen reicht: obere und untere Gruppe dürfen sich nicht
    // durchdringen – sie rücken dann symmetrisch auf Mindestabstand
    // auseinander (die Community hat in dem Fall kein Band mehr, die
    // Boxen bleiben aber sauber gestapelt).
    if (yLower - yUpper < step) {
        var mid = (yUpper + yLower) / 2
        yUpper = mid - step / 2
        yLower = mid + step / 2
    }
    return {
        "L_bottom": [colX,     yBottom / H],
        "L_lower":  [colX,     yLower  / H],
        "L_upper":  [colX,     yUpper  / H],
        "TL":       [colX,     yTop    / H],
        "TC":       [0.50,     yTC     / H],
        "TR":       [1 - colX, yTop    / H],
        "R_upper":  [1 - colX, yUpper  / H],
        "R_lower":  [1 - colX, yLower  / H],
        "R_bottom": [1 - colX, yBottom / H],
        "BC":       [0.50,     yBC     / H]
    }
}

function buildLandscapeSlots(env, s, ringCnt, forProbe) {
    var width = env.width, height = env.height, wide = env.wide,
        spectating = env.spectating, seatCount = env.seatCount,
        ringCount = env.ringCount, _peakSeatCount = env.peakSeatCount,
        oppBaseWidth = env.oppBaseWidth, oppBaseHeight = env.oppBaseHeight,
        selfBaseWidth = env.selfBaseWidth, selfBaseHeight = env.selfBaseHeight,
        opponentGapBase = env.opponentGapBase,
        opponentHorizontalGapBase = env.opponentHorizontalGapBase,
        selfGapBase = env.selfGapBase, selfBadgeGapBase = env.selfBadgeGapBase,
        sideBadgeGapBase = env.sideBadgeGapBase,
        landscapeRowCount = env.landscapeRowCount,
        Config = env.Config
    var rc = (ringCnt === undefined) ? ringCount : ringCnt
    var visualW = oppBaseWidth * s
    var visualH = oppBaseHeight * s
    // Zuschauer: keine Self-Box unter der Ellipse – ihr Bodenpunkt
    // ist selbst der Mittelpunkt des untersten Ring-Sitzes.
    var selfVisualH = spectating ? 0 : selfBaseHeight * s
    var sideMargin = Math.max(18, width * 0.025) + sideBadgeGapBase * s
    var wantedGapY = opponentGapBase * s
    var gapY = Math.max(8, wantedGapY)
    // Im landscapeCompact ziehen wir die untere Ellipsen-Hälfte näher
    // an die Self-Box: das verschafft den Seiten-Paaren mehr
    // vertikalen Spielraum (sonst kleben Player 7↔8 / 2↔3 visuell
    // zusammen). selfBadgeGapBase bleibt als Minimum, damit Bet-/
    // Action-Badges unterhalb der Bottom-Reihe nicht ins Self-Avatar
    // hineinragen.
    var selfGapY = spectating ? 0
        : (Config.Responsive.landscapeCompact
           ? Math.max(8, selfBadgeGapBase * s * 0.5)
           : selfBadgeGapBase * s)
    var sideX = (sideMargin + visualW / 2) / Math.max(width, 1)
    // radiusX so groß wie möglich (Seiten-Sitze landen am Rand).
    // Top-Trio passt durch den boxScale-Cap (siehe boxScale oben)
    // automatisch in dieses Bogenstück, ohne dass wir radiusX hier
    // weiter aufblasen müssen (sonst rutschen Seiten-Sitze raus).
    // radiusX nach oben auf 0.36 deckeln → Seiten-Sitze bleiben an
    // breiten Fenstern näher am Zentrum (statt ganz an den Rand zu
    // rücken).
    var radiusX = Math.min(0.36, Math.max(0.22, 0.5 - sideX))
    // Top- und Bottom-Rand bewusst klein – die offene Ellipse
    // soll möglichst viel vertikalen Platz beanspruchen, damit
    // bei mittlerer Skalierung Player 2↔3 (L↔TLo) genug Luft
    // bekommen.
    // Compact: oberste Box bündig an die Tisch-Oberkante (0 statt 4) –
    // schafft Luft zwischen ihrem Bet-Badge und dem Pot-Badge.
    // Desktop-Top-Pad 8 (statt 4): mehr Abstand der obersten Box zur
    // Status-/Info-Leiste.
    // NUR in der Probe: Reservierung für eine Bet-Badge UNTERHALB der
    // obersten Box. Das gezeichnete Layout braucht sie nicht (die
    // oben-mittige Box zeigt Einsatz und Puck seit betSplit LINKS/
    // RECHTS neben sich), die Bisektion hält den Platz auf dem
    // Desktop-Compact aber bewusst frei und deckelt die Boxen dort
    // entsprechend. Auf Mobilgeräten 0, Desktop-Ultrawide (nicht
    // compact) ebenfalls - dort hängt unterhalb nichts mehr.
    var topBadgeExt = (forProbe && Config.Responsive.landscapeCompact
                       && !Config.Responsive.isMobile) ? 39 * s : 0
    var topY = ((Config.Responsive.landscapeCompact ? 0 : 12)
                + visualH / 2 + topBadgeExt) / Math.max(height, 1)
    var selfTop = height - 4 - selfVisualH
    var bottomY = (selfTop - selfGapY - visualH / 2) / Math.max(height, 1)
    var centerY = (topY + bottomY) / 2
    // radiusY STRIKT auf das verfügbare Bahn-Stück begrenzen:
    // sonst kann das Top-Slot-Center bei `centerY - radiusY` unter
    // den `topY`-Wert rutschen → Box-Visual wird über die tableZone
    // hinaus gezeichnet und überlappt die Status-Bar. Falls der
    // resultierende radiusY zu klein für die nötige Paartrennung
    // ist, sorgt der boxScale-Cap (Bisection) dafür, dass die
    // Boxen kleiner werden.
    var radiusY = (bottomY - topY) / 2

    // lowerSquash (compact only): sin>0-Spieler via sin^0.3 nach
    // bottomY gedrückt.
    //
    // sideGravity: Zusatz-Y proportional zu |cos| → Seitenspieler
    // (|cos|→1) nach unten, TC (cos=0) bleibt. In compact-Mode nur
    // für die obere Hälfte (sinV≤0), da lowerSquash die untere Hälfte
    // bereits stark pusht und ein doppelter Push bottomY übersteigen
    // würde.
    //
    // topCosSquash: obere Hälfte (sinV≤0) nutzt |cos|^topCosSquash →
    // TL/TR (cos≈±0.62) horizontal näher an TC, reine Seitenspieler
    // (cos≈±0.97) kaum verändert.
    var lowerSquash        = Config.Responsive.landscapeCompact ? 0.2  : 1.0
    var sideGravity        = 0.25
    var topCosSquash       = 1.4
    var gravityUpperOnly   = Config.Responsive.landscapeCompact
    // Untere Sitze (sinV>0, v. a. die Bottom-Boxen bem2/danielv) werden
    // im normalen Landscape zusätzlich proportional zu sin Richtung
    // bottomY gezogen – sonst sitzen sie zu hoch und zu nah an ihren
    // oberen Nachbarn. Begrenzung auf bottomY (vFactor ≤ 1) hält den
    // selfGapY-Abstand zur Self-Box ein. Im compact-Mode übernimmt
    // das bereits lowerSquash.
    var lowerGravity       = Config.Responsive.landscapeCompact ? 0.0 : 0.15
    // Compact: Die Ecken links/rechts neben der Self-Box sind frei.
    // Untere Seiten-Sitze, die horizontal an der Self-Box vorbeigehen,
    // dürfen deshalb etwas unter bottomY absinken – das entzerrt die
    // Seiten-Paare (z. B. Player 2↔3 / 7↔8) vertikal. Maximal bis die
    // Box-Unterkante 55 % in die Self-Box-Höhe hineinragt (tiefer =
    // Player 1/9 rücken weiter runter, mehr Luft in der Mitte für
    // Community + größere Boxen).
    var maxBottomY = (selfTop + selfVisualH * 0.55 - visualH / 2) / Math.max(height, 1)
    var vMaxLower  = radiusY > 0 ? (maxBottomY - centerY) / radiusY : 1.0
    // Ohne Self-Box (Zuschauer) gibt es keine freien Ecken neben ihr →
    // Absenkung aus.
    var lowerToSelf = Config.Responsive.landscapeCompact && !spectating
    var selfClearX = (selfBaseWidth * s / 2 + visualW / 2 + 12) / Math.max(width, 1)
    function point(degrees) {
        var radians = degrees * Math.PI / 180
        var sinV = Math.sin(radians)
        var cosV = Math.cos(radians)
        var sinOrig = sinV
        if (sinV > 0 && lowerSquash !== 1.0)
            sinV = Math.pow(sinV, lowerSquash)
        if (sinV <= 0 && cosV !== 0)
            cosV = (cosV < 0 ? -1 : 1) * Math.pow(Math.abs(cosV), topCosSquash)
        var vFactor = sinV
                    + ((!gravityUpperOnly || sinV <= 0) ? sideGravity * Math.abs(cosV) : 0)
                    + (sinV > 0 ? lowerGravity * sinV : 0)
        // Desktop-Landscape: die seitlichen Außen-Paare (|cos|→1)
        // vertikal dezent entzerren – die obere Box nach oben, die
        // untere nach unten (Richtung über sinOrig). Damit ragt das
        // WINNER-Badge der unteren Box nicht mehr in die obere
        // Nachbarbox. |cos|-gewichtet, sodass die Top-Mitte (cos≈0)
        // unberührt bleibt. BEWUSST nur beim Zeichnen (!forProbe):
        // sonst würde die Bisection den größeren Paarabstand sofort
        // wieder mit größeren Boxen auffüllen statt echten Abstand zu
        // schaffen. Die Caps unten halten die Boxen in der Bahn.
        if (!forProbe && !Config.Responsive.landscapeCompact && cosV !== 0) {
            var pairSpread = 0.02 * Math.abs(cosV)
            vFactor += (sinOrig < 0 ? -pairSpread : pairSpread)
        }
        if (vFactor > 1.0) vFactor = 1.0   // nie unter bottomY (Self-Box)
        if (vFactor < -1.0) vFactor = -1.0 // nie über die obere Bahn-Kante
        // Desktop: oberen Bogen abflachen – obere Sitze (vFactor<0) 10 %
        // Richtung Mitte ziehen → flacherer Top-Bogen + mehr Abstand der
        // Top-Box zur Info-Leiste.
        if (!Config.Responsive.landscapeCompact && vFactor < 0)
            vFactor *= 0.82
        // Graduell Richtung vMaxLower absenken, gewichtet mit dem
        // ORIGINAL-sin: die untersten Sitze (BL/BR, sin≈0.88) sinken
        // fast voll ab, die darüber (sin≈0.40) nur teilweise – ein
        // einheitliches vMaxLower setzte alle auf dieselbe Höhe
        // (flache Linie statt Ellipsenbogen).
        if (lowerToSelf && sinV > 0
            && Math.abs(radiusX * cosV) > selfClearX
            && vMaxLower > vFactor)
            vFactor = vFactor + (vMaxLower - vFactor) * sinOrig
        // Fast quadratischer Tisch: obere SEITEN-Sitze (Player 3/7 auf
        // Community-Höhe) nach oben liften, sodass sie ÜBER der Community
        // liegen (sonst Overlap mit der breiten Kartenreihe). |cos|-
        // gewichtet (reine Seiten am stärksten), nur bei Aspect → 1
        // (breite Fenster bleiben unverändert).
        var sqLift = Math.max(0, Math.min(1,
            (1.6 - width / Math.max(height, 1)) / 0.6))
        if (!Config.Responsive.landscapeCompact && sinV < 0 && sqLift > 0) {
            vFactor -= 0.3 * sqLift * Math.abs(cosV)
            if (vFactor < -1.0) vFactor = -1.0
        }
        return [0.5 + radiusX * cosV, centerY + radiusY * vFactor]
    }

    // Kreis öffnet sich nach oben:
    //   – TL/TR bei 230°/310° (statt 240°/300°) → mehr horizontaler
    //     Abstand zur TC, TL/TR rücken in y-Richtung etwas tiefer.
    //   – TLo/TRo bei 200°/340° (statt 205°/335°) → fast vertikal
    //     mit L/R, dadurch mehr y-Abstand zur TL/TR.
    // BL/BR auf 120°/60° (statt 125°/55°): sin steigt 0.819→0.866
    // → Bottom-Sitze rücken ca. 5 % von radiusY weiter Richtung
    // Self-Box, der vertikale Leerraum unter ihnen schrumpft.
    // Halsketten-Modell: Self ist eine "größere Perle" am unteren
    // Bodenpunkt der Ellipse mit angularer Gewichtung relativ zu
    // einer Gegner-"Perle" (selfWeight). Die N Gegner verteilen
    // sich GLEICHMÄSSIG auf den restlichen Bogen.
    //
    // selfWeight steuert, wie viel angulare Bogenlänge die Self
    // beansprucht. Kleiner = Gegner rücken näher an die Self / weiter
    // zur Boden-Mitte und damit tiefer (mehr sin) → der Ring schließt
    // sich enger um die Self-Box, sie hebt sich nur noch minimal ab
    // (User-Wunsch). Reguläres Wide: 0.3; im landscapeCompact bleibt
    // 0.5 erhalten (eigenes, separat abgestimmtes Layout).
    //
    // Disconnectet ein Spieler, ändert sich N → automatische,
    // saubere Re-Verteilung über die unten generierten Winkel.
    //
    // Zuschauer: Sitz 0 ist eine ganz normale Perle (selfWeight 1.0)
    // → dOpp = 360/seatCount, firstOppAngle = 90 + dOpp. Sitz 0 liegt
    // damit exakt auf dem Bodenpunkt (90°), die übrigen Sitze folgen
    // gleichmäßig im Kreis.
    // Gegner-"Perlen" aus dem Parameter (NICHT aus seatCount): die
    // Probe rechnet bewusst mit _peakSeatCount, damit ausscheidende
    // Spieler die Box-Größe nicht verändern, das Zeichnen dagegen mit
    // der aktuellen Sitzzahl. rc ist in beiden Fällen die Zahl der
    // Ring-Sitze (als Zuschauer inkl. Sitz 0).
    var opps = Math.max(1, spectating ? rc - 1 : rc)
    var selfWeight = spectating ? 1.0
                   : (Config.Responsive.landscapeCompact ? 0.5 : 0.3)
    var dOpp = 360 / (opps + selfWeight)
    var dSelf = selfWeight * dOpp
    var firstOppAngle = 90 + (dSelf + dOpp) / 2
    var slots = {}
    // Sitz 0 des Zuschauers: exakt auf dem Bodenpunkt der Ellipse.
    if (spectating)
        slots["opp0"] = point(90)
    for (var i = 1; i <= opps; i++) {
        slots["opp" + i] = point(firstOppAngle + (i - 1) * dOpp)
    }

    // Der Ring ist von Haus aus kopflastig-nach-unten: sideGravity und
    // lowerGravity drücken die Sitze abwärts, und am oberen Scheitel
    // (270°) sitzt bei den meisten Spielerzahlen niemand. Mit Self-Box
    // ist das gewollt – der Ring soll sich um sie schließen. Als
    // Zuschauer bleibt oben dadurch eine große Lücke, während der
    // Bodensitz am Fensterrand klebt.
    //
    // Statt die (fein austarierten) Gravity-Terme anzufassen, wird der
    // FERTIGE Ring als Ganzes vertikal in der Zone zentriert. Eine reine
    // Verschiebung: alle Paar-Abstände – und damit die Bisektion in
    // boxScale – bleiben unberührt, ebenso der Abstand zwischen oberen
    // Sitzen und Community (beide verschieben sich gleich weit).
    if (spectating && !forProbe) {
        var halfH = (visualH / 2) / Math.max(height, 1)
        var minY = Infinity, maxY = -Infinity
        for (var name in slots) {
            var sy = slots[name][1]
            if (sy < minY) minY = sy
            if (sy > maxY) maxY = sy
        }
        // Gleiche Ränder oben/unten: top + shift == 1 - (bottom + shift)
        var shiftY = (1 - (maxY + halfH) - (minY - halfH)) / 2
        for (var key in slots)
            slots[key] = [slots[key][0], slots[key][1] + shiftY]
    }
    // geom erlaubt der Probe, aus [xNorm, yNorm] wieder cos und
    // vFactor zu gewinnen (x = 0.5 + radiusX·cos, y = centerY + radiusY·v).
    return { slots: slots,
             geom: { radiusX: radiusX, radiusY: radiusY, centerY: centerY } }
}

function boxScale(env) {
    var width = env.width, height = env.height, wide = env.wide,
        spectating = env.spectating, seatCount = env.seatCount,
        ringCount = env.ringCount, _peakSeatCount = env.peakSeatCount,
        oppBaseWidth = env.oppBaseWidth, oppBaseHeight = env.oppBaseHeight,
        selfBaseWidth = env.selfBaseWidth, selfBaseHeight = env.selfBaseHeight,
        opponentGapBase = env.opponentGapBase,
        opponentHorizontalGapBase = env.opponentHorizontalGapBase,
        selfGapBase = env.selfGapBase, selfBadgeGapBase = env.selfBadgeGapBase,
        sideBadgeGapBase = env.sideBadgeGapBase,
        landscapeRowCount = env.landscapeRowCount,
        Config = env.Config
    if (width <= 0 || height <= 0) return 1.0
    // Für die Skalierungsberechnung den Peak-Wert nutzen, damit
    // ausscheidende Spieler die Box-Größe nicht verändern.
    var oppCnt = _peakSeatCount - 1
    // Sitze auf dem Ring (als Zuschauer inkl. Sitz 0), s. ringCount.
    var ringCnt = spectating ? _peakSeatCount : oppCnt
    var s

    // Box-Skala-Obergrenze wächst mit der Spielerzahl: bei wenigen
    // Spielern sollen die Boxen NICHT den ganzen leeren Tisch
    // ausfüllen. Ohne diesen Deckel sprang die Skala bei z. B. nur
    // zwei Spielern auf das Maximum (1.4 bzw. 1.7) – Self- und
    // Gegnerbox wirkten viel zu groß und ihre Bet-Badges berührten
    // die Gemeinschaftskarten. Ab 6 Gegnern (7+ Spielern) wird die
    // volle Obergrenze erreicht, die dicht besetzten Tische bleiben
    // daher unverändert. Der Deckel SENKT nur (nie erhöht) und kann
    // somit keine neuen Überlappungen erzeugen.
    function fillCap(maxScale, minBase) {
        var base = (minBase === undefined) ? 0.95 : minBase
        var t = Math.max(0, Math.min(1, (oppCnt - 1) / 5))
        var countCap = base + (maxScale - base) * t
        // Wenige Spieler bei großen (maximierten/Vollbild-) Fenstern
        // mitwachsen lassen: ohne dies bleibt boxScale gedeckelt,
        // während die Ellipse (Bruchteil der Breite) aufspreizt → im
        // Vollbild winzige Boxen am Rand + leere Mitte. Wächst NUR
        // oberhalb der gewohnten Fenstergröße (√(w·h) ≈ 760 bei
        // 1024×521) und nur bei wenigen Spielern (1-t); gedeckelt
        // bei 2.2, damit Boxen/Schrift nicht grotesk groß werden.
        // √(width·height) statt nur height, weil das Problem im
        // Fullscreen-Querformat vor allem durch die BREITE entsteht:
        // bei 1920×960 ist √ ≈ 1358 statt 960 → grow dreimal stärker
        // → boxScale ~1.75 statt ~1.28 → Boxen füllen proportional
        // zum Bildschirm. Dichte Tische und die Startauflösung bleiben
        // unverändert.
        var grow = (1 - t) * Math.max(0, (Math.sqrt(width * height) - 760) / 700)
        // Voll besetzte Tische (t≈1) bei breiten Fenstern: die
        // Bottom-Seiten-Sitze (Player 1/N) bekommen durch den größer
        // werdenden radiusX mehr Abstand zu ihren oberen Nachbarn –
        // feasibleAt() erlaubt dadurch deutlich höhere boxScale als
        // bei Normalfenster. Bei hohen Einsatzbeträgen könnte das
        // Badge von Player 1/N in die Nachbarbox ragen. Linear ab
        // 1024 px, maximal –0.15 (Deckel ≥ 1.25 für 9+ Spieler).
        var denseShrink = t * Math.min(0.15, Math.max(0, (width - 1024) / 4000))
        return Math.min(2.2, countCap * (1 + grow) - denseShrink)
    }

    // Strategie: Box-Skala = MAXIMUM, das alle geometrischen
    // Constraints erfüllt. Dadurch füllen die Boxen den
    // verfügbaren Tisch optimal aus – breite Fenster bekommen
    // große Boxen (vertikale Reihenpassung als Obergrenze),
    // schmale Fenster bekommen automatisch kleinere Boxen
    // (Slot-Sichtbarkeit als Obergrenze). Kein künstlicher
    // ref/700-Boden mehr, der breite Fenster auf der kleineren
    // Dimension klein hielt.
    if (wide) {
        // Landscape-Cap per Bisektion: maximaler boxScale, für den
        // ALLE benachbarten Ellipsen-Sitzpaare entweder horizontal
        // ODER vertikal voneinander getrennt bleiben.
        //
        // WICHTIG: jeder Probepunkt rechnet `radiusX`/`radiusY`
        // mit den exakt gleichen s-abhängigen Formeln wie
        // `buildLandscapeSlots()`. Frühere statische Schätzwerte
        // (`sideMarginBase` ohne s-Faktor, `approxRy = 0.30`)
        // unterschätzten den BL/BR-Pair-Bedarf bei großen s →
        // bei 9–10 Spielern und sehr breitem Fenster überlappten
        // die Boxen, obwohl die alte Cap-Formel noch grünes
        // Licht gab.
        // Reiner Sicherheits-Abstand zwischen benachbarten Box-Paaren
        // in der Bisektions-Probe. Kleiner = Boxen dürfen enger packen
        // = höheres feasibles boxScale (größere Boxen/Karten/Schrift).
        // Im landscapeCompact (kleine Phone-Landscape-Auflösungen wie
        // iPhone mini, Höhe < 600) deutlich großzügiger: dort standen die
        // oberen/seitlichen Gegner-Paare (Player 2↔3 / 7↔8) so eng, dass
        // sie sich visuell berührten. Mehr Sicherheits-Abstand → die
        // Bisektion cappt boxScale niedriger → kleinere, überlappungs-
        // freie Boxen.
        var gap = Config.Responsive.landscapeCompact ? 12 : 4
        // Ring-Sitze in Winkelreihenfolge. Bei Zuschauern sitzt
        // zusätzlich opp0 auf dem Bodenpunkt (90°), sodass auch das
        // Paar opp0↔opp1 geprüft wird; das Wrap-Paar oppN↔opp0 ist
        // dazu spiegelsymmetrisch und damit abgedeckt.
        var probeSeq = []
        if (spectating) probeSeq.push("opp0")
        for (var iSeq = 1; iSeq <= (spectating ? ringCnt - 1 : ringCnt); iSeq++)
            probeSeq.push("opp" + iSeq)

        function feasibleAt(sTest) {
            if (oppCnt < 2) return true
            var visualH = oppBaseHeight * sTest
            // Zuschauer: unterhalb der Ellipse liegt keine Self-Box,
            // der unterste Sitz IST der Bodenpunkt der Ellipse.
            var selfVisualH = spectating ? 0 : selfBaseHeight * sTest
            // Exakt die Geometrie, die bei dieser Skala auch gezeichnet
            // würde - keine Nachbildung mehr, daher kein Drift möglich.
            var built = buildLandscapeSlots(env, sTest, ringCnt, true)
            var pos  = built.slots
            var geom = built.geom
            if (geom.radiusY <= 0 || geom.radiusX <= 0) return false

            // Community-Karten werden mittig in die Lücke zwischen der
            // Unterkante der obersten Gegnerbox und der Oberkante der
            // Self-Box gelegt (siehe communityCenterY). Diese Lücke muss
            // groß genug sein, damit Kartenreihe + Pott-Badge hinein-
            // passen - sonst boxScale verkleinern. Nur reguläres Wide;
            // im landscapeCompact sitzt die Community per eigener Formel.
            if (!Config.Responsive.landscapeCompact) {
                // Oberkante des untersten Tisch-Inhalts, gegen die die
                // Community-Reihe geprüft wird: die Self-Box bzw. - als
                // Zuschauer - der unterste Ring-Sitz selbst.
                var bottomContentTop = spectating
                    ? height - 4 - visualH
                    : height - 12 - selfVisualH
                // Tiefste Unterkante der oberen Sitze - die zentrale
                // Top-Box zählt zusätzlich ihre nach unten zeigende
                // Bet-Badge mit (sonst ragt sie in die Kartenreihe).
                var topOppBottom = -1e9
                for (var key in pos) {
                    var pC = pos[key]
                    // Ellipsen-Vektor zurückrechnen:
                    //   x = 0.5 + radiusX·cos,  y = centerY + radiusY·v
                    var vY = (pC[1] - geom.centerY) / geom.radiusY
                    if (vY >= 0) continue          // nur obere Sitze
                    var vX = (pC[0] - 0.5) / geom.radiusX
                    var b = pC[1] * height + visualH / 2
                          + (Math.abs(vX) < 0.25 ? sTest * 25 : 0)
                    if (b > topOppBottom) topOppBottom = b
                }
                // Community-Gesamthöhe (Kartenreihe 64 + Pott-Badge 40 +
                // Winner-Badge 20) · communityScale + Pad.
                if (topOppBottom > -1e9
                    && bottomContentTop - topOppBottom
                       < 0.72 * sTest * 124 + 28)
                    return false
            }

            // Paar-Trennung: benachbarte Ring-Sitze müssen entweder
            // horizontal ODER vertikal genug Abstand haben.
            // Bet-Badges auf beiden Seiten einrechnen (chip+text+Abstand).
            // Ohne diesen Aufschlag erlaubt die Bisection zu große scales
            // und die Einsatz-Anzeige reicht in die Nachbarbox hinein.
            var xNeeded = sTest * (oppBaseWidth + sideBadgeGapBase) + gap
            var yNeeded = sTest * oppBaseHeight + gap
            for (var iPair = 0; iPair < probeSeq.length - 1; iPair++) {
                var a = pos[probeSeq[iPair]]
                var b2 = pos[probeSeq[iPair + 1]]
                if (!a || !b2) continue
                if (Math.abs(a[0] - b2[0]) * width  < xNeeded
                    && Math.abs(a[1] - b2[1]) * height < yNeeded)
                    return false
            }
            return true
        }

        // Heads-up (1 Gegner): feasibleAt() hat kein Nachbar-Paar zu
        // prüfen. Stattdessen sicherstellen, dass die mittig in die
        // Lücke zwischen oben-zentrierter Gegnerbox (inkl. nach unten
        // zeigender Bet-Badge) und Self-Box gelegte Community-Reihe
        // hineinpasst – kritisch in flachen Fenstern. Gleiche Logik
        // wie die Community-Prüfung in feasibleAt().
        function feasibleHeadsUp(sTest) {
            if (sTest <= 0) return false
            var visualH = oppBaseHeight * sTest
            // Oberkante der untersten Box: Self-Box bzw. – als
            // Zuschauer – der gleich große unterste Ring-Sitz.
            var bottomContentTop = spectating
                ? height - 4 - visualH
                : height - 12 - selfBaseHeight * sTest
            var topYband = (Config.Responsive.landscapeCompact ? 0 : 4) + visualH / 2
            var topOppBottom = topYband + visualH / 2 + sTest * 25
            return bottomContentTop - topOppBottom
                   >= 0.72 * sTest * 124 + 28
        }

        // Gemeinsames Limit für Gegnerboxen, Self-Box und Community-Badges:
        // 1.4 verhindert zu große Schrift und Bet-Überlappungen bei
        // Vollbild/maximiert; compact bleibt bei 1.7 (breiter, flacher).
        // fillCap() dämpft das Maximum bei wenigen Spielern zusätzlich.
        var lo = 0.55, hi = fillCap(Config.Responsive.landscapeCompact ? 2.3 : 1.9)
        if (oppCnt < 2) {
            // Bis zum (gedeckelten) hi gehen, solange die Badges die
            // Community nicht berühren.
            if (!feasibleHeadsUp(lo)) {
                s = lo
            } else if (feasibleHeadsUp(hi)) {
                s = hi
            } else {
                for (var iterH = 0; iterH < 14; iterH++) {
                    var midH = (lo + hi) / 2
                    if (feasibleHeadsUp(midH)) lo = midH
                    else hi = midH
                }
                s = lo
            }
        } else if (!feasibleAt(lo)) {
            s = lo
        } else {
            for (var iter = 0; iter < 14; iter++) {
                var mid = (lo + hi) / 2
                if (feasibleAt(mid)) lo = mid
                else hi = mid
            }
            s = lo
        }
    } else {
        // Portrait-Cap per Bisektion (analog Wide-Screen).
        //
        // MOBIL: die Probepunkte holen ihre Slots aus
        // buildPortraitSlots(env, sTest) – exakt der Funktion, die auch
        // das Layout zeichnet. Sie ist eine reine Funktion der Skala,
        // daher gibt es keine Zirkularität und keine zweite, leicht
        // abweichende Formel-Kopie wie im Querformat. Es bleiben nur
        // die Grenzen, die wirklich anliegen:
        //   • Bildschirmbreite (die Spalten rücken mit der Box mit)
        //   • Abstand der beiden Spalten inkl. Einsatz/Puck daneben
        //   • Mittelband für die Community-Reihe
        // Der Reihenabstand selbst ist durch die Konstruktion erfüllt;
        // die Paar-Schleife unten bleibt als Absicherung stehen.
        //
        // DESKTOP (statische Slots, slotPosPortraitFixed): unverändert
        // die alten Wand- und Paar-Constraints.
        //   • Wand links/rechts:  Seitenspalten x=0.15/0.85
        //   • Wand oben:          TC bei y=0.075
        //   • Wand unten:         Bottom-Reihe (L_bottom/R_bottom
        //                         bei y=0.785) darf die Self-Box
        //                         (bottomMargin=20) nicht berühren.
        //   • Paar-Trennung:      benachbarte Sitze in
        //                         slotSeqPortrait[oppCnt] müssen
        //                         entweder horizontal ODER vertikal
        //                         genug Abstand zueinander haben.
        var gapP = 8
        var mobileP = Config.Responsive.isMobile
        var seqP = (spectating ? slotSeqSpectate(wide) : slotSeqPortrait)[ringCnt] || []

        function feasibleAtP(sTest) {
            if (sTest <= 0) return false
            var visualW = oppBaseWidth * sTest
            var visualH = oppBaseHeight * sTest
            var selfVisualH = selfBaseHeight * sTest
            var posP = mobileP ? buildPortraitSlots(env, sTest, ringCnt)
                               : slotPosPortraitFixed
            // Zwischen den beiden Spalten stehen sich zwei Boxen
            // gegenüber – im Stil "classic" trägt JEDE ihren Einsatz
            // zur Tischmitte hin, im Stil "inset" nur noch den Puck
            // (Config.SeatStyle.betSideOutset).
            var xNeeded = mobileP
                ? sTest * (oppBaseWidth + 2 * Config.SeatStyle.betSideOutset) + gapP
                : sTest * oppBaseWidth + gapP
            var yNeeded = sTest * oppBaseHeight + gapP

            if (mobileP) {
                // Wand links/rechts: nur noch die Bildschirmbreite –
                // die Spalten selbst wandern mit der Box nach außen.
                if (visualW > width - 8) return false
                // Mittelband muss die Community-Reihe tragen.
                var bandP = portraitBandAt(env, sTest, posP, seqP)
                if (bandP[1] - bandP[0] < portraitCommunityNeed(env, sTest))
                    return false
            } else {
                // Wand-Checks
                if (visualW > 2 * (0.15 * width - 4)) return false
                if (visualH > 2 * (0.075 * height - 4)) return false
                if (spectating) {
                    // Wand unten: der BC-Sitz (y=0.90) darf den unteren
                    // Rand nicht berühren. Es gibt keine Self-Box.
                    if (0.90 * height + visualH / 2 > height - 4) return false
                } else if (oppCnt >= 8) {
                    // Self-Box vs. Bottom-Reihe (L_bottom/R_bottom bei oppCnt>=8).
                    // seatNudge=+14 für diese Slots wird berücksichtigt:
                    //   self_top    = height - 4 - selfVisualH  (scale-kompensierbares bottomMargin)
                    //   bottom_kant = 0.785*height + 14 + visualH/2
                    //   Abstand     = 0.215*height - 18 - selfVisualH - visualH/2
                    //   Constraint  = Abstand >= gapP  →  0.215*H - 26 - ... >= 0
                    if (0.215 * height - 26 - selfVisualH - visualH / 2 < gapP)
                        return false
                }
            }

            // Paar-Trennung
            if (seqP.length < 2) return true
            for (var i = 0; i < seqP.length - 1; i++) {
                var a = posP[seqP[i]]
                var b = posP[seqP[i + 1]]
                if (!a || !b) continue
                var dxPix = Math.abs(a[0] - b[0]) * width
                var dyPix = Math.abs(a[1] - b[1]) * height
                if (dxPix < xNeeded && dyPix < yNeeded)
                    return false
            }
            return true
        }

        // Mobil darf die Obergrenze höher liegen (und der Boden des
        // Spielerzahl-Deckels ebenfalls): mit den dynamischen Reihen
        // begrenzt jetzt echte Geometrie statt fester Brüche, und ein
        // Zweier-Tisch im Hochformat soll den Platz auch nutzen.
        var loP = 0.55, hiP = mobileP ? fillCap(2.0, 1.15) : fillCap(1.85)
        if (!feasibleAtP(loP)) {
            s = loP
        } else {
            for (var iterP = 0; iterP < 14; iterP++) {
                var midP = (loP + hiP) / 2
                if (feasibleAtP(midP)) loP = midP
                else hiP = midP
            }
            s = loP
        }
    }

    // Lesbarkeits-Boden – Schrift/Karten skalieren mit, dürfen aber
    // nicht beliebig klein werden.
    return Math.max(0.55, s)
}
