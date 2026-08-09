import QtQuick
import QtQuick.Controls

import "../config" as Config

// Markiert den Tisch-Admin (Ersteller/Host eines Spiels) in den Spielerlisten
// von Lobby, Warteraum und aufgeklappter Spielliste. Pendant zur grün
// hinterlegten Zeile im Widget-Client (gameLobbyDialogImpl::newGameAdmin).
// Sichtbarkeit setzt der Aufrufer (modelData.isGameAdmin).
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
