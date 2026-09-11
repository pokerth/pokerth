import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Key figure tile "large value above a small label" of the player pages –
// header grid of the current season and expanded season results.
Rectangle {
    id: tile

    property string label: ""
    property string value: "–"

    Layout.fillWidth: true
    Layout.preferredHeight: 56
    radius: 6
    color: Config.StaticData.palette.secondary.col600

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 2
        AppLabel {
            Layout.alignment: Qt.AlignHCenter
            text: tile.value
            color: Config.StaticData.palette.secondary.col100
            font.pixelSize: Config.Theme.fontSizeTitle
            font.bold: true
        }
        AppLabel {
            Layout.alignment: Qt.AlignHCenter
            text: tile.label
            color: Config.StaticData.palette.secondary.col300
            font.pixelSize: Config.Theme.fontSizeCaption
        }
    }
}
