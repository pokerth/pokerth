import QtQuick
import QtQuick.Effects

import "../config" as Config

// Action-Timeout: schlanker Fortschrittsbalken (Track + animierte Füllung), der
// über GameTable.timeoutSec herunterzählt. `active` startet/stoppt die
// Animation; Größe und Sichtbarkeit setzt der Aufrufer.
Item {
    id: bar
    property bool active: false
    property color fillColor: Config.Theme.colorTimeout
    property real progress: 1.0

    // Track (statisch): Kontur + Dropshadow.
    Rectangle {
        anchors.fill: parent
        radius: height / 2
        color: Config.Theme.colorTimeoutTrack
        border.color: Qt.rgba(1, 1, 1, 0.55)
        border.width: 1
        layer.enabled: Config.Theme.effectsEnabled && bar.visible
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#000000"
            shadowOpacity: 0.6
            shadowBlur: 0.7
            shadowVerticalOffset: 1
            shadowHorizontalOffset: 0
        }
    }

    // Füllung (animiert) ÜBER dem Track – bewusst NICHT im Layer, damit die
    // Breiten-Animation zuverlässig läuft.
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 1
        height: parent.height - 2
        radius: height / 2
        color: bar.fillColor
        width: (parent.width - 2) * bar.progress
    }

    onActiveChanged: {
        if (active) {
            progress = 1.0
            timeoutAnim.restart()
        } else {
            timeoutAnim.stop()
        }
    }
    NumberAnimation {
        id: timeoutAnim
        target: bar
        property: "progress"
        from: 1.0; to: 0.0
        duration: ((typeof GameTable !== "undefined" && GameTable) ? GameTable.timeoutSec : 0) * 1000
        easing.type: Easing.Linear
    }
}
