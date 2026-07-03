import QtQuick
import QtQuick.Effects

import "../config" as Config

// Karten-Hintergrund der Spielerboxen: dezenter Verlauf + weicher Schlagschatten,
// damit die Box als angehobene Karte statt als flache Fläche wirkt.
Rectangle {
    anchors.fill: parent
    radius: 6
    opacity: 0.9
    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.lighter("#394150", 1.18) }
        GradientStop { position: 1.0; color: "#1d222b" }
    }
    border.color: Qt.rgba(1, 1, 1, 0.06)
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
