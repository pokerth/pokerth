import QtQuick
import QtQuick.Effects

import "../config" as Config

// Reaction animation – a port of the web client choreography (playReactionFx):
// a large emoji appears at the player's seat, plays one of 16
// choreographies (rise, wobble, spin, fall …) and fades; plus a
// particle burst (sparks/confetti/drops/coins … depending on the emoji).
//
// The catalogue, the keyframes and the particle specifications live in
// config/ReactionCatalog.qml (shared with the ReactionPicker).
//
// Usage:  reactionFx.play("🎉", x, y)
//   (x, y) = anchor point in the coordinates of this item (centre/top edge of
//   the player box). Every reaction is a separate, self-destroying instance –
//   several simultaneous reactions are possible.
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

    // ── Create the particles of a specification/preset ──────────────────────
    // Return value: list of {kind, ch, color, w, h, size, ox, oy, dx, dy, g, rot,
    // pulse, life, delay}; kind = "glyph" (emoji), "dot" (coloured dot) or
    // "confetti". (ox, oy) is the start point, (dx, dy) the target – both
    // relative to the anchor point.
    function buildParticles(spec) {
        var delay = 0
        if (spec === "sparkle")
            spec = { chars: ["✦", "✧"], count: 7, color: "#E3C800", size: 12,
                     a0: 0, a1: 360, dist: 54, life: 700 }
        else if (spec === "shock")
            spec = { chars: ["💥", "✦"], count: 8, size: 15,
                     a0: 0, a1: 360, dist: 70, life: 800 }
        else if (spec === "boom") {
            // 💣: the bomb falls first ("drop"), it only explodes on
            // impact – hence the offset of 420 ms, which is stretched
            // along with the choreography (durationScale).
            spec = { chars: ["💥", "🔥", "✦"], count: 14, size: 18,
                     a0: 0, a1: 360, dist: 95, life: 950, rot: true }
            delay = catalog.scaled(420)
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
                    ox: 0, oy: 0,
                    dx: Math.cos(angC) * dC, dy: Math.sin(angC) * dC,
                    g: 130, rot: Math.random() * 720 - 360, pulse: false,
                    life: 1300 + Math.random() * 400, delay: 0
                })
            }
            return conf
        } else if (spec === "gunshot")
            return gunshotParticles()
        return spawn(spec, delay)
    }

    // Throw the particles of an explicit specification into the angle range
    // a0..a1.
    function spawn(spec, delay) {
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
                ox: 0, oy: 0,
                dx: Math.cos(ang) * d, dy: Math.sin(ang) * d,
                g: spec.g || 0,
                rot: spec.rot ? (Math.random() * 720 - 360) : 0,
                pulse: false,
                life: spec.life || 1000,
                delay: delay
            })
        }
        return pts
    }

    // 🔫 "gunshot": muzzle flash, a golden tracer projectile to the LEFT
    // (the glyph points left in all emoji fonts), its spark trail and
    // the cartridge ejected to the upper right. The emoji itself plays the
    // choreography "recoil" along with it.
    function gunshotParticles() {
        var pts = [
            // Muzzle flash: stays at the muzzle and flashes briefly.
            { kind: "glyph", ch: "💥", color: "#E3C800", w: 0, h: 0, size: 20,
              ox: -20, oy: 0, dx: -30, dy: 0, g: 0, rot: 0, pulse: true,
              life: 240, delay: 0 },
            // The projectile.
            { kind: "dot", ch: "", color: "#E3C800", w: 0, h: 0, size: 7,
              ox: -24, oy: 0, dx: -195, dy: 10, g: 0, rot: 0, pulse: false,
              life: 420, delay: 0 }
        ]
        return pts.concat(
            spawn({ chars: ["✦"], count: 5, size: 9, a0: 172, a1: 188,
                    dist: 120, life: 420 }, 0),
            spawn({ chars: ["✨"], count: 2, size: 10, a0: -110, a1: -60,
                    dist: 40, g: 55, life: 650 }, 0))
    }

    // Shock wave rings: a golden ring for the preset "shock", two wide
    // orange ones for "boom" (the second offset by 120 ms).
    function buildRings(spec) {
        if (spec === "shock")
            return [{ delay: 0, dur: 800, color: "#FFE066", width: 3, to: 4 }]
        if (spec === "boom")
            return [{ delay: catalog.scaled(420), dur: 900, color: "#ff9040", width: 4, to: 6.5 },
                    { delay: catalog.scaled(540), dur: 900, color: "#ff9040", width: 4, to: 6.5 }]
        return []
    }

    Component {
        id: burstComp

        Item {
            id: burst

            // Set on creation (see play): emoji, choreography,
            // particles and shock wave rings.
            property string emoji: ""
            property var anim: null
            property var particles: []
            property var rings: []

            readonly property real k: Config.ReactionCatalog.pxPerPercent
            readonly property int animDur: Config.ReactionCatalog.durationOf(anim)

            // Progress of the choreography (0..1, linear). The timing function
            // sits – as in CSS – in the evaluation of each keyframe section.
            property real prog

            // Lifetime = the longest sub-animation (emoji, particles, rings).
            readonly property int lifeMs: {
                var m = animDur
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
                duration: burst.animDur
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

            // ── Large emoji ──
            Text {
                id: bigEmoji
                text: burst.emoji
                // Colour emojis ignore color; if a glyph is rendered
                // monochrome (font fallback), it appears white instead of
                // black (the table background is dark green).
                color: "#FFFFFF"
                font.family: Config.StaticData.emojiFamily
                font.pixelSize: Config.ReactionCatalog.baseSize
                transformOrigin: Item.Center

                x: -width / 2 + (Config.ReactionCatalog.sample(burst.anim, "x", burst.prog, -50) + 50) * burst.k
                y: -height / 2 + (Config.ReactionCatalog.sample(burst.anim, "y", burst.prog, -50) + 50) * burst.k
                scale: Config.ReactionCatalog.sample(burst.anim, "s", burst.prog, 1)
                rotation: Config.ReactionCatalog.sample(burst.anim, "r", burst.prog, 0)
                opacity: Config.ReactionCatalog.sample(burst.anim, "o", burst.prog, 1)

                // Card turner ("flip"): rotation around the Y axis.
                transform: Rotation {
                    origin.x: bigEmoji.width / 2
                    origin.y: bigEmoji.height / 2
                    axis: Qt.vector3d(0, 1, 0)
                    angle: Config.ReactionCatalog.sample(burst.anim, "ry", burst.prog, 0)
                }

                // Flash of the "shine" choreography.
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
                    // Muzzle flash of the preset "gunshot": instead of the full
                    // opacity the character flashes up and fades again.
                    readonly property bool pulse: modelData.pulse === true
                    x: modelData.ox; y: modelData.oy
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
                    // … or a confetti rectangle
                    Rectangle {
                        visible: pt.modelData.kind === "confetti"
                        anchors.centerIn: parent
                        width: pt.modelData.w; height: pt.modelData.h
                        radius: 1
                        color: pt.modelData.color
                    }

                    // Flash: size 0.4 → 1.25 → 0.7 over the lifetime.
                    SequentialAnimation {
                        running: pt.pulse
                        PauseAnimation { duration: pt.modelData.delay }
                        NumberAnimation { target: pt; property: "scale"; from: 0.4; to: 1.25
                                          duration: pt.modelData.life * 0.35 }
                        NumberAnimation { target: pt; property: "scale"; to: 0.7
                                          duration: pt.modelData.life * 0.65 }
                    }

                    SequentialAnimation {
                        running: true
                        PauseAnimation { duration: pt.modelData.delay }
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
                                NumberAnimation { target: pt; property: "opacity"; to: 1
                                                  duration: pt.pulse ? pt.modelData.life * 0.35 : 0 }
                                PauseAnimation { duration: pt.pulse ? 0 : pt.modelData.life * 0.65 }
                                NumberAnimation { target: pt; property: "opacity"; to: 0
                                                  duration: pt.modelData.life * (pt.pulse ? 0.65 : 0.35) }
                            }
                        }
                    }
                }
            }

            // Self-destruction after all animations have finished.
            Timer {
                interval: burst.lifeMs
                running: true
                onTriggered: burst.destroy()
            }
        }
    }
}
