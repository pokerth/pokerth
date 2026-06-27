import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Collapsible game list entry used in GameWaitPage (panel & sidebar).
// Required properties injected by the ListView delegate binding:
//   index, collapseResetCounter, listView, searchFilter, gameRevision
Item {
    id: gameItem

    required property int index
    required property int collapseResetCounter
    required property var listView
    required property string searchFilter
    required property int gameRevision

    // ── Model roles (Qt6 ListView auto-binds required properties to roles by name) ──
    required property var gameId
    required property var gameName
    required property var playerCount
    required property var maxPlayers
    required property var gameType
    required property var gameMode

    // ── Convenience aliases ───────────────────────────────────────────────
    readonly property int    itemGameId:      gameId      || 0
    readonly property string itemGameName:    gameName    || ""
    readonly property int    itemPlayerCount: playerCount || 0
    readonly property int    itemMaxPlayers:  maxPlayers  || 10
    readonly property int    itemGameType:    gameType    || 1
    readonly property int    itemGameMode:    gameMode    || 1

    // ── Players in this game (reactive on gameRevision) ───────────────────
    readonly property var gamePlayers: {
        var _r = gameRevision
        return (Lobby && itemGameId) ? (Lobby.gamePlayersInGame(itemGameId) || []) : []
    }

    // ── Kontextaktionen (wie im Qt-Widgets-Client) ────────────────────────
    readonly property bool canReportGame:   Lobby && itemGameId > 0
    readonly property bool canAdminCloseGame: Lobby && Lobby.isCurrentPlayerAdmin && itemGameId > 0

    readonly property color reportColor: Config.StaticData.chartColor(6, true)
    readonly property color closeColor:  Config.StaticData.chartColor(5, true)

    // ── Filter ────────────────────────────────────────────────────────────
    readonly property bool matchesFilter: {
        var f = searchFilter.toLowerCase()
        return f.length === 0 || itemGameName.toLowerCase().includes(f)
    }

    // ── Collapse state ────────────────────────────────────────────────────
    property bool expanded: false

    onCollapseResetCounterChanged: { expanded = false }

    // ── Sizing ────────────────────────────────────────────────────────────
    width: listView.width
    height: matchesFilter ? (headerRect.height + (expanded ? playersCol.height : 0)) : 0
    visible: matchesFilter
    clip: true

    Behavior on height {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }

    // ── Header row ────────────────────────────────────────────────────────
    Rectangle {
        id: headerRect
        width: parent.width
        height: 52
        color: headerMouse.containsMouse
               ? Qt.lighter(Config.StaticData.palette.secondary.col700, 1.2)
               : "transparent"
        radius: 3

        RowLayout {
            // Über headerMouse, damit die Action-Icons Klicks erhalten;
            // Klicks neben den Icons fallen durch (kein MouseArea) auf
            // headerMouse zurück und klappen die Zeile auf/zu.
            z: 1
            anchors { fill: parent; leftMargin: 8; rightMargin: 8 }
            spacing: 5

            // Game type icon
            Image {
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                source: {
                    if (gameItem.itemGameType === 2) return "../resources/userSquare.svg"
                    if (gameItem.itemGameType === 3) return "../resources/users.svg"
                    if (gameItem.itemGameType === 4) return "../resources/chipStack.svg"
                    return "../resources/user.svg"
                }
                sourceSize: Qt.size(28, 28)
                smooth: true
                antialiasing: true
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: Config.StaticData.palette.secondary.col300
                }
            }

            // Name + status line
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                AppText {
                    text: gameItem.itemGameName || ("Game #" + gameItem.itemGameId)
                    font.bold: true
                    font.pixelSize: 12
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                AppText {
                    text: gameItem.itemPlayerCount + "/" + gameItem.itemMaxPlayers
                          + "  ·  "
                          + (Lobby ? Lobby.gameStatusText(gameItem.itemGameMode,
                                                          gameItem.itemPlayerCount,
                                                          gameItem.itemMaxPlayers) : "")
                    font.pixelSize: 11
                    color: {
                        if (gameItem.itemGameMode === 2) return Config.Theme.colorStatusRunning
                        if (gameItem.itemGameMode === 3) return Config.Theme.colorStatusClosed
                        return gameItem.itemPlayerCount < gameItem.itemMaxPlayers
                            ? Config.Theme.colorStatusOpen
                            : Config.Theme.colorStatusFull
                    }
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }

            // Kontextaktionen (rechtsbündig neben dem Tischnamen)
            PlayerActionIcon {
                visible: gameItem.canReportGame
                iconSize: 16
                source: "qrc:/resources/flag.svg"
                baseColor: gameItem.reportColor
                tooltipText: qsTr("Report inappropriate game name")
                onTriggered: reportGamePopup.openWith(
                    qsTr("Report game name"),
                    qsTr("Are you sure you want to report the game name:\n\"%1\" as inappropriate?")
                        .arg(gameItem.itemGameName),
                    qsTr("Report"))
            }
            PlayerActionIcon {
                visible: gameItem.canAdminCloseGame
                iconSize: 16
                source: "qrc:/resources/gavel.svg"
                baseColor: gameItem.closeColor
                tooltipText: qsTr("Close game (admin)")
                onTriggered: closeGamePopup.openWith(
                    qsTr("Close game"),
                    qsTr("Are you sure you want to close the game:\n\"%1\"?")
                        .arg(gameItem.itemGameName),
                    qsTr("Close game"))
            }

            // Expand / collapse chevron
            Image {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                source: "../resources/caretLeft.svg"
                sourceSize: Qt.size(24, 24)
                rotation: gameItem.expanded ? 90 : -90
                smooth: true
                antialiasing: true
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: Config.StaticData.palette.secondary.col400
                }
                Behavior on rotation {
                    NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                }
            }
        }

        MouseArea {
            id: headerMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: gameItem.expanded = !gameItem.expanded
        }
    }

    // ── Expanded player sub-list ──────────────────────────────────────────
    Column {
        id: playersCol
        width: parent.width
        anchors.top: headerRect.bottom
        topPadding: 2
        bottomPadding: 6
        leftPadding: 22

        Repeater {
            model: gameItem.gamePlayers
            // Länderflagge + Name (wie die Spielerliste der Game-Info-Ansicht)
            // statt einer Aufzählung mit vorangestelltem Punkt.
            delegate: Item {
                required property var modelData
                width: playersCol.width - playersCol.leftPadding - 8
                height: 18

                Image {
                    id: playerFlag
                    visible: (parent.modelData.countryCode || "") !== ""
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18
                    height: 14
                    source: (parent.modelData.countryCode || "") !== ""
                        ? "qrc:/resources/cflags/" + (parent.modelData.countryCode || "").toLowerCase() + ".svg"
                        : ""
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }

                AppText {
                    anchors.left: playerFlag.visible ? playerFlag.right : parent.left
                    anchors.leftMargin: playerFlag.visible ? 6 : 0
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: parent.modelData.playerName || parent.modelData.name || ""
                    font.pixelSize: 11
                    color: Config.StaticData.palette.secondary.col300
                    elide: Text.ElideRight
                }
            }
        }
    }

    ConfirmPopup {
        id: reportGamePopup
        onConfirmed: { if (Lobby) Lobby.reportGameName(gameItem.itemGameId) }
    }

    ConfirmPopup {
        id: closeGamePopup
        onConfirmed: { if (Lobby) Lobby.adminCloseGame(gameItem.itemGameId) }
    }
}
