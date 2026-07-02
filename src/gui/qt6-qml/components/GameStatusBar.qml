import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Status-Leiste oben am Tisch: links die Pott-Info (Total/Bets), rechts
// Spielphase, Game-ID und Hand-Nummer – analog zum Qt-Widgets-Client neben
// den Community-Cards. Höhe/Layout werden vom Aufrufer über Layout.* gesetzt.
Rectangle {
    color: Qt.rgba(0, 0, 0, 0.78)

    // Horizontal zentriert oben: Tischname (nur Netzwerkspiele). Bei Local-
    // Games ist Lobby.currentGameId == 0 → currentGameName() leer, die Anzeige
    // verschwindet automatisch. Die Bindung referenziert currentGameId, damit
    // sie beim Spielwechsel/Beitritt reaktiv neu ausgewertet wird.
    readonly property string tableName:
        (typeof Lobby !== "undefined" && Lobby && Lobby.currentGameId > 0)
            ? Lobby.currentGameName() : ""

    // URL der Tisch-Statistikübersicht (nur Ranglistenspiele; sonst leer). An
    // currentGameId und die Spielerliste gebunden, damit sie bei Beitritt/
    // Verlassen neu ausgewertet wird – analog zum Qt-Widgets-Client.
    readonly property string tableStatsUrl:
        (typeof Lobby !== "undefined" && Lobby && Lobby.currentGameId > 0
            && (GameTable ? GameTable.players : true))
            ? Lobby.currentTableStatsUrl() : ""

    AppText {
        id: tableNameLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        // Nicht in die Pot-/Phasen-Spalten an den Rändern hineinragen.
        width: Math.min(implicitWidth, parent.width * 0.5)
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        visible: text !== ""
        text: tableName
        color: "#FFFFFF"
        font.pixelSize: Config.Responsive.landscapeCompact ? 12 : 14
        font.weight: Font.DemiBold
        font.letterSpacing: 0.5
        // Nur bei vorhandener Statistik-URL (Ranglistenspiel) als Link
        // darstellen und anklickbar machen.
        readonly property bool clickable: tableStatsUrl !== ""
        font.underline: clickable && tableNameHover.hovered

        HoverHandler {
            id: tableNameHover
            enabled: tableNameLabel.clickable
            cursorShape: Qt.PointingHandCursor
        }

        MouseArea {
            anchors.fill: parent
            enabled: tableNameLabel.clickable
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                // AppImage-sicher über den LobbyHandler öffnen (nicht
                // Qt.openUrlExternally, siehe ChatBox/LobbyStatsBar).
                var opened = Lobby ? Lobby.openExternalUrl(tableStatsUrl)
                                   : Qt.openUrlExternally(tableStatsUrl)
                if (!opened)
                    console.warn("Failed to open table stats URL:", tableStatsUrl)
            }
        }
    }

    RowLayout {
        anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
        spacing: 0

        // Links: Pot-Info (1:1 wie Widget-Client links neben den Community-Cards)
        // "Total" = aufgelaufener Pot (getPot), "Bets" = laufende Einsätze dieser Runde (getSets)
        Column {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
            spacing: 0
            Row {
                spacing: 4
                AppText {
                    text: qsTr("Total:")
                    color: "#9e9e9e"
                    font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                    font.weight: Font.Medium
                }
                AppText {
                    text: "$%1".arg(GameTable ? GameTable.pot : 0)
                    color: "#99D500"
                    font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                    font.bold: true
                }
            }
            Row {
                spacing: 4
                AppText {
                    text: qsTr("Bets:")
                    color: "#9e9e9e"
                    font.pixelSize: Config.Responsive.landscapeCompact ? 10 : 11
                    font.weight: Font.Medium
                }
                AppText {
                    text: "$%1".arg(GameTable ? (GameTable.totalPot - GameTable.pot) : 0)
                    color: "#7aa800"
                    font.pixelSize: Config.Responsive.landscapeCompact ? 10 : 11
                    font.weight: Font.Medium
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Zuschauer-Anzeige (Auge + Anzahl) links neben der rechtsbündigen
        // Phasen-/Game-Info. Nur sichtbar, wenn das laufende Spiel mindestens
        // einen Zuschauer hat – analog zum Qt-Widgets-Client. Namen im Tooltip.
        Row {
            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: 12
            spacing: 4
            visible: GameTable ? GameTable.spectatorCount > 0 : false

            SvgIcon {
                anchors.verticalCenter: parent.verticalCenter
                width: Config.Responsive.landscapeCompact ? 14 : 16
                height: width
                source: "../resources/eye.svg"
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: "#FFFFFF"
                }
            }
            AppText {
                anchors.verticalCenter: parent.verticalCenter
                text: GameTable ? GameTable.spectatorCount : 0
                color: "#FFFFFF"
                font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                font.bold: true
            }

            HoverHandler { id: spectatorHover }
            ToolTip {
                visible: spectatorHover.hovered && text.length > 0
                text: GameTable ? GameTable.spectatorNames.join("\n") : ""
            }
        }

        // Rechts: Phase + Game-ID + Hand-Nummer (1:1 wie Widget-Client rechts neben den Community-Cards)
        Column {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            spacing: 0
            AppText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: GameTable ? GameTable.phaseText : qsTr("Preflop")
                color: "#FFFFFF"
                font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                font.weight: Font.DemiBold
                font.letterSpacing: 0.5
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 8
                AppText {
                    text: qsTr("Game: %1").arg(GameTable ? GameTable.gameId : 0)
                    color: "#9e9e9e"
                    font.pixelSize: Config.Responsive.landscapeCompact ? 9 : 11
                    font.weight: Font.Medium
                }
                AppText {
                    text: qsTr("Hand: %1").arg(GameTable ? GameTable.handNumber : 1)
                    color: "#9e9e9e"
                    font.pixelSize: Config.Responsive.landscapeCompact ? 9 : 11
                    font.weight: Font.Medium
                }
            }
        }
    }
}
