import QtQuick
import QtQuick.Effects

import "../config" as Config

// Gold-Highlight für den Spieler am Zug: weicher Außen-Glow (gelayert, optional)
// + immer sichtbarer Gold-Rahmen, mit ruhigem Puls. Der Rahmen liegt bewusst als
// eigene Ebene OHNE Layer vor, damit er sichtbar bleibt, selbst wenn der
// MultiEffect-Glow auf einem System nicht rendert. `active` schaltet ein.
Item {
    id: glow
    property bool active: false
    property int borderWidth: 1

    anchors.fill: parent
    anchors.margins: -2
    z: 10
    visible: active

    // Puls nur bei aktivierten Effekten – sonst läuft eine Endlos-Animation, die
    // die GESAMTE Szene mit 60 fps neu zeichnen lässt.
    SequentialAnimation on opacity {
        running: Config.Theme.effectsEnabled && glow.active
        loops: Animation.Infinite
        NumberAnimation { from: 0.65; to: 1.0; duration: 750; easing.type: Easing.InOutSine }
        NumberAnimation { from: 1.0; to: 0.65; duration: 750; easing.type: Easing.InOutSine }
    }

    // Weicher Außen-Glow (gelayert) – reine Eye-Candy, optional.
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 6
        border.color: "#FFD54A"
        border.width: glow.borderWidth
        layer.enabled: Config.Theme.effectsEnabled && glow.active
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#FFD700"
            shadowOpacity: 0.9
            shadowBlur: 1.0
            shadowVerticalOffset: 0
            shadowHorizontalOffset: 0
        }
    }

    // Gold-Rahmen (immer sichtbar, KEIN Layer).
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 6
        border.color: "#CCFFD54A"
        border.width: glow.borderWidth
    }
}
