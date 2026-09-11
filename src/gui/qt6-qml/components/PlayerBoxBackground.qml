import QtQuick
import QtQuick.Effects

import "../config" as Config

// Card background of the player boxes: subtle gradient + soft drop shadow, so
// that the box looks like a raised card instead of a flat surface.
//
// A table style may tint the box via <PlayerBoxAccent> (e.g. the Star Trek
// style with the blue of the bridge consoles). The accent tints the gradient and the frame,
// but does not replace them – the box stays dark and the text on it readable. Without
// the tag the neutral, bundled look remains.
//
// The strengths apply to ALL styles together. With stronger values, warm
// accents have to be chosen carefully: orange on the blue-grey base tone
// quickly turns brownish.
Rectangle {
    id: boxBackground

    readonly property string accentName:
        (typeof StyleProvider !== "undefined" && StyleProvider)
        ? StyleProvider.playerBoxAccent : ""
    readonly property bool hasAccent: accentName !== ""
    readonly property color accent: hasAccent ? accentName : "transparent"

    // Base gradient of the neutral look.
    readonly property color baseTop: Qt.lighter("#394150", 1.18)
    readonly property color baseBottom: "#1d222b"

    function tinted(base, strength) {
        return boxBackground.hasAccent
               ? Qt.tint(base, Qt.rgba(boxBackground.accent.r, boxBackground.accent.g,
                                       boxBackground.accent.b, strength))
               : base
    }

    anchors.fill: parent
    radius: 6
    opacity: 0.9
    gradient: Gradient {
        GradientStop { position: 0.0; color: boxBackground.tinted(boxBackground.baseTop, 0.34) }
        GradientStop { position: 1.0; color: boxBackground.tinted(boxBackground.baseBottom, 0.22) }
    }
    border.color: boxBackground.hasAccent
                  ? Qt.rgba(boxBackground.accent.r, boxBackground.accent.g,
                            boxBackground.accent.b, 0.60)
                  : Qt.rgba(1, 1, 1, 0.06)
    border.width: 1

    layer.enabled: Config.Theme.effectsEnabled
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#000000"
        shadowOpacity: 0.42
        shadowBlur: 0.9
        shadowVerticalOffset: 3
        shadowHorizontalOffset: 0
    }
}
