import QtQuick
import QtQuick.Effects

import "../config" as Config

// Karten-Hintergrund der Spielerboxen: dezenter Verlauf + weicher Schlagschatten,
// damit die Box als angehobene Karte statt als flache Fläche wirkt.
//
// Ein Tisch-Stil darf die Box über <PlayerBoxAccent> einfärben (z. B. der Star-
// Trek-Stil mit dem Blau der Brücken-Konsolen). Der Akzent tönt Verlauf und Rahmen, ersetzt
// sie aber nicht – die Box bleibt dunkel und der Text darauf lesbar. Ohne den
// Tag bleibt der neutrale, gebündelte Look.
//
// Die Stärken gelten für ALLE Stile gemeinsam. Bei kräftigeren Werten müssen
// warme Akzente vorsichtig gewählt werden: Orange auf dem blaugrauen Basiston
// kippt schnell ins Bräunliche.
Rectangle {
    id: boxBackground

    readonly property string accentName:
        (typeof StyleProvider !== "undefined" && StyleProvider)
        ? StyleProvider.playerBoxAccent : ""
    readonly property bool hasAccent: accentName !== ""
    readonly property color accent: hasAccent ? accentName : "transparent"

    // Basis-Verlauf des neutralen Looks.
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
