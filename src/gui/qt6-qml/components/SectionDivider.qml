import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Feine, zu den Rändern hin auslaufende Trennlinie unter Sektions-Überschriften
// ("Game List", "Lobby Chat" …). Ersetzt die harte 1px-Kante durch einen
// dezenten horizontalen Verlauf, der mittig am kräftigsten ist.
Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 1
    implicitHeight: 1

    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: "transparent" }
        GradientStop { position: 0.5; color: Config.StaticData.palette.secondary.col500 }
        GradientStop { position: 1.0; color: "transparent" }
    }
}
