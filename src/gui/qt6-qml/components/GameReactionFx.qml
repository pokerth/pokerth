import QtQuick
import QtQuick.Effects

import "../config" as Config

// Reaktions-Animation – Port der Web-Client-Choreografie (playReactionFx):
// ein großes Emoji erscheint am Sitz des Spielers, spielt eine von 15
// Choreografien (Aufstieg, Wackeln, Drehen, Fallen …) und verblasst; dazu ein
// Partikel-Burst (Funken/Konfetti/Tropfen/Münzen … je nach Emoji).
//
// Katalog, Keyframes und Partikel-Spezifikationen stehen in
// config/ReactionCatalog.qml (gemeinsam mit dem ReactionPicker).
//
// Verwendung:  reactionFx.play("🎉", x, y)
//   (x, y) = Ankerpunkt in Koordinaten dieses Items (Mitte/Oberkante der
//   Spielerbox). Jede Reaktion ist eine eigene, selbstzerstörende Instanz –
//   mehrere gleichzeitige Reaktionen sind möglich.
Item {
    id: root

    readonly property var catalog: Config.ReactionCatalog

    function play(emoji, x, y) {
        var fx = catalog.fxFor(emoji)
        burstComp.createObject(root, {
            emoji: emoji, x: x, y: y,
            anim: catalog.animFor(fx.a),
            particles: buildParticles(fx.p),
            rings: buildRings(fx.p)
        })
    }

    // ── Partikel einer Spezifikation/eines Presets erzeugen ─────────────────
    // Rückgabe: Liste von {kind, ch, color, w, h, size, dx, dy, g, rot, life,
    // delay}; kind = "glyph" (Emoji), "dot" (farbiger Punkt) oder "confetti".
    function buildParticles(spec) {
        var delay = 0
        if (spec === "sparkle")
            spec = { chars: ["✦", "✧"], count: 7, color: "#E3C800", size: 12,
                     a0: 0, a1: 360, dist: 54, life: 700 }
        else if (spec === "shock")
            spec = { chars: ["💥", "✦"], count: 8, size: 15,
                     a0: 0, a1: 360, dist: 70, life: 800 }
        else if (spec === "boom") {
            // 💣: Die Bombe fällt zuerst ("drop"), erst beim Aufschlag
            // explodiert sie – daher der Versatz von 420 ms.
            spec = { chars: ["💥", "🔥", "✦"], count: 14, size: 18,
                     a0: 0, a1: 360, dist: 95, life: 950, rot: true }
            delay = 420
        } else if (spec === "confetti") {
            var cols = ["#9b59b6", "#e84393", "#27ae60", "#c0392b", "#7ec8e3", "#e67e22", "#ffffff"]
            var conf = []
            for (var c = 0; c < 24; c++) {
                var angC = (-170 + Math.random() * 160) * Math.PI / 180
                var dC = 70 + Math.random() * 60
                conf.push({
                    kind: "confetti", ch: "",
                    color: cols[Math.floor(Math.random() * cols.length)],
                    w: 5 + Math.random() * 4, h: 7 + Math.random() * 4, size: 0,
                    dx: Math.cos(angC) * dC, dy: Math.sin(angC) * dC,
                    g: 130, rot: Math.random() * 720 - 360,
                    life: 1300 + Math.random() * 400, delay: 0
                })
            }
            return conf
        }
        if (!spec || typeof spec !== "object")
            return []
        var pts = []
        for (var i = 0; i < spec.count; i++) {
            var ang = (spec.a0 + Math.random() * (spec.a1 - spec.a0)) * Math.PI / 180
            var d = spec.dist * (0.55 + Math.random() * 0.6)
            pts.push({
                kind: spec.chars ? "glyph" : "dot",
                ch: spec.chars ? spec.chars[Math.floor(Math.random() * spec.chars.length)] : "",
                color: spec.color || "#E3C800",
                w: 0, h: 0,
                size: spec.size || (spec.chars ? 14 : 7),
                dx: Math.cos(ang) * d, dy: Math.sin(ang) * d,
                g: spec.g || 0,
                rot: spec.rot ? (Math.random() * 720 - 360) : 0,
                life: spec.life || 1000,
                delay: delay
            })
        }
        return pts
    }

    // Druckwellen-Ringe: goldener Ring beim Preset "shock", zwei breite
    // orangene beim "boom" (der zweite 120 ms versetzt).
    function buildRings(spec) {
        if (spec === "shock")
            return [{ delay: 0, dur: 800, color: "#FFE066", width: 3, to: 4 }]
        if (spec === "boom")
            return [{ delay: 420, dur: 900, color: "#ff9040", width: 4, to: 6.5 },
                    { delay: 540, dur: 900, color: "#ff9040", width: 4, to: 6.5 }]
        return []
    }

    Component {
        id: burstComp

        Item {
            id: burst

            // Beim Erzeugen gesetzt (siehe play): Emoji, Choreografie,
            // Partikel und Druckwellen-Ringe.
            property string emoji: ""
            property var anim: null
            property var particles: []
            property var rings: []

            readonly property real k: Config.ReactionCatalog.pxPerPercent

            // Fortschritt der Choreografie (0..1, linear). Die Timing-Function
            // steckt – wie in CSS – in der Auswertung jedes Keyframe-Abschnitts.
            property real prog

            // Lebensdauer = längste Teilanimation (Emoji, Partikel, Ringe).
            readonly property int lifeMs: {
                var m = anim.dur
                for (var i = 0; i < particles.length; i++)
                    m = Math.max(m, particles[i].delay + particles[i].life)
                for (var r = 0; r < rings.length; r++)
                    m = Math.max(m, rings[r].delay + rings[r].dur)
                return m + 150
            }

            width: 0; height: 0
            z: 1

            NumberAnimation on prog {
                from: 0; to: 1
                duration: burst.anim.dur
                running: true
            }

            // ── Druckwellen-Ringe ──
            Repeater {
                model: burst.rings
                delegate: Rectangle {
                    id: ring
                    required property var modelData
                    anchors.centerIn: parent
                    width: 30; height: 30; radius: 15
                    color: "transparent"
                    border.color: modelData.color
                    border.width: modelData.width
                    scale: 0.3
                    opacity: 0

                    SequentialAnimation {
                        running: true
                        PauseAnimation { duration: ring.modelData.delay }
                        PropertyAction { target: ring; property: "opacity"; value: 0.9 }
                        ParallelAnimation {
                            NumberAnimation { target: ring; property: "scale"; to: ring.modelData.to
                                              duration: ring.modelData.dur; easing.type: Easing.OutQuad }
                            NumberAnimation { target: ring; property: "opacity"; to: 0
                                              duration: ring.modelData.dur }
                        }
                    }
                }
            }

            // ── Großes Emoji ──
            Text {
                id: bigEmoji
                text: burst.emoji
                // Farb-Emojis ignorieren color; falls ein Glyph monochrom
                // gerendert wird (Font-Fallback), erscheint er weiß statt
                // schwarz (Tisch-Hintergrund ist dunkelgrün).
                color: "#FFFFFF"
                font.family: Config.StaticData.emojiFamily
                font.pixelSize: Config.ReactionCatalog.baseSize
                transformOrigin: Item.Center

                x: -width / 2 + (Config.ReactionCatalog.sample(burst.anim, "x", burst.prog, -50) + 50) * burst.k
                y: -height / 2 + (Config.ReactionCatalog.sample(burst.anim, "y", burst.prog, -50) + 50) * burst.k
                scale: Config.ReactionCatalog.sample(burst.anim, "s", burst.prog, 1)
                rotation: Config.ReactionCatalog.sample(burst.anim, "r", burst.prog, 0)
                opacity: Config.ReactionCatalog.sample(burst.anim, "o", burst.prog, 1)

                // Kartendreher ("flip"): Drehung um die Y-Achse.
                transform: Rotation {
                    origin.x: bigEmoji.width / 2
                    origin.y: bigEmoji.height / 2
                    axis: Qt.vector3d(0, 1, 0)
                    angle: Config.ReactionCatalog.sample(burst.anim, "ry", burst.prog, 0)
                }

                // Aufblitzen der "shine"-Choreografie.
                layer.enabled: burst.anim.b !== undefined
                layer.effect: MultiEffect {
                    brightness: Config.ReactionCatalog.sample(burst.anim, "b", burst.prog, 0)
                }
            }

            // ── Partikel-Burst ──
            Repeater {
                model: burst.particles
                delegate: Item {
                    id: pt
                    required property var modelData
                    x: 0; y: 0
                    opacity: 0

                    // Emoji-/Zeichen-Partikel …
                    Text {
                        visible: pt.modelData.kind === "glyph"
                        anchors.centerIn: parent
                        text: pt.modelData.ch
                        color: pt.modelData.color
                        font.family: Config.StaticData.emojiFamily
                        font.pixelSize: Math.max(8, pt.modelData.size)
                    }
                    // … farbiger Punkt …
                    Rectangle {
                        visible: pt.modelData.kind === "dot"
                        anchors.centerIn: parent
                        width: pt.modelData.size; height: pt.modelData.size
                        radius: width / 2
                        color: pt.modelData.color
                    }
                    // … oder Konfetti-Rechteck
                    Rectangle {
                        visible: pt.modelData.kind === "confetti"
                        anchors.centerIn: parent
                        width: pt.modelData.w; height: pt.modelData.h
                        radius: 1
                        color: pt.modelData.color
                    }

                    SequentialAnimation {
                        running: true
                        PauseAnimation { duration: pt.modelData.delay }
                        PropertyAction { target: pt; property: "opacity"; value: 1 }
                        ParallelAnimation {
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
                                NumberAnimation { target: pt; property: "opacity"; to: 0
                                                  duration: pt.modelData.life * 0.35 }
                            }
                        }
                    }
                }
            }

            // Selbstzerstörung nach Ablauf aller Animationen.
            Timer {
                interval: burst.lifeMs
                running: true
                onTriggered: burst.destroy()
            }
        }
    }
}
