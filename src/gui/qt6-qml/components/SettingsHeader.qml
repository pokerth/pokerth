import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Sektions-Kopf einer Settings-Seite: fetter Titel + feine Trennlinie (harte
// 1px-Linie in col500). Ersetzt den in allen Settings-Komponenten wiederholten
// Label+Rectangle-Block. `topGap` deckt die zwei Varianten ab (8 bzw. 4); das
// innere `spacing: 5` reproduziert den Abstand der äußeren ColumnLayouts.
ColumnLayout {
    id: header
    property alias title: titleLabel.text
    property real topGap: 8

    Layout.fillWidth: true
    spacing: 5

    AppLabel {
        id: titleLabel
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: header.topGap
        Layout.bottomMargin: 0
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.fillHeight: false
        horizontalAlignment: Text.AlignLeft
        font.bold: true
        font.pointSize: 12
        color: Config.StaticData.palette.secondary.col200
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        Layout.fillHeight: false
        Layout.topMargin: 0
        Layout.bottomMargin: 4
        Layout.leftMargin: 12
        Layout.rightMargin: 12
        Layout.alignment: Qt.AlignTop
        color: Config.StaticData.palette.secondary.col500
    }
}
