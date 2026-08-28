pragma Singleton
import QtQuick

// Katalog der Emoji-Reaktionen – Port des Web-Clients (Stand 2026-08-28):
// 90 Reaktionen auf drei thematischen Seiten, 16 Choreografien und die
// Partikel-Presets sparkle/shock/confetti/boom/gunshot.
//
// Die Keyframes sind die CSS-Animationen rfx* des Web-Clients
// (public/pokerth.css, dokumentiert in dessen docs/REACTIONS_FX.md); die
// CSS-Prozentwerte der Transformationen werden über pxPerPercent in Pixel
// umgerechnet (-150 % ⇒ 160 px Aufstieg).
//
// Genutzt von ReactionPicker.qml (Auswahl) und GameReactionFx.qml
// (Animation). Der Qt-Widgets-Client führt dieselben Tabellen in
// gui/qt/chattools/emojipicker.cpp und gui/qt/gametable/reactionfx.cpp.
// Das Protokoll bleibt unverändert: gesendet wird "/emoji <Zeichen>".
QtObject {

    // ── Die 90 Reaktionen, drei Seiten à 30 (Reihenfolge wie im Web-Client) ──
    readonly property var pages: [
        [
         "😂", "🤣", "😅", "😭", "🥺", "😢",
         "😏", "🙄", "😳", "🤪", "😇", "😍",
         "🥰", "😘", "😬", "😴", "🤔", "👀",
         "😮", "😱", "🤯", "😡", "😤", "🤢",
         "🥴", "🙃", "🫣", "😐", "🥱", "🙈"
        ],
        [
         "😎", "🤩", "🤡", "😈", "🫠", "🥶",
         "🥵", "🎉", "🥳", "🍿", "👏", "🙌",
         "💪", "👍", "👎", "🤝", "👊", "🙏",
         "🤞", "🫵", "🫡", "🤫", "🤦", "🚬",
         "⏳", "🍺", "☕", "💣", "🚀", "⚡"
        ],
        [
         "💰", "🤑", "💵", "💎", "🎰", "🍀",
         "🃏", "♠️", "🎲", "🎯", "🏆", "🥇",
         "💸", "🪤", "👑", "🔥", "💀", "🦈",
         "🐟", "🐔", "🫏", "🎩", "🧊", "🌪️",
         "🔫", "📈", "📉", "🔮", "💯", "⭐"
        ]
    ]

    // Symbol der jeweiligen Seite (Emotionen / Stimmung & Gesten / Poker & Glück)
    readonly property var pageIcons: ["😀", "👏", "♠️"]

    // ── Effekt-Katalog je Reaktion ──
    //   a: Choreografie des großen Emojis (siehe anims)
    //   p: Partikel-Spezifikation oder Preset "sparkle" | "shock" |
    //      "confetti" | "boom" | "gunshot"
    //      {chars, count, size, a0, a1, dist, g, life, rot, color}
    //      a0..a1 = Winkelbereich (Grad, 0=rechts, -90=oben), dist = Wurfweite,
    //      g = zusätzlicher Fall am Ende, life = Lebensdauer ms.
    //      Ohne chars werden farbige Punkte geworfen.
    readonly property var fx: ({
        // ── Seite 1 (😀 Emotions) ──
        "😂": { a: "shake", p: { chars: ["💧"], count: 7, size: 13, a0: -30, a1: 210, dist: 55, g: 36, life: 850 } },
        "🤣": { a: "shake", p: { chars: ["💧"], count: 7, size: 13, a0: -30, a1: 210, dist: 55, g: 36, life: 850 } },
        "😅": { a: "shake", p: { chars: ["💧"], count: 5, size: 12, a0: -40, a1: 220, dist: 48, g: 40, life: 800 } },
        "😭": { a: "shake", p: { chars: ["💧"], count: 11, size: 14, a0: -20, a1: 200, dist: 60, g: 70, life: 1000 } },
        "🥺": { a: "heartbeat", p: { chars: ["✨", "💖"], count: 8, size: 13, a0: 0, a1: 360, dist: 56, life: 850 } },
        "😢": { a: "wobble", p: { chars: ["💧"], count: 6, size: 13, a0: -30, a1: 210, dist: 50, g: 55, life: 900 } },
        "😏": { a: "pop", p: "sparkle" },
        "🙄": { a: "tilt", p: { chars: ["✦"], count: 5, size: 11, a0: 0, a1: 360, dist: 44, life: 650 } },
        "😳": { a: "zoomout", p: "sparkle" },
        "🤪": { a: "wobble", p: { chars: ["✦", "✧"], count: 8, size: 12, a0: 0, a1: 360, dist: 58, life: 800, rot: true } },
        "😇": { a: "shine", p: { chars: ["✨"], count: 8, size: 13, a0: -160, a1: -20, dist: 58, g: -26, life: 900, color: "#E3C800" } },
        "😍": { a: "heartbeat", p: { chars: ["❤️", "💖"], count: 8, size: 16, a0: -160, a1: -20, dist: 64, g: -30, life: 1100 } },
        "🥰": { a: "heartbeat", p: { chars: ["💕", "💖"], count: 9, size: 15, a0: -170, a1: -10, dist: 62, g: -28, life: 1050 } },
        "😘": { a: "heartbeat", p: { chars: ["💋", "❤️"], count: 7, size: 15, a0: -150, a1: -30, dist: 60, g: -32, life: 1000 } },
        "😬": { a: "shiver", p: { chars: ["💦"], count: 4, size: 11, a0: -120, a1: -60, dist: 40, g: 44, life: 700 } },
        "😴": { a: "drop", p: { chars: ["💤"], count: 5, size: 14, a0: -120, a1: -60, dist: 52, g: -40, life: 1100 } },
        "🤔": { a: "wobble", p: { chars: ["✦"], count: 5, size: 11, a0: 0, a1: 360, dist: 42, life: 650 } },
        "👀": { a: "zoomout", p: "sparkle" },
        "😮": { a: "zoomout", p: { chars: ["✦"], count: 6, size: 12, a0: 0, a1: 360, dist: 50, life: 700 } },
        "😱": { a: "shake", p: { chars: ["💦"], count: 6, size: 12, a0: -120, a1: -60, dist: 48, g: 50, life: 780 } },
        "🤯": { a: "tilt", p: "shock" },
        "😡": { a: "shiver", p: { chars: ["💢", "🔥"], count: 7, size: 13, a0: 0, a1: 360, dist: 54, life: 800 } },
        "😤": { a: "flex", p: { chars: ["💨"], count: 6, size: 14, a0: -190, a1: 10, dist: 52, life: 750 } },
        "🤢": { a: "wobble", p: { count: 8, size: 7, a0: 0, a1: 360, dist: 50, life: 750, color: "#7ee37e" } },
        "🥴": { a: "wobble", p: { chars: ["🌀", "✦"], count: 6, size: 12, a0: 0, a1: 360, dist: 52, life: 850, rot: true } },
        "🙃": { a: "flip", p: "sparkle" },
        "🫣": { a: "pop", p: { chars: ["✦"], count: 5, size: 11, a0: 0, a1: 360, dist: 44, life: 650 } },
        "😐": { a: "pop", p: { chars: ["✦"], count: 3, size: 10, a0: 0, a1: 360, dist: 36, life: 600 } },
        "🥱": { a: "wobble", p: { chars: ["💤"], count: 5, size: 14, a0: -130, a1: -50, dist: 52, g: -42, life: 1100 } },
        "🙈": { a: "shake", p: { chars: ["✦"], count: 6, size: 11, a0: 0, a1: 360, dist: 48, life: 700 } },
        // ── Seite 2 (👏 Mood & gestures) ──
        "😎": { a: "pop", p: "sparkle" },
        "🤩": { a: "shine", p: { chars: ["✨"], count: 8, size: 13, a0: 0, a1: 360, dist: 60, life: 800, rot: true } },
        "🤡": { a: "wobble", p: "confetti" },
        "😈": { a: "tilt", p: { chars: ["🔥", "✦"], count: 8, size: 13, a0: 0, a1: 360, dist: 58, life: 850, rot: true } },
        "🫠": { a: "wobble", p: { chars: ["💧"], count: 6, size: 12, a0: 40, a1: 140, dist: 44, g: 70, life: 950 } },
        "🥶": { a: "shiver", p: { chars: ["❄️", "🧊"], count: 8, size: 13, a0: 0, a1: 360, dist: 56, life: 900, rot: true } },
        "🥵": { a: "fire", p: { chars: ["🔥", "💦"], count: 8, size: 13, a0: -160, a1: -20, dist: 60, g: -20, life: 900 } },
        "🎉": { a: "pop", p: "confetti" },
        "🥳": { a: "pop", p: "confetti" },
        "🍿": { a: "beat", p: { chars: ["🍿"], count: 9, size: 13, a0: -160, a1: -20, dist: 60, g: 70, life: 1000, rot: true } },
        "👏": { a: "beat", p: { chars: ["✦", "✧"], count: 9, size: 13, a0: 0, a1: 360, dist: 60, life: 750, color: "#E3C800" } },
        "🙌": { a: "beat", p: { chars: ["✦", "✧"], count: 9, size: 13, a0: 0, a1: 360, dist: 62, life: 780, color: "#E3C800" } },
        "💪": { a: "flex", p: { chars: ["✦"], count: 6, size: 13, a0: 0, a1: 360, dist: 50, life: 700, color: "#E3C800" } },
        "👍": { a: "beat", p: "sparkle" },
        "👎": { a: "drop", p: { count: 7, size: 6, a0: 20, a1: 160, dist: 48, g: 60, life: 800, color: "#9aa0a6" } },
        "🤝": { a: "pop", p: "sparkle" },
        "👊": { a: "flex", p: "shock" },
        "🙏": { a: "shine", p: { chars: ["✨"], count: 9, size: 13, a0: -160, a1: -20, dist: 60, g: -24, life: 950, color: "#E3C800" } },
        "🤞": { a: "beat", p: { chars: ["🍀", "✨"], count: 8, size: 13, a0: 0, a1: 360, dist: 58, life: 850, rot: true } },
        "🫵": { a: "zoomout", p: "sparkle" },
        "🫡": { a: "pop", p: "sparkle" },
        "🤫": { a: "pop", p: { chars: ["✦"], count: 4, size: 10, a0: 0, a1: 360, dist: 38, life: 600 } },
        "🤦": { a: "drop", p: { chars: ["💧"], count: 4, size: 12, a0: -120, a1: -60, dist: 42, g: 46, life: 700 } },
        "🚬": { a: "wobble", p: { chars: ["💨"], count: 7, size: 14, a0: -130, a1: -50, dist: 64, g: -46, life: 1400, rot: true } },
        "⏳": { a: "flip", p: { chars: ["✦"], count: 6, size: 11, a0: 0, a1: 360, dist: 48, life: 700 } },
        "🍺": { a: "wobble", p: { chars: ["🫧"], count: 9, size: 12, a0: -140, a1: -40, dist: 58, g: -50, life: 1100 } },
        "☕": { a: "pop", p: { chars: ["💨"], count: 5, size: 13, a0: -120, a1: -60, dist: 50, g: -40, life: 1000 } },
        "💣": { a: "drop", p: "boom" },
        "🚀": { a: "launch", p: { chars: ["🔥", "✨"], count: 10, size: 13, a0: 60, a1: 120, dist: 80, g: 60, life: 900 } },
        "⚡": { a: "zoomout", p: { chars: ["⚡", "✦"], count: 8, size: 14, a0: 0, a1: 360, dist: 66, life: 750, rot: true } },
        // ── Seite 3 (♠️ Poker & luck) ──
        "💰": { a: "pop", p: { chars: ["🪙", "💵", "✦"], count: 12, size: 16, a0: -170, a1: -10, dist: 72, g: 90, life: 1200, rot: true } },
        "🤑": { a: "pop", p: { chars: ["🪙", "💵"], count: 10, size: 16, a0: -170, a1: -10, dist: 70, g: 90, life: 1100, rot: true } },
        "💵": { a: "drop", p: { chars: ["💵", "🪙"], count: 10, size: 15, a0: -170, a1: -10, dist: 70, g: 85, life: 1150, rot: true } },
        "💎": { a: "shine", p: { chars: ["✨", "✦"], count: 9, size: 13, a0: 0, a1: 360, dist: 64, life: 850, rot: true } },
        "🎰": { a: "spin", p: { chars: ["✨", "🪙"], count: 9, size: 14, a0: 0, a1: 360, dist: 66, life: 950, rot: true } },
        "🍀": { a: "spin", p: { chars: ["✨", "🍀"], count: 8, size: 13, a0: 0, a1: 360, dist: 62, life: 950, color: "#7ee37e", rot: true } },
        "🃏": { a: "flip", p: "sparkle" },
        "♠️": { a: "flip", p: { chars: ["♠️", "♥️", "♦️", "♣️"], count: 8, size: 14, a0: 0, a1: 360, dist: 62, life: 900, rot: true } },
        "🎲": { a: "spin", p: { chars: ["✦", "✧"], count: 8, size: 12, a0: 0, a1: 360, dist: 58, life: 800, rot: true } },
        "🎯": { a: "zoomout", p: "sparkle" },
        "🏆": { a: "shine", p: { chars: ["⭐", "✨"], count: 10, size: 14, a0: 0, a1: 360, dist: 68, life: 1000, color: "#E3C800", rot: true } },
        "🥇": { a: "shine", p: { chars: ["✨"], count: 8, size: 13, a0: 0, a1: 360, dist: 60, life: 900, color: "#E3C800" } },
        "💸": { a: "launch", p: { chars: ["💵", "🪙"], count: 10, size: 14, a0: -150, a1: -30, dist: 75, g: -40, life: 1100, rot: true } },
        "🪤": { a: "drop", p: "shock" },
        "👑": { a: "shine", p: { chars: ["✨", "⭐"], count: 10, size: 14, a0: 0, a1: 360, dist: 70, life: 1000, color: "#E3C800", rot: true } },
        "🔥": { a: "fire", p: { chars: ["🔥", "✦"], count: 9, size: 14, a0: -150, a1: -30, dist: 70, g: -24, life: 1000, rot: true } },
        "💀": { a: "shiver", p: { count: 8, size: 6, a0: 0, a1: 360, dist: 52, life: 800, color: "#9aa0a6" } },
        "🦈": { a: "pop", p: { chars: ["💦", "🌊"], count: 8, size: 14, a0: -170, a1: -10, dist: 62, g: 40, life: 900 } },
        "🐟": { a: "wobble", p: { chars: ["🫧"], count: 9, size: 12, a0: -140, a1: -40, dist: 58, g: -52, life: 1150 } },
        "🐔": { a: "shake", p: { chars: ["🪶"], count: 8, size: 14, a0: -30, a1: 210, dist: 56, g: 60, life: 1200, rot: true } },
        "🫏": { a: "wobble", p: { chars: ["✦"], count: 6, size: 11, a0: 0, a1: 360, dist: 48, life: 750 } },
        "🎩": { a: "flip", p: { chars: ["✨"], count: 7, size: 12, a0: 0, a1: 360, dist: 54, life: 800 } },
        "🧊": { a: "shiver", p: { chars: ["❄️"], count: 7, size: 12, a0: 0, a1: 360, dist: 52, life: 850 } },
        "🌪️": { a: "tilt", p: { chars: ["🍃", "💨"], count: 10, size: 13, a0: 0, a1: 360, dist: 74, life: 950, rot: true } },
        "🔫": { a: "recoil", p: "gunshot" },
        "📈": { a: "launch", p: { count: 8, size: 6, a0: -120, a1: -60, dist: 62, g: -30, life: 850, color: "#7ee37e" } },
        "📉": { a: "drop", p: { count: 8, size: 6, a0: 60, a1: 120, dist: 58, g: 70, life: 850, color: "#e05252" } },
        "🔮": { a: "shine", p: { chars: ["✨", "✦"], count: 8, size: 13, a0: 0, a1: 360, dist: 60, life: 900, rot: true } },
        "💯": { a: "zoomout", p: { chars: ["✦", "💯"], count: 6, size: 13, a0: 0, a1: 360, dist: 56, life: 800 } },
        "⭐": { a: "shine", p: { chars: ["⭐", "✨"], count: 9, size: 13, a0: 0, a1: 360, dist: 62, life: 900, rot: true } }
    })

    // Umrechnung der CSS-Prozentwerte in Pixel: -150 % (Standard-Aufstieg)
    // entspricht 160 px – die gewohnte Flughöhe am Tisch.
    readonly property real pxPerPercent: 1.6
    // Grundgröße des großen Emojis (Skalierung 1.0).
    readonly property real baseSize: 34

    // ── Die 16 Choreografien (CSS-Keyframes rfx*) ──
    //   dur = Dauer in ms, e = Timing-Function als kubische Bézier-Kontroll-
    //   punkte; sie wirkt – wie in CSS – auf JEDEN Keyframe-Abschnitt einzeln.
    //   Kanäle (jeweils [Zeitanteil, Wert], fehlende Kanäle = Standardwert):
    //     x, y = CSS-Prozent (Basis -50), s = Skalierung, r = Drehung (Grad),
    //     ry = Drehung um die Y-Achse (Kartendreher), o = Deckkraft,
    //     b = Aufhellung (shine).
    readonly property var anims: ({
        "pop": { dur: 1600, e: [0.2, 0.8, 0.3, 1],
            y: [[0, -50], [0.25, -50], [0.55, -60], [1, -150]],
            s: [[0, 0.2], [0.25, 1.45], [0.55, 1.05], [1, 0.9]],
            o: [[0, 0], [0.25, 1], [1, 0]] },
        "fire": { dur: 1700, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.2, -55], [0.4, -70], [0.6, -90], [1, -150]],
            s: [[0, 0.3], [0.2, 1.4], [0.4, 1.2], [0.6, 1.35], [1, 1]],
            r: [[0.2, 0], [0.4, -4], [0.6, 4], [1, 0]],
            o: [[0, 0], [0.2, 1], [1, 0]] },
        "shake": { dur: 1700, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.15, -55], [0.25, -60], [0.35, -65], [0.45, -72],
                [0.55, -80], [0.7, -95], [0.85, -115], [1, -150]],
            s: [[0, 0.3], [0.15, 1.35]],
            r: [[0.15, 0], [0.25, -14], [0.35, 14], [0.45, -12], [0.55, 12],
                [0.7, -6], [0.85, 4], [1, 0]],
            o: [[0, 0], [0.15, 1], [1, 0]] },
        "beat": { dur: 1700, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.2, -55], [0.33, -60], [0.48, -70], [0.63, -82], [1, -150]],
            s: [[0, 0.2], [0.2, 1.5], [0.33, 1.05], [0.48, 1.4], [0.63, 1.1], [1, 1]],
            o: [[0, 0], [0.2, 1], [1, 0]] },
        "spin": { dur: 1600, e: [0.2, 0.7, 0.3, 1],
            y: [[0, -50], [0.25, -60], [1, -150]],
            s: [[0, 0.3], [0.25, 1.45], [1, 1]],
            r: [[0, 0], [0.25, 200], [1, 720]],
            o: [[0, 0], [0.25, 1], [1, 0]] },
        "shine": { dur: 1600, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.25, -58], [1, -150]],
            s: [[0, 0.3], [0.25, 1.45], [1, 1.05]],
            o: [[0, 0], [0.25, 1], [1, 0]],
            // CSS: brightness(2.2) + Glow zur Hälfte der Animation.
            b: [[0, 0], [0.25, 0], [0.5, 0.55], [1, 0]] },
        // Hinweis: Im Web-Client bleibt flex mit opacity 0 → 0 unsichtbar
        // (CSS-Fehler in rfxFlex). Hier die offensichtlich gemeinte Kurve.
        "flex": { dur: 1600, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.2, -55], [0.35, -60], [0.5, -70], [0.7, -90], [1, -150]],
            s: [[0, 0.3], [0.2, 1.5], [0.35, 1.1], [0.5, 1.45], [0.7, 1.15], [1, 1]],
            o: [[0, 0], [0.2, 1], [1, 0]] },
        "launch": { dur: 1400, e: [0.3, 0.1, 0.4, 1],
            y: [[0, -30], [0.2, -50], [1, -260]],
            s: [[0, 0.5], [0.2, 1.3], [1, 1]],
            r: [[0, -8], [1, -8]],
            o: [[0, 0], [0.2, 1], [1, 0]] },
        "drop": { dur: 1500, e: [0, 0, 0.58, 1],
            y: [[0, -170], [0.35, -50], [0.5, -72], [0.65, -50], [0.8, -58], [1, -70]],
            s: [[0, 0.7], [0.35, 1.25], [0.5, 1.1], [0.65, 1.2], [0.8, 1.15], [1, 1.1]],
            o: [[0, 0], [0.35, 1], [1, 0]] },
        "wobble": { dur: 1700, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.15, -55], [0.3, -62], [0.45, -70], [0.6, -80],
                [0.75, -95], [1, -150]],
            s: [[0, 0.3], [0.15, 1.35]],
            r: [[0, 0], [0.15, 16], [0.3, -14], [0.45, 11], [0.6, -8], [0.75, 5], [1, 0]],
            o: [[0, 0], [0.15, 1], [1, 0]] },
        "flip": { dur: 1600, e: [0.2, 0.7, 0.3, 1],
            y: [[0, -50], [0.25, -58], [1, -150]],
            s: [[0, 0.4], [0.25, 1.4], [1, 1]],
            ry: [[0, 0], [0.25, 360], [1, 900]],
            o: [[0, 0], [0.25, 1], [1, 0]] },
        "zoomout": { dur: 1600, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.2, -50], [0.45, -55], [0.6, -58], [1, -150]],
            s: [[0, 3], [0.2, 2.4], [0.45, 1.05], [0.6, 1.18], [1, 1]],
            o: [[0, 0], [0.2, 1], [1, 0]] },
        "heartbeat": { dur: 1700, e: [0, 0, 0.58, 1],
            y: [[0, -50], [0.12, -52], [0.22, -56], [0.32, -60], [0.42, -66],
                [0.58, -76], [0.72, -90], [1, -150]],
            s: [[0, 0.2], [0.12, 1.45], [0.22, 1.05], [0.32, 1.5], [0.42, 1.08],
                [0.58, 1.42], [0.72, 1.1], [1, 1]],
            o: [[0, 0], [0.12, 1], [1, 0]] },
        "shiver": { dur: 1600, e: [0, 0, 1, 1],
            x: [[0, -50], [0.12, -52], [0.24, -48], [0.36, -52], [0.48, -48],
                [0.6, -52], [0.72, -48], [0.84, -52], [1, -50]],
            y: [[0, -50], [0.12, -54], [0.24, -58], [0.36, -63], [0.48, -68],
                [0.6, -74], [0.72, -82], [0.84, -95], [1, -150]],
            s: [[0, 0.3], [0.12, 1.3], [0.6, 1.3], [0.72, 1.25], [0.84, 1.2], [1, 1.1]],
            o: [[0, 0], [0.12, 1], [1, 0]] },
        "tilt": { dur: 1600, e: [0.42, 0, 1, 1],
            y: [[0, -50], [0.2, -55], [0.45, -65], [1, -160]],
            s: [[0, 0.3], [0.2, 1.4], [0.45, 1.2], [1, 1]],
            r: [[0, 0], [0.2, 12], [0.45, 28], [1, 560]],
            o: [[0, 0], [0.2, 1], [1, 0]] },
        // 🔫: doppelter Rückstoß nach RECHTS (der Glyph zeigt nach links),
        // dazu ein leichtes Hochreißen der Mündung. Der seitliche Versatz ist
        // im Web-Client in Pixeln angegeben (14/4/10 px); geteilt durch
        // pxPerPercent ergeben sich die CSS-Prozente dieses Kanals.
        "recoil": { dur: 1500, e: [0, 0, 0.58, 1],
            x: [[0, -50], [0.12, -50], [0.2, -41.25], [0.34, -47.5],
                [0.42, -43.75], [0.56, -50], [1, -50]],
            y: [[0, -50], [0.12, -50], [0.2, -52], [0.34, -52], [0.42, -54],
                [0.56, -58], [1, -150]],
            s: [[0, 0.4], [0.12, 1.35], [0.2, 1.3], [0.34, 1.25], [0.42, 1.28],
                [0.56, 1.2], [1, 1]],
            r: [[0.12, 0], [0.2, 9], [0.34, 2], [0.42, 6], [0.56, 0], [1, 0]],
            o: [[0, 0], [0.12, 1], [1, 0]] }
    })

    // Effekt einer Reaktion; unbekannte Emojis bekommen den Standard.
    function fxFor(emoji) {
        return fx[emoji] || { a: "pop", p: "sparkle" }
    }

    function animFor(name) {
        return anims[name] || anims["pop"]
    }

    // Kubische Bézier-Timing-Function (wie CSS cubic-bezier): Newton-Iteration
    // auf x(t) = f, danach y(t).
    function ease(a, f) {
        var c = a.e || [0, 0, 1, 1]
        if (f <= 0) return 0
        if (f >= 1) return 1
        var cx = 3 * c[0], bx = 3 * (c[2] - c[0]) - cx, ax = 1 - cx - bx
        var cy = 3 * c[1], by = 3 * (c[3] - c[1]) - cy, ay = 1 - cy - by
        var t = f
        for (var i = 0; i < 8; i++) {
            var x = ((ax * t + bx) * t + cx) * t - f
            if (Math.abs(x) < 1e-5) break
            var d = (3 * ax * t + 2 * bx) * t + cx
            if (Math.abs(d) < 1e-6) break
            t -= x / d
        }
        return ((ay * t + by) * t + cy) * t
    }

    // Wert des Kanals ch der Choreografie a zum Zeitanteil t (0..1). Vor dem
    // ersten und nach dem letzten Keyframe gilt der jeweilige Randwert – so
    // hält CSS Eigenschaften, die nur am Anfang gesetzt sind.
    function sample(a, ch, t, fallback) {
        var kf = a[ch]
        if (!kf || kf.length === 0)
            return fallback
        if (t <= kf[0][0])
            return kf[0][1]
        for (var i = 1; i < kf.length; i++) {
            if (t <= kf[i][0]) {
                var span = kf[i][0] - kf[i - 1][0]
                var f = span > 0 ? (t - kf[i - 1][0]) / span : 1
                return kf[i - 1][1] + (kf[i][1] - kf[i - 1][1]) * ease(a, f)
            }
        }
        return kf[kf.length - 1][1]
    }
}
