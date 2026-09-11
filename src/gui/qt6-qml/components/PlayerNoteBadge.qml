import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Compact display of your own note/rating for a fellow player in the opponent
// box: "★3" for the rating and an "i" mark if a note is stored (like the
// tooltip label of the Qt widgets client). The opponent boxes are scaled down
// heavily at the table – five individual stars would be neither readable nor
// hittable there, hence star + number.
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

    // Note mark: small "i" like the hint label at the seat in the
    // Qt widgets client (gametableImpl::playerTipLabelArray).
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

        // Show the note text when hovering the mark (desktop). On touch a long press
        // on the box opens the dialog, which shows the note anyway.
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
