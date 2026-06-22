import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Gemeinsame Statusleiste (ganz unten): Anzahl verbundener Spieler sowie
// laufende/offene Spiele links, PokerTH.net-Link rechts. Liest die Werte
// selbst aus dem Lobby-Handler, damit sie auf jeder Seite identisch ist.
RowLayout {
    id: statsBar
    Layout.fillWidth: true
    spacing: 6

    readonly property int connectedPlayers: Lobby ? Lobby.playerListModel.count : 0
    readonly property int runningGames: Lobby ? Lobby.gameListModel.runningCount : 0
    readonly property int openGames: Lobby ? Lobby.gameListModel.openCount : 0

    // Compact: eine Zeile mit Kurzform + elide
    Label {
        visible: Config.Responsive.compact
        Layout.fillWidth: true
        text: qsTr("%1 players · %2 running · %3 open")
              .arg(statsBar.connectedPlayers).arg(statsBar.runningGames).arg(statsBar.openGames)
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: 12
        color: Config.StaticData.palette.secondary.col300
        elide: Text.ElideRight
    }

    // Wide: einzelne Labels
    Label {
        visible: !Config.Responsive.compact
        text: qsTr("connected players: %1").arg(statsBar.connectedPlayers)
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: 12
        color: Config.StaticData.palette.secondary.col300
    }
    Label {
        visible: !Config.Responsive.compact
        text: " | " + qsTr("running games: %1").arg(statsBar.runningGames)
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: 12
        color: Config.StaticData.palette.secondary.col300
    }
    Label {
        visible: !Config.Responsive.compact
        text: " | " + qsTr("open games: %1").arg(statsBar.openGames)
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: 12
        color: Config.StaticData.palette.secondary.col300
    }

    Item { Layout.fillWidth: true }

    Text {
        text: qsTr("PokerTH.net")
        font.family: Config.StaticData.loadedFont.font.family
        font.pixelSize: 12
        color: (Config.StaticData.palette.primary && Config.StaticData.palette.primary.col400)
               ? Config.StaticData.palette.primary.col400
               : Config.StaticData.palette.secondary.col300
        font.underline: true

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                var url = "https://www.pokerth.net"
                var opened = false
                if (Lobby) {
                    opened = Lobby.openExternalUrl(url)
                } else {
                    opened = Qt.openUrlExternally(url)
                }

                if (!opened) {
                    console.warn("Failed to open footer URL:", url)
                }
            }
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }
    }
}
