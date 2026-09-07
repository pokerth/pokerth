import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Kompakte Anzeige der eigenen Notiz/Bewertung zu einem Mitspieler in der
// Gegnerbox: "★3" für die Bewertung und eine "i"-Marke, wenn eine Notiz
// hinterlegt ist (wie das Tooltip-Label des Qt-Widgets-Clients). Die
// Gegnerboxen sind am Tisch stark skaliert – fünf einzelne Sterne wären dort
// weder lesbar noch treffbar, deshalb Stern + Zahl.
RowLayout {
    id: root

    property int rating: 0
    property string note: ""
    property int glyphSize: 12

    readonly property bool hasNote: note !== ""
    visible: rating > 0 || hasNote
    spacing: 3

    AppText {
        Layout.alignment: Qt.AlignVCenter
        visible: root.rating > 0
        text: "★" + root.rating
        color: Config.Theme.colorAccent
        font.pixelSize: root.glyphSize
        font.bold: true
    }

    // Notiz-Marke: kleines "i" wie das Hinweis-Label am Sitz im
    // Qt-Widgets-Client (gametableImpl::playerTipLabelArray).
    Rectangle {
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: Math.round(root.glyphSize * 0.9)
        Layout.preferredHeight: Layout.preferredWidth
        visible: root.hasNote
        radius: 2
        color: Config.StaticData.palette.secondary.col600
        border.width: 1
        border.color: Config.Theme.colorAccent

        AppText {
            anchors.centerIn: parent
            text: "i"
            color: Config.Theme.colorAccent
            font.pixelSize: Math.round(root.glyphSize * 0.8)
            font.bold: true
            font.family: "serif"
        }

        HoverHandler { id: badgeHover }

        // Notiztext beim Überfahren der Marke zeigen (Desktop). Auf Touch führt
        // der lange Druck auf die Box zum Dialog, der die Notiz ohnehin anzeigt.
        ToolTip {
            visible: badgeHover.hovered && root.hasNote
            delay: 300
            y: -implicitHeight - 6

            contentItem: AppText {
                text: root.note
                color: Config.Theme.colorTextPrimary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                width: Math.min(implicitWidth, 260)
            }
            background: Rectangle {
                color: Config.Theme.colorBox
                border.width: 1
                border.color: Config.StaticData.palette.secondary.col500
                radius: Config.Theme.radiusSmall
            }
        }
    }
}
