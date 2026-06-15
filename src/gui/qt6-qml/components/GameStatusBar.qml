import QtQuick
import QtQuick.Layouts

import "../config" as Config

// Status-Leiste oben am Tisch: links die Pott-Info (Total/Bets), rechts
// Spielphase, Game-ID und Hand-Nummer – analog zum Qt-Widgets-Client neben
// den Community-Cards. Höhe/Layout werden vom Aufrufer über Layout.* gesetzt.
Rectangle {
    color: Qt.rgba(0, 0, 0, 0.78)

    RowLayout {
        anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
        spacing: 0

        // Links: Pot-Info (1:1 wie Widget-Client links neben den Community-Cards)
        // "Total" = aufgelaufener Pot (getPot), "Bets" = laufende Einsätze dieser Runde (getSets)
        Column {
            spacing: 0
            Row {
                spacing: 4
                Text {
                    text: qsTr("Total:")
                    color: "#9e9e9e"
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                    font.weight: Font.Medium
                }
                Text {
                    text: "$%1".arg(GameTable ? GameTable.pot : 0)
                    color: "#99D500"
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                    font.bold: true
                }
            }
            Row {
                spacing: 4
                Text {
                    text: qsTr("Bets:")
                    color: "#9e9e9e"
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Responsive.landscapeCompact ? 10 : 11
                    font.weight: Font.Medium
                }
                Text {
                    text: "$%1".arg(GameTable ? (GameTable.totalPot - GameTable.pot) : 0)
                    color: "#7aa800"
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Responsive.landscapeCompact ? 10 : 11
                    font.weight: Font.Medium
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Rechts: Phase + Game-ID + Hand-Nummer (1:1 wie Widget-Client rechts neben den Community-Cards)
        Column {
            spacing: 0
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: GameTable ? GameTable.phaseText : qsTr("Preflop")
                color: "#FFFFFF"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                font.weight: Font.DemiBold
                font.letterSpacing: 0.5
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8
                Text {
                    text: qsTr("Game: %1").arg(GameTable ? GameTable.gameId : 0)
                    color: "#9e9e9e"
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Responsive.landscapeCompact ? 9 : 11
                    font.weight: Font.Medium
                }
                Text {
                    text: qsTr("Hand: %1").arg(GameTable ? GameTable.handNumber : 1)
                    color: "#9e9e9e"
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Config.Responsive.landscapeCompact ? 9 : 11
                    font.weight: Font.Medium
                }
            }
        }
    }
}
