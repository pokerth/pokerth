import QtQuick

import "../config" as Config

// Reaktions-Animation – Port der Web-Client-Choreografie (playReactionFx):
// ein großes Emoji poppt am Sitz des Spielers auf, steigt mit leichtem
// Wackeln/Drehen nach oben und verblasst; dazu ein Partikel-Burst
// (Funken/Konfetti/Tropfen/Münzen … je nach Emoji).
//
// Verwendung:  reactionFx.play("🎉", x, y)
//   (x, y) = Ankerpunkt in Koordinaten dieses Items (Mitte/Oberkante der
//   Spielerbox). Jede Reaktion ist eine eigene, selbstzerstörende Instanz –
//   mehrere gleichzeitige Reaktionen sind möglich.
Item {
    id: root

    // Effekt-Katalog je Emoji (vereinfachter Port von REACTION_FX):
    //   a: "pop" | "shake" | "spin"  – Bewegungs-Stil des großen Emojis
    //   p: Partikel-Spez oder Preset "sparkle" | "confetti" | "shock"
    //      {chars, count, size, a0, a1, dist, g, life, rot}
    //      a0..a1 = Winkelbereich (Grad, 0=rechts, -90=oben), dist = Wurfweite,
    //      g = zusätzlicher Fall am Ende, life = Lebensdauer ms.
    readonly property var fxCatalog: ({
        "🎉": { a: "pop",   p: "confetti" },
        "🥳": { a: "pop",   p: "confetti" },
        "🎊": { a: "pop",   p: "confetti" },
        "🔥": { a: "shake", p: { chars: ["🔥", "✦"], count: 9, size: 14, a0: -150, a1: -30, dist: 70, g: -24, life: 1000, rot: true } },
        "💰": { a: "pop",   p: { chars: ["🪙", "💵", "✦"], count: 12, size: 16, a0: -170, a1: -10, dist: 72, g: 90, life: 1200, rot: true } },
        "🤑": { a: "pop",   p: { chars: ["🪙", "💵"], count: 10, size: 16, a0: -170, a1: -10, dist: 70, g: 90, life: 1100, rot: true } },
        "💎": { a: "pop",   p: { chars: ["✨", "✦"], count: 9, size: 13, a0: 0, a1: 360, dist: 64, life: 850, rot: true } },
        "🤩": { a: "pop",   p: { chars: ["✨"], count: 8, size: 13, a0: 0, a1: 360, dist: 60, life: 800, rot: true } },
        "😂": { a: "shake", p: { chars: ["💧"], count: 7, size: 13, a0: -30, a1: 210, dist: 55, g: 36, life: 850 } },
        "🤣": { a: "shake", p: { chars: ["💧"], count: 7, size: 13, a0: -30, a1: 210, dist: 55, g: 36, life: 850 } },
        "👏": { a: "pop",   p: "sparkle" },
        "🙌": { a: "pop",   p: "sparkle" },
        "💪": { a: "pop",   p: "sparkle" },
        "😱": { a: "shake", p: { chars: ["💦"], count: 6, size: 12, a0: -120, a1: -60, dist: 48, g: 50, life: 780 } },
        "🤯": { a: "pop",   p: "shock" },
        "🍀": { a: "spin",  p: { chars: ["✨", "🍀"], count: 8, size: 13, a0: 0, a1: 360, dist: 62, life: 950, rot: true } },
        "🎰": { a: "spin",  p: { chars: ["✨", "🪙"], count: 9, size: 14, a0: 0, a1: 360, dist: 66, life: 950, rot: true } },
        "👑": { a: "pop",   p: { chars: ["✨", "⭐"], count: 10, size: 14, a0: 0, a1: 360, dist: 70, life: 1000, rot: true } },
        "😍": { a: "pop",   p: { chars: ["❤️", "💖"], count: 8, size: 16, a0: -160, a1: -20, dist: 64, g: -30, life: 1100 } }
    })

    function play(emoji, x, y) {
        var fx = fxCatalog[emoji] || { a: "pop", p: "sparkle" }
        burstComp.createObject(root, { emoji: emoji, anim: fx.a, pSpec: fx.p, x: x, y: y })
    }

    Component {
        id: burstComp

        Item {
            id: burst
            property string emoji: ""
            property string anim: "pop"
            property var pSpec: "sparkle"

            width: 0; height: 0
            z: 1

            // Partikel-Liste aus der Spez generieren (einmalig beim Erzeugen).
            property var particles: {
                var spec = pSpec
                if (spec === "sparkle")
                    spec = { chars: ["✦", "✧"], count: 7, color: "#E3C800", size: 12, a0: 0, a1: 360, dist: 54, life: 700 }
                else if (spec === "shock")
                    spec = { chars: ["💥", "✦"], count: 8, size: 15, a0: 0, a1: 360, dist: 70, life: 800 }
                else if (spec === "confetti") {
                    var cols = ["#9b59b6", "#e84393", "#27ae60", "#c0392b", "#7ec8e3", "#e67e22", "#ffffff"]
                    var conf = []
                    for (var c = 0; c < 24; c++) {
                        var angC = (-170 + Math.random() * 160) * Math.PI / 180
                        var dC = 70 + Math.random() * 60
                        conf.push({
                            ch: "", color: cols[Math.floor(Math.random() * cols.length)],
                            w: 5 + Math.random() * 4, h: 7 + Math.random() * 4,
                            size: 0,
                            dx: Math.cos(angC) * dC, dy: Math.sin(angC) * dC,
                            g: 130, rot: Math.random() * 720 - 360,
                            life: 1300 + Math.random() * 400
                        })
                    }
                    return conf
                }
                if (!spec || typeof spec !== "object") return []
                var pts = []
                for (var i = 0; i < spec.count; i++) {
                    var ang = (spec.a0 + Math.random() * (spec.a1 - spec.a0)) * Math.PI / 180
                    var d = spec.dist * (0.55 + Math.random() * 0.6)
                    pts.push({
                        ch: spec.chars[Math.floor(Math.random() * spec.chars.length)],
                        color: spec.color || "#E3C800",
                        w: 0, h: 0,
                        size: spec.size || 14,
                        dx: Math.cos(ang) * d, dy: Math.sin(ang) * d,
                        g: spec.g || 0,
                        rot: spec.rot ? (Math.random() * 720 - 360) : 0,
                        life: spec.life || 1000
                    })
                }
                return pts
            }

            // Druckwellen-Ring (nur "shock"-Preset, z. B. 🤯).
            Rectangle {
                visible: burst.pSpec === "shock"
                anchors.centerIn: parent
                width: 30; height: 30; radius: 15
                color: "transparent"
                border.color: "#FFE066"
                border.width: 3
                scale: 0.3
                opacity: 0.9
                ParallelAnimation {
                    running: burst.pSpec === "shock"
                    NumberAnimation { target: parent; property: "scale"; to: 4; duration: 800; easing.type: Easing.OutQuad }
                    NumberAnimation { target: parent; property: "opacity"; to: 0; duration: 800 }
                }
            }

            // ── Großes Emoji: Pop-in, Aufstieg, Wobble/Spin, Fade-out ──
            Text {
                id: bigEmoji
                text: burst.emoji
                // Farb-Emojis ignorieren color; falls ein Glyph monochrom
                // gerendert wird (Font-Fallback), erscheint er weiß statt
                // schwarz (Tisch-Hintergrund ist dunkelgrün).
                color: "#FFFFFF"
                font.family: Config.StaticData.emojiFamily
                font.pixelSize: 34
                x: -width / 2
                y: -height / 2
                transformOrigin: Item.Center
                scale: 0.2
                opacity: 0

                ParallelAnimation {
                    running: true
                    // Aufstieg (Bahn der floatReaction-Keyframes)
                    SequentialAnimation {
                        NumberAnimation { target: bigEmoji; property: "y"; to: -bigEmoji.height / 2 - 22; duration: 330; easing.type: Easing.OutQuad }
                        NumberAnimation { target: bigEmoji; property: "y"; to: -bigEmoji.height / 2 - 110; duration: 1100; easing.type: Easing.InOutQuad }
                        NumberAnimation { target: bigEmoji; property: "y"; to: -bigEmoji.height / 2 - 160; duration: 570; easing.type: Easing.InQuad }
                    }
                    // Pop-Skalierung
                    SequentialAnimation {
                        NumberAnimation { target: bigEmoji; property: "scale"; to: 1.45; duration: 330; easing.type: Easing.OutBack }
                        NumberAnimation { target: bigEmoji; property: "scale"; to: 1.05; duration: 450; easing.type: Easing.OutQuad }
                        NumberAnimation { target: bigEmoji; property: "scale"; to: 0.9; duration: 1220 }
                    }
                    // Ein-/Ausblenden
                    SequentialAnimation {
                        NumberAnimation { target: bigEmoji; property: "opacity"; to: 1; duration: 250 }
                        PauseAnimation { duration: 1150 }
                        NumberAnimation { target: bigEmoji; property: "opacity"; to: 0; duration: 600 }
                    }
                }
                // Bewegungs-Stil: leichtes Pendeln (pop), heftiges Wackeln
                // (shake) oder volle Drehung (spin).
                SequentialAnimation {
                    running: burst.anim === "pop"
                    NumberAnimation { target: bigEmoji; property: "rotation"; from: -8; to: 5; duration: 500; easing.type: Easing.InOutSine }
                    NumberAnimation { target: bigEmoji; property: "rotation"; to: -3; duration: 800; easing.type: Easing.InOutSine }
                    NumberAnimation { target: bigEmoji; property: "rotation"; to: 8; duration: 700; easing.type: Easing.InOutSine }
                }
                SequentialAnimation {
                    running: burst.anim === "shake"
                    loops: 3
                    NumberAnimation { target: bigEmoji; property: "rotation"; to: -14; duration: 110; easing.type: Easing.InOutSine }
                    NumberAnimation { target: bigEmoji; property: "rotation"; to: 14; duration: 110; easing.type: Easing.InOutSine }
                }
                NumberAnimation {
                    running: burst.anim === "spin"
                    target: bigEmoji; property: "rotation"; from: 0; to: 720
                    duration: 1800; easing.type: Easing.OutQuad
                }
            }

            // ── Partikel-Burst ──
            Repeater {
                model: burst.particles
                delegate: Item {
                    id: pt
                    required property var modelData
                    x: 0; y: 0
                    opacity: 1

                    // Emoji-/Zeichen-Partikel …
                    Text {
                        visible: pt.modelData.ch !== ""
                        anchors.centerIn: parent
                        text: pt.modelData.ch
                        color: pt.modelData.color
                        font.family: Config.StaticData.emojiFamily
                        font.pixelSize: Math.max(8, pt.modelData.size)
                    }
                    // … oder Konfetti-Rechteck
                    Rectangle {
                        visible: pt.modelData.ch === ""
                        anchors.centerIn: parent
                        width: pt.modelData.w; height: pt.modelData.h
                        radius: 1
                        color: pt.modelData.color
                    }

                    ParallelAnimation {
                        running: true
                        NumberAnimation {
                            target: pt; property: "x"; to: pt.modelData.dx
                            duration: pt.modelData.life * 0.65; easing.type: Easing.OutCubic
                        }
                        SequentialAnimation {
                            NumberAnimation {
                                target: pt; property: "y"; to: pt.modelData.dy
                                duration: pt.modelData.life * 0.65; easing.type: Easing.OutCubic
                            }
                            NumberAnimation {
                                target: pt; property: "y"; to: pt.modelData.dy + pt.modelData.g
                                duration: pt.modelData.life * 0.35; easing.type: Easing.InQuad
                            }
                        }
                        NumberAnimation {
                            target: pt; property: "rotation"; to: pt.modelData.rot
                            duration: pt.modelData.life
                        }
                        SequentialAnimation {
                            PauseAnimation { duration: pt.modelData.life * 0.65 }
                            NumberAnimation { target: pt; property: "opacity"; to: 0; duration: pt.modelData.life * 0.35 }
                        }
                    }
                }
            }

            // Selbstzerstörung nach Ablauf aller Animationen.
            Timer {
                interval: 2400
                running: true
                onTriggered: burst.destroy()
            }
        }
    }
}
