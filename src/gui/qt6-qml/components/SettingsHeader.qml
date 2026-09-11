import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Section header of a settings page: bold title + fine separator (hard 1px
// line in col500). Replaces the label+rectangle block repeated in all
// settings components. `topGap` covers the two variants (8 and 4); the inner
// `spacing: 5` reproduces the spacing of the outer ColumnLayouts.
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
