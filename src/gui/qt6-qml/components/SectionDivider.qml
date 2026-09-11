import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Fine separator line below section headings that fades out towards the
// edges ("Game List", "Lobby Chat" …). Replaces the hard 1px edge with a
// subtle horizontal gradient that is strongest in the middle.
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
