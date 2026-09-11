import QtQuick
import QtQuick.Controls

import "../config" as Config

// Marks the table admin (creator/host of a game) in the player lists of
// the lobby, the waiting room and the expanded game list. Counterpart to
// the green row in the widget client (gameLobbyDialogImpl::newGameAdmin).
// Visibility is set by the caller (modelData.isGameAdmin).
Rectangle {
    id: badge

    readonly property color adminColor: Config.Theme.colorGameAdmin

    implicitWidth: badgeText.implicitWidth + 12
    implicitHeight: 16
    radius: 8
    color: Qt.rgba(adminColor.r, adminColor.g, adminColor.b, 0.22)
    border.width: 1
    border.color: Qt.rgba(adminColor.r, adminColor.g, adminColor.b, 0.7)

    AppText {
        id: badgeText
        anchors.centerIn: parent
        text: qsTr("Admin")
        font.pixelSize: 10
        font.bold: true
        color: badge.adminColor
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    ToolTip.text: qsTr("Game admin: starts the game and can kick players")
    ToolTip.visible: hoverArea.containsMouse && Config.Parameters.showTooltips
    ToolTip.delay: 400
}
