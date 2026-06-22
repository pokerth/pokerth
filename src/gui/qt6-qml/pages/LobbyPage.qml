import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config
import "../components"

Rectangle {
    id: lobbyPage
    objectName: "lobbyPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    color: Config.StaticData.palette.secondary.col700

    // Mock data for development
    property int connectedPlayers: Lobby ? Lobby.playerListModel.count : 0
    property int runningGames: Lobby ? Lobby.gameListModel.runningCount : 0
    property int openGames: Lobby ? Lobby.gameListModel.openCount : 0

    // Portrait-mode overlay state
    property bool showingPlayerList: false
    property bool showingGameInfo: false
    property var selectedGame: null
    property int playerListCollapseResetCounter: 0

    readonly property int gameListRevision: Lobby ? Lobby.gameListRevision : 0
    readonly property bool selectedGameJoinable: {
        var _gameRev = gameListRevision
        return (Lobby && selectedGame) ? Lobby.canJoinGame(selectedGame.gameId || 0) : false
    }
    readonly property var selectedGamePlayers: {
        var _playerRev = Lobby ? Lobby.playerListRevision : 0
        var _gameRev = gameListRevision
        var gid = selectedGame ? selectedGame.gameId : 0
        return (Lobby && gid) ? Lobby.gamePlayersInGame(gid) : []
    }

    function gameTypeIconSource(gameType) {
        if (gameType === 2) return "../resources/userSquare.svg"
        if (gameType === 3) return "../resources/users.svg"
        if (gameType === 4) return "../resources/chipStack.svg"
        return "../resources/user.svg"
    }

    function gameStatusColor(mode, count, maxPlayers) {
        if (mode === 2) return Config.Theme.colorStatusRunning
        if (mode === 3) return Config.Theme.colorStatusClosed
        return count < maxPlayers ? Config.Theme.colorStatusOpen : Config.Theme.colorStatusFull
    }

    // Kontextaktionen für das ausgewählte Spiel (Bestätigung wie im Widget-Client)
    function requestReportGame() {
        if (!selectedGame) return
        reportGamePopup.openWith(
            qsTr("Report game name"),
            qsTr("Are you sure you want to report the game name:\n\"%1\" as inappropriate?")
                .arg(selectedGame.gameName || ("Game #" + selectedGame.gameId)),
            qsTr("Report"))
    }

    function requestCloseGame() {
        if (!selectedGame) return
        closeGamePopup.openWith(
            qsTr("Close game"),
            qsTr("Are you sure you want to close the game:\n\"%1\"?")
                .arg(selectedGame.gameName || ("Game #" + selectedGame.gameId)),
            qsTr("Close game"))
    }

    function resetPlayerListDelegates() {
        playerListCollapseResetCounter += 1
        playerPanelList.currentIndex = -1
        playerListView.currentIndex = -1
    }

    function applyPlayerListFilter(mode) {
        if (!Lobby) {
            return
        }

        if (Lobby.playerListFilterMode !== mode) {
            Lobby.playerListFilterMode = mode
        }

        resetPlayerListDelegates()
    }

    Connections {
        target: Lobby

        function onPlayerListFilterModeChanged() {
            if (playerListFilterCompact.currentIndex !== Lobby.playerListFilterMode) {
                playerListFilterCompact.currentIndex = Lobby.playerListFilterMode
            }
            if (playerListFilterWide.currentIndex !== Lobby.playerListFilterMode) {
                playerListFilterWide.currentIndex = Lobby.playerListFilterMode
            }
            lobbyPage.resetPlayerListDelegates()
        }

        function onGameListFilterModeChanged() {
            if (gameListFilter.currentIndex !== Lobby.gameListFilterMode) {
                gameListFilter.currentIndex = Lobby.gameListFilterMode
            }
        }

        function onSelfJoinedGame() {
            console.log("[NAV] onSelfJoinedGame | depth before:", mainStackView.depth, "| currentItem:", mainStackView.currentItem ? (mainStackView.currentItem.objectName || mainStackView.currentItem.toString()) : "null")
            // pop(lobbyPage) entfernt alles ÜBER lobbyPage; ist lobbyPage schon oben, passiert nichts
            mainStackView.pop(lobbyPage, StackView.Immediate)
            console.log("[NAV]   pushing GameWaitPage | depth now:", mainStackView.depth)
            mainStackView.push("GameWaitPage.qml")
        }
    }

    // ── Compact: Player list panel (slides in from left) ───────────────────
    Rectangle {
        id: playerPanel
        width: lobbyPage.width
        height: lobbyPage.height
        // Use lobbyPage.width/height (not mainWindow) so it always fills the StackView item
        y: 0
        x: lobbyPage.showingPlayerList ? 0 : -width
        z: 3
        color: Config.StaticData.palette.secondary.col700
        visible: Config.Responsive.compact

        Behavior on x {
            NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
        }

        // Right border line
        Rectangle {
            anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
            width: 1
            color: Config.StaticData.palette.secondary.col500
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Players")
                    font.family: Config.StaticData.loadedFont.font.family
                    font.bold: true
                    font.pixelSize: 15
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 30
                    height: 30
                    radius: 4
                    color: closePlayerArea.containsMouse
                           ? Config.StaticData.palette.secondary.col600
                           : "transparent"

                    Image {
                        anchors.centerIn: parent
                        width: 14
                        height: 14
                        source: "../resources/close.svg"
                        sourceSize: Qt.size(28, 28)
                        smooth: true
                        antialiasing: true
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            colorization: 1.0
                            colorizationColor: Config.Theme.colorTextSecondary
                        }
                    }

                    MouseArea {
                        id: closePlayerArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: lobbyPage.showingPlayerList = false
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Config.StaticData.palette.secondary.col500
            }

            TextField {
                id: playerSearchField
                Layout.fillWidth: true
                placeholderText: qsTr("search for player ...")
                font.family: Config.StaticData.loadedFont.font.family
                color: Config.StaticData.palette.secondary.col200
                background: Rectangle {
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.3)
                    radius: 3
                    border.width: 1
                    border.color: playerSearchField.activeFocus
                                  ? Config.Theme.withAlpha(Config.Theme.colorAccent, 0.75)
                                  : "transparent"
                    Behavior on border.color { ColorAnimation { duration: 140 } }
                }
                placeholderTextColor: Qt.lighter(Config.StaticData.palette.secondary.col200, 1.5)
            }

            ListView {
                id: playerPanelList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                property int expandedPlayerIndex: -1
                model: Lobby ? Lobby.playerListProxyModel : null

                delegate: PlayerListItem {
                    collapseResetCounter: lobbyPage.playerListCollapseResetCounter
                    listView: playerPanelList
                    visible: (playerSearchField.text.length === 0 ||
                             displayName.toLowerCase().includes(playerSearchField.text.toLowerCase()))
                }
            }

            ComboBox {
                id: playerListFilterCompact
                Layout.fillWidth: true
                font.family: Config.StaticData.loadedFont.font.family
                model: [
                    qsTr("Sort alphabetically"),
                    qsTr("Sort by country"),
                    qsTr("Display idle players")
                ]
                currentIndex: Lobby ? Lobby.playerListFilterMode : 0
                onCurrentIndexChanged: {
                    lobbyPage.applyPlayerListFilter(currentIndex)
                }
            }
        }
    }

    // ── Compact: Game info overlay (slides in from right) ──────────────────
    Rectangle {
        id: gameInfoOverlay
        width: lobbyPage.width
        height: lobbyPage.height
        y: 0
        x: lobbyPage.showingGameInfo ? 0 : lobbyPage.width
        z: 2
        color: lobbyPage.color
        visible: Config.Responsive.compact

        Behavior on x {
            NumberAnimation { duration: 280; easing.type: Easing.OutCubic }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Config.Theme.margin
            spacing: Config.Theme.spacing

            // Header: back arrow + title
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    implicitWidth: 38
                    implicitHeight: 38
                    radius: 5
                    color: backArea.containsMouse
                           ? Config.StaticData.palette.secondary.col600
                           : Config.StaticData.palette.secondary.col700
                    border.color: Config.StaticData.palette.secondary.col500
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        source: "../resources/caretLeft.svg"
                        sourceSize: Qt.size(36, 36)
                        smooth: true
                        antialiasing: true
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            colorization: 1.0
                            colorizationColor: Config.StaticData.palette.secondary.col200
                        }
                    }

                    MouseArea {
                        id: backArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: lobbyPage.showingGameInfo = false
                    }
                }

                Label {
                    text: qsTr("Game Info")
                    font.family: Config.StaticData.loadedFont.font.family
                    font.bold: true
                    font.pixelSize: 16
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                }

                // Kontextaktionen für das ausgewählte Spiel
                Row {
                    visible: lobbyPage.selectedGame !== null
                    spacing: 2
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                    PlayerActionIcon {
                        iconSize: 22
                        source: "qrc:/resources/flag.svg"
                        baseColor: Config.StaticData.chartColor(6, true)
                        tooltipText: qsTr("Report inappropriate game name")
                        onTriggered: lobbyPage.requestReportGame()
                    }
                    PlayerActionIcon {
                        visible: Lobby && Lobby.isCurrentPlayerAdmin
                        iconSize: 22
                        source: "qrc:/resources/gavel.svg"
                        baseColor: Config.StaticData.chartColor(5, true)
                        tooltipText: qsTr("Close game (admin)")
                        onTriggered: lobbyPage.requestCloseGame()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Config.StaticData.palette.secondary.col500
            }

            // Game details card
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                radius: 6

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Label {
                        text: lobbyPage.selectedGame
                              ? (lobbyPage.selectedGame.gameName || ("Game #" + lobbyPage.selectedGame.gameId))
                              : ""
                        font.family: Config.StaticData.loadedFont.font.family
                        font.bold: true
                        font.pixelSize: 15
                        color: Config.StaticData.palette.secondary.col100
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Label {
                        text: lobbyPage.selectedGame
                              ? qsTr("Players: %1 / %2")
                                .arg(lobbyPage.selectedGame.playerCount || 0)
                                .arg(lobbyPage.selectedGame.maxPlayers || 10)
                              : ""
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col200
                    }

                    Label {
                        text: {
                            if (!lobbyPage.selectedGame) return ""
                            var mode = lobbyPage.selectedGame.gameMode || 1
                            var cnt = lobbyPage.selectedGame.playerCount || 0
                            var max = lobbyPage.selectedGame.maxPlayers || 10
                            return qsTr("Status: %1").arg(Lobby ? Lobby.gameStatusText(mode, cnt, max) : "")
                        }
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: {
                            if (!lobbyPage.selectedGame) return Config.StaticData.palette.secondary.col300
                            var mode = lobbyPage.selectedGame.gameMode || 1
                            var cnt = lobbyPage.selectedGame.playerCount || 0
                            var max = lobbyPage.selectedGame.maxPlayers || 10
                            return lobbyPage.gameStatusColor(mode, cnt, max)
                        }
                    }

                    RowLayout {
                        visible: lobbyPage.selectedGame !== null
                        Layout.fillWidth: true
                        spacing: 6

                        Image {
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            source: lobbyPage.gameTypeIconSource((lobbyPage.selectedGame && lobbyPage.selectedGame.gameType) || 1)
                            sourceSize: Qt.size(28, 28)
                            smooth: true
                            antialiasing: true
                            layer.enabled: true
                            layer.effect: MultiEffect {
                                colorization: 1.0
                                colorizationColor: Config.StaticData.palette.secondary.col300
                            }
                        }

                        Label {
                            text: lobbyPage.selectedGame
                                  ? qsTr("Type: %1").arg(Lobby ? Lobby.gameTypeText(lobbyPage.selectedGame.gameType || 1) : "")
                                  : ""
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 13
                            color: Config.StaticData.palette.secondary.col200
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }

                    Label {
                        text: lobbyPage.selectedGame
                              ? qsTr("Small blind: %1").arg(lobbyPage.selectedGame.firstSmallBlind || 10)
                              : ""
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col200
                    }

                    Label {
                        text: lobbyPage.selectedGame
                              ? qsTr("Start cash: %1").arg(lobbyPage.selectedGame.startMoney || 0)
                              : ""
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col200
                    }

                    Label {
                        text: {
                            if (!lobbyPage.selectedGame) return ""
                            var mode = lobbyPage.selectedGame.raiseIntervalMode || 1
                            if (mode === 1) {
                                return qsTr("Blinds raise interval: %1 hands").arg(lobbyPage.selectedGame.raiseEveryHands || 0)
                            }
                            return qsTr("Blinds raise interval: %1 minutes").arg(lobbyPage.selectedGame.raiseEveryMinutes || 0)
                        }
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        text: lobbyPage.selectedGame
                              ? qsTr("Blinds raise mode: %1").arg((lobbyPage.selectedGame.raiseMode || 1) === 1
                                  ? qsTr("double blinds") : qsTr("manual blinds order"))
                              : ""
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: lobbyPage.selectedGame && (lobbyPage.selectedGame.raiseMode || 1) !== 1 &&
                                 (lobbyPage.selectedGame.manualBlindsText || "").length > 0
                        text: lobbyPage.selectedGame
                              ? qsTr("Blinds list: %1").arg(lobbyPage.selectedGame.manualBlindsText || "")
                              : ""
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        text: lobbyPage.selectedGame
                              ? qsTr("Game timing: %1 sec (action)\n%2 sec (hand delay)")
                                .arg(lobbyPage.selectedGame.playerActionTimeoutSec || 0)
                                .arg(lobbyPage.selectedGame.delayBetweenHandsSec || 0)
                              : ""
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        visible: lobbyPage.selectedGame !== null
                        text: qsTr("Players in game (%1)").arg(lobbyPage.selectedGamePlayers.length)
                        font.family: Config.StaticData.loadedFont.font.family
                        font.bold: true
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col100
                    }

                    ListView {
                        visible: lobbyPage.selectedGame !== null
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: lobbyPage.selectedGamePlayers
                        spacing: 4

                        delegate: Rectangle {
                            required property var modelData
                            width: ListView.view ? ListView.view.width : 0
                            height: 32
                            radius: 4
                            color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.1)

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 6

                                Rectangle {
                                    Layout.preferredWidth: 22
                                    Layout.preferredHeight: 22
                                    radius: 11
                                    color: "transparent"
                                    clip: true

                                    Image {
                                        anchors.fill: parent
                                        visible: (modelData.avatarUrl || "").length > 0
                                        source: modelData.avatarUrl || ""
                                        fillMode: Image.PreserveAspectCrop
                                        smooth: true
                                    }

                                    SvgIcon {
                                        visible: !((modelData.avatarUrl || "").length > 0)
                                        anchors.fill: parent
                                        source: "../resources/pokerth.svg"
                                    }
                                }

                                Image {
                                    visible: (modelData.countryCode || "") !== ""
                                    source: (modelData.countryCode || "") !== ""
                                        ? "qrc:/resources/cflags/" + (modelData.countryCode || "").toLowerCase() + ".svg"
                                        : ""
                                    Layout.preferredWidth: 18
                                    Layout.preferredHeight: 14
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                }

                                Text {
                                    text: modelData.playerName || ""
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: 12
                                    color: (modelData.isAdmin === true)
                                        ? Config.StaticData.chartColor(3, true)
                                        : Config.StaticData.palette.secondary.col200
                                    font.bold: modelData.isAdmin === true
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }

                }
            }

            // Join button
            CustomButton {
                text: (Lobby && Lobby.isInGame) ? qsTr("Leave Game") : qsTr("Join Game")
                Layout.fillWidth: true
                visible: (Lobby && Lobby.isInGame) || lobbyPage.selectedGameJoinable
                enabled: (Lobby && Lobby.isInGame) || lobbyPage.selectedGameJoinable
                onClicked: {
                    if (Lobby && Lobby.isInGame) {
                        Lobby.leaveGame()
                    } else if (lobbyPage.selectedGame && lobbyPage.selectedGameJoinable) {
                        if (lobbyPage.selectedGame.isPrivate) {
                            joinPasswordPopup.pendingGameId = lobbyPage.selectedGame.gameId
                            joinPasswordPopup.open()
                        } else {
                            Lobby.joinGame(lobbyPage.selectedGame.gameId, "")
                        }
                        lobbyPage.showingGameInfo = false
                    }
                }
            }
        }
    }

    // ── Main layout ────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Config.Theme.margin
        spacing: Config.Theme.spacing

        // Filter row
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Compact: player list toggle button
            Rectangle {
                visible: Config.Responsive.compact
                implicitWidth: 38
                implicitHeight: 38
                radius: 5
                color: playerToggleArea.containsMouse
                       ? Config.StaticData.palette.secondary.col600
                       : Config.StaticData.palette.secondary.col700
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "../resources/users.svg"
                    sourceSize: Qt.size(48, 48)
                    smooth: true
                    antialiasing: true
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: Config.StaticData.palette.secondary.col200
                    }
                }

                MouseArea {
                    id: playerToggleArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: lobbyPage.showingPlayerList = !lobbyPage.showingPlayerList
                }
            }

            // Wide: player search in filter bar
            TextField {
                id: searchPlayerField
                visible: !Config.Responsive.compact
                Layout.fillWidth: true
                placeholderText: qsTr("search for player ...")
                font.family: Config.StaticData.loadedFont.font.family
                color: Config.StaticData.palette.secondary.col200
                background: Rectangle {
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.3)
                    radius: 3
                    border.width: 1
                    border.color: searchPlayerField.activeFocus
                                  ? Config.Theme.withAlpha(Config.Theme.colorAccent, 0.75)
                                  : "transparent"
                    Behavior on border.color { ColorAnimation { duration: 140 } }
                }
                placeholderTextColor: Qt.lighter(Config.StaticData.palette.secondary.col200, 1.5)
            }

            ComboBox {
                id: gameListFilter
                Layout.fillWidth: true
                font.family: Config.StaticData.loadedFont.font.family
                model: [
                    qsTr("No game list filter"),
                    qsTr("Show open games"),
                    qsTr("Show open & non-full games"),
                    qsTr("Show open & non-full & non-private games"),
                    qsTr("Show open & non-full & private games"),
                    qsTr("Show open & non-full & ranking games")
                ]
                currentIndex: Lobby ? Lobby.gameListFilterMode : 0
                onCurrentIndexChanged: {
                    if (Lobby && Lobby.gameListFilterMode !== currentIndex) {
                        Lobby.gameListFilterMode = currentIndex
                    }
                }
            }
        }

        // Main content — desktop: drei resizable Spalten (Spielerliste |
        // Spielliste/Chat | Game-Info/Chat) mit Min-/Max-Breiten, sodass immer
        // alle drei Spalten sinnvoll sichtbar bleiben. Compact: nur die
        // Mittelspalte ist sichtbar, die Seitenspalten entfallen.
        SplitView {
            id: lobbyContentSplit
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal
            handle: ResizeHandle { horizontal: true }

            // Wide: Player list (hidden in compact — uses slide panel instead)
            // Initial 1:2:1 ratio (1/4 width)
            Rectangle {
                visible: !Config.Responsive.compact
                SplitView.preferredWidth: lobbyContentSplit.width / 4
                SplitView.minimumWidth: 160
                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                radius: 5

                PanelShadow { anchors.fill: parent; radius: parent.radius; color: parent.color }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5

                    Label {
                        text: qsTr("Available Players")
                        font.family: Config.StaticData.loadedFont.font.family
                        font.bold: true
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    SectionDivider {}

                    ListView {
                        id: playerListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        property int expandedPlayerIndex: -1
                        model: Lobby ? Lobby.playerListProxyModel : null

                        delegate: PlayerListItem {
                            collapseResetCounter: lobbyPage.playerListCollapseResetCounter
                            listView: playerListView
                        }
                    }

                    ComboBox {
                        id: playerListFilterWide
                        Layout.fillWidth: true
                        font.family: Config.StaticData.loadedFont.font.family
                        model: [
                            qsTr("Sort alphabetically"),
                            qsTr("Sort by country"),
                            qsTr("Display idle players")
                        ]
                        currentIndex: Lobby ? Lobby.playerListFilterMode : 0
                        onCurrentIndexChanged: {
                            lobbyPage.applyPlayerListFilter(currentIndex)
                        }
                    }
                }
            }

            // Center: Game list + (compact: chat below)
            ColumnLayout {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 280
                spacing: 5

                // Compact: Spielliste und Chat sind vertikal resizable
                // (Min-Höhe je 1/3). Wide: nur die Spielliste, füllt den Platz.
                SplitView {
                    id: lobbyCenterSplit
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Vertical
                    handle: ResizeHandle { horizontal: false }

                // Game list: same height as chat in compact
                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: Config.Responsive.compact ? lobbyCenterSplit.height / 3 : 0
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5

                    PanelShadow { anchors.fill: parent; radius: parent.radius; color: parent.color }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 5

                        Label {
                            text: qsTr("Game List")
                            font.family: Config.StaticData.loadedFont.font.family
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        SectionDivider {}

                        // Game list
                        ListView {
                            id: gameListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: Lobby ? Lobby.gameListProxyModel : null

                            delegate: ItemDelegate {
                                id: gameRow
                                width: gameListView.width
                                height: 54

                                readonly property bool selected: ListView.isCurrentItem

                                ColumnLayout {
                                    anchors {
                                        fill: parent
                                        leftMargin: 8; rightMargin: 8
                                        topMargin: 5; bottomMargin: 5
                                    }
                                    spacing: 3

                                    Text {
                                        text: model.gameName || ("Game #" + model.gameId)
                                        font.family: Config.StaticData.loadedFont.font.family
                                        font.bold: true
                                        font.pixelSize: 13
                                        color: Config.StaticData.palette.secondary.col200
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 5

                                        Text {
                                            text: (model.playerCount || 0) + "/" + (model.maxPlayers || 10)
                                            font.family: Config.StaticData.loadedFont.font.family
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col300
                                        }
                                        Text {
                                            text: "·"
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col500
                                        }
                                        Text {
                                            readonly property int gm: (model.gameMode || 1)
                                            readonly property int cnt: (model.playerCount || 0)
                                            readonly property int max: (model.maxPlayers || 10)
                                            text: Lobby ? Lobby.gameStatusText(gm, cnt, max) : ""
                                            font.family: Config.StaticData.loadedFont.font.family
                                            font.pixelSize: 12
                                            color: {
                                                if (gm === 2) return Config.Theme.colorStatusRunning
                                                if (gm === 3) return Config.Theme.colorStatusClosed
                                                return cnt < max ? Config.Theme.colorStatusOpen : Config.Theme.colorStatusFull
                                            }
                                        }
                                        Text {
                                            text: "·"
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col500
                                        }
                                        Text {
                                            readonly property int actionSec: model.playerActionTimeoutSec > 0 ? model.playerActionTimeoutSec : 0
                                            readonly property int handDelaySec: model.delayBetweenHandsSec > 0 ? model.delayBetweenHandsSec : 0
                                            text: qsTr("Time: %1s/%2s").arg(actionSec).arg(handDelaySec)
                                            font.family: Config.StaticData.loadedFont.font.family
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col300
                                        }
                                        Text {
                                            text: "·"
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col500
                                        }
                                        Text {
                                            text: model.isPrivate ? qsTr("Private") : qsTr("Public")
                                            font.family: Config.StaticData.loadedFont.font.family
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col300
                                        }
                                        Text {
                                            text: "·"
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col500
                                            visible: model.gameType === 4
                                        }
                                        Text {
                                            text: qsTr("Ranking")
                                            font.family: Config.StaticData.loadedFont.font.family
                                            font.pixelSize: 12
                                            color: Config.StaticData.palette.secondary.col300
                                            visible: model.gameType === 4
                                        }
                                        Item { Layout.fillWidth: true }
                                    }
                                }

                                background: Rectangle {
                                    color: gameRow.selected
                                           ? Config.Theme.withAlpha(Config.Theme.colorAccent, 0.13)
                                           : gameRow.hovered
                                             ? Qt.lighter(Config.StaticData.palette.secondary.col700, 1.3)
                                             : "transparent"
                                    radius: 3
                                    Behavior on color { ColorAnimation { duration: 130 } }

                                    // Dezenter Gold-Akzentbalken links bei Auswahl
                                    Rectangle {
                                        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                                        width: 3
                                        radius: width / 2
                                        color: Config.Theme.colorAccent
                                        opacity: gameRow.selected ? 0.85 : 0
                                        Behavior on opacity { NumberAnimation { duration: 130 } }
                                    }
                                }

                                onClicked: {
                                    gameListView.currentIndex = index
                                    lobbyPage.selectedGame = {
                                        gameId: model.gameId,
                                        gameName: model.gameName,
                                        playerCount: model.playerCount,
                                        maxPlayers: model.maxPlayers,
                                        gameMode: model.gameMode,
                                        isPrivate: model.isPrivate,
                                        gameType: model.gameType,
                                        firstSmallBlind: model.firstSmallBlind,
                                        startMoney: model.startMoney,
                                        raiseIntervalMode: model.raiseIntervalMode,
                                        raiseEveryHands: model.raiseEveryHands,
                                        raiseEveryMinutes: model.raiseEveryMinutes,
                                        raiseMode: model.raiseMode,
                                        manualBlindsText: model.manualBlindsText,
                                        playerActionTimeoutSec: model.playerActionTimeoutSec,
                                        delayBetweenHandsSec: model.delayBetweenHandsSec
                                    }

                                    if (Config.Responsive.compact) {
                                        lobbyPage.showingGameInfo = true
                                    }
                                }

                                // Desktop-Lobby: Doppelklick auf eine Spielzeile
                                // tritt dem Spiel direkt bei (wie der "Join Game"-
                                // Button). selectedGame ist durch onClicked oben
                                // bereits auf die geklickte Zeile gesetzt.
                                onDoubleClicked: {
                                    if (Config.Responsive.compact) return
                                    if (!Lobby || Lobby.isInGame) return
                                    if (!lobbyPage.selectedGame || !lobbyPage.selectedGameJoinable) return
                                    if (lobbyPage.selectedGame.isPrivate) {
                                        joinPasswordPopup.pendingGameId = lobbyPage.selectedGame.gameId
                                        joinPasswordPopup.open()
                                    } else {
                                        Lobby.joinGame(lobbyPage.selectedGame.gameId, "")
                                    }
                                    lobbyPage.showingGameInfo = false
                                }
                            }
                        }
                    }
                }

                // Compact: Lobby Chat — vertikal resizable, Min-Höhe 1/3
                Rectangle {
                    visible: Config.Responsive.compact
                    SplitView.preferredHeight: lobbyCenterSplit.height / 2
                    SplitView.minimumHeight: lobbyCenterSplit.height / 3
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5

                    PanelShadow { anchors.fill: parent; radius: parent.radius; color: parent.color }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 4

                        Label {
                            text: qsTr("Lobby Chat")
                            font.family: Config.StaticData.loadedFont.font.family
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        SectionDivider {}

                        ChatBox {
                            id: lobbyChatBoxCompact
                            historyStore: Config.StaticData.lobbyChatHistory
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            chatModel: (typeof Lobby !== "undefined" && Lobby) ? Lobby.chatLog : []
                            // Vollständige (ungefilterte) Spielerliste für die
                            // Tab-Vervollständigung – damit auch Spieler in
                            // (offenen) Spielen vervollständigt werden, die der
                            // Spielerlisten-Filter ausblenden kann.
                            nickList: {
                                var _r = (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerListRevision : 0
                                return (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerNickList() : []
                            }
                            pickerInlineHeight: 140
                            onSendRequested: (text) => {
                                if (typeof Lobby !== "undefined" && Lobby)
                                    Lobby.sendChatMessage(text)
                            }
                        }
                    }
                }
                } // lobbyCenterSplit

                // Action buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: qsTr("Player: %1").arg(
                            Lobby && Lobby.myPlayerName !== "" ? Lobby.myPlayerName : "Guest")
                        font.family: Config.StaticData.loadedFont.font.family
                        font.bold: true
                        font.pixelSize: 14
                        color: Config.StaticData.palette.secondary.col100
                    }

                    Button {
                        text: qsTr("Create Game")
                        font.family: Config.StaticData.loadedFont.font.family
                        Layout.fillWidth: true
                        onClicked: mainStackView.push("LobbyCreateGamePage.qml")
                    }

                    Button {
                        text: (Lobby && Lobby.isInGame) ? qsTr("Leave Game") : qsTr("Join Game")
                        font.family: Config.StaticData.loadedFont.font.family
                        Layout.fillWidth: true
                        visible: !Config.Responsive.compact && ((Lobby && Lobby.isInGame) || lobbyPage.selectedGameJoinable)
                        enabled: (Lobby && Lobby.isInGame) || lobbyPage.selectedGameJoinable
                        onClicked: {
                            if (Lobby && Lobby.isInGame) {
                                Lobby.leaveGame()
                            } else if (lobbyPage.selectedGame && lobbyPage.selectedGameJoinable) {
                                if (lobbyPage.selectedGame.isPrivate) {
                                    joinPasswordPopup.pendingGameId = lobbyPage.selectedGame.gameId
                                    joinPasswordPopup.open()
                                } else {
                                    Lobby.joinGame(lobbyPage.selectedGame.gameId, "")
                                }
                            }
                        }
                    }
                }
            }

            // Wide: right panel — Game Info + Chat, vertikal resizable
            // (Min-Höhe je 1/3), damit Game-Info verkleinert und der Chat
            // vergrößert werden kann (und umgekehrt).
            SplitView {
                id: lobbyRightSplit
                visible: !Config.Responsive.compact
                SplitView.preferredWidth: lobbyContentSplit.width / 4
                SplitView.minimumWidth: 200
                orientation: Qt.Vertical
                handle: ResizeHandle { horizontal: false }

                // Game Info
                Rectangle {
                    SplitView.preferredHeight: 200
                    SplitView.minimumHeight: lobbyRightSplit.height / 3
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5

                    PanelShadow { anchors.fill: parent; radius: parent.radius; color: parent.color }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: qsTr("Game Info")
                                font.family: Config.StaticData.loadedFont.font.family
                                font.bold: true
                                font.pixelSize: 14
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }

                            // Kontextaktionen für das ausgewählte Spiel
                            PlayerActionIcon {
                                visible: lobbyPage.selectedGame !== null
                                source: "qrc:/resources/flag.svg"
                                baseColor: Config.StaticData.chartColor(6, true)
                                tooltipText: qsTr("Report inappropriate game name")
                                onTriggered: lobbyPage.requestReportGame()
                            }
                            PlayerActionIcon {
                                visible: lobbyPage.selectedGame !== null && Lobby && Lobby.isCurrentPlayerAdmin
                                source: "qrc:/resources/gavel.svg"
                                baseColor: Config.StaticData.chartColor(5, true)
                                tooltipText: qsTr("Close game (admin)")
                                onTriggered: lobbyPage.requestCloseGame()
                            }
                        }

                        SectionDivider {}

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Image {
                                visible: lobbyPage.selectedGame !== null
                                Layout.preferredWidth: 14
                                Layout.preferredHeight: 14
                                source: lobbyPage.gameTypeIconSource((lobbyPage.selectedGame && lobbyPage.selectedGame.gameType) || 1)
                                sourceSize: Qt.size(28, 28)
                                smooth: true
                                antialiasing: true
                                layer.enabled: true
                                layer.effect: MultiEffect {
                                    colorization: 1.0
                                    colorizationColor: Config.StaticData.palette.secondary.col300
                                }
                            }

                            Label {
                                text: lobbyPage.selectedGame
                                      ? qsTr("Type: %1").arg(Lobby ? Lobby.gameTypeText(lobbyPage.selectedGame.gameType || 1) : "")
                                      : qsTr("Select a game to see details")
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 12
                                color: Config.StaticData.palette.secondary.col300
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }

                        Label {
                            visible: lobbyPage.selectedGame !== null
                            text: lobbyPage.selectedGame
                                  ? qsTr("SB: %1 | Start cash: %2")
                                    .arg(lobbyPage.selectedGame.firstSmallBlind || 10)
                                    .arg(lobbyPage.selectedGame.startMoney || 0)
                                  : ""
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 12
                            color: Config.StaticData.palette.secondary.col300
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Label {
                            visible: lobbyPage.selectedGame !== null
                            text: qsTr("Players in game (%1)").arg(lobbyPage.selectedGamePlayers.length)
                            font.family: Config.StaticData.loadedFont.font.family
                            font.bold: true
                            font.pixelSize: 12
                            color: Config.StaticData.palette.secondary.col200
                        }

                        ListView {
                            visible: lobbyPage.selectedGame !== null
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: lobbyPage.selectedGamePlayers
                            spacing: 4

                            delegate: Rectangle {
                                required property var modelData
                                width: ListView.view ? ListView.view.width : 0
                                height: 30
                                radius: 4
                                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.1)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 6
                                    anchors.rightMargin: 6
                                    spacing: 6

                                    Rectangle {
                                        Layout.preferredWidth: 20
                                        Layout.preferredHeight: 20
                                        radius: 10
                                        color: "transparent"
                                        clip: true

                                        Image {
                                            anchors.fill: parent
                                            visible: (modelData.avatarUrl || "").length > 0
                                            source: modelData.avatarUrl || ""
                                            fillMode: Image.PreserveAspectCrop
                                            smooth: true
                                        }

                                        SvgIcon {
                                            visible: !((modelData.avatarUrl || "").length > 0)
                                            anchors.fill: parent
                                            source: "../resources/pokerth.svg"
                                        }
                                    }

                                    Image {
                                        visible: (modelData.countryCode || "") !== ""
                                        source: (modelData.countryCode || "") !== ""
                                            ? "qrc:/resources/cflags/" + (modelData.countryCode || "").toLowerCase() + ".svg"
                                            : ""
                                        Layout.preferredWidth: 16
                                        Layout.preferredHeight: 12
                                        fillMode: Image.PreserveAspectFit
                                        smooth: true
                                    }

                                    Text {
                                        text: modelData.playerName || ""
                                        font.family: Config.StaticData.loadedFont.font.family
                                        font.pixelSize: 11
                                        color: (modelData.isAdmin === true)
                                            ? Config.StaticData.chartColor(3, true)
                                            : Config.StaticData.palette.secondary.col200
                                        font.bold: modelData.isAdmin === true
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }

                // Lobby Chat
                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: lobbyRightSplit.height / 3
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5

                    PanelShadow { anchors.fill: parent; radius: parent.radius; color: parent.color }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 5

                        Label {
                            text: qsTr("Lobby Chat")
                            font.family: Config.StaticData.loadedFont.font.family
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        SectionDivider {}

                        ChatBox {
                            id: lobbyChatBoxWide
                            historyStore: Config.StaticData.lobbyChatHistory
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            chatModel: (typeof Lobby !== "undefined" && Lobby) ? Lobby.chatLog : []
                            // Vollständige (ungefilterte) Spielerliste für die
                            // Tab-Vervollständigung – damit auch Spieler in
                            // (offenen) Spielen vervollständigt werden, die der
                            // Spielerlisten-Filter ausblenden kann.
                            nickList: {
                                var _r = (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerListRevision : 0
                                return (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerNickList() : []
                            }
                            pickerInlineHeight: 150
                            onSendRequested: (text) => {
                                if (typeof Lobby !== "undefined" && Lobby)
                                    Lobby.sendChatMessage(text)
                            }
                        }
                    }
                }
            }
        }

        // Status bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Compact: eine Zeile mit Kurzform + elide
            Label {
                visible: Config.Responsive.compact
                Layout.fillWidth: true
                text: qsTr("%1 players · %2 running · %3 open")
                      .arg(connectedPlayers).arg(runningGames).arg(openGames)
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                color: Config.StaticData.palette.secondary.col300
                elide: Text.ElideRight
            }

            // Wide: einzelne Labels
            Label {
                visible: !Config.Responsive.compact
                text: qsTr("connected players: %1").arg(connectedPlayers)
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                color: Config.StaticData.palette.secondary.col300
            }
            Label {
                visible: !Config.Responsive.compact
                text: " | " + qsTr("running games: %1").arg(runningGames)
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                color: Config.StaticData.palette.secondary.col300
            }
            Label {
                visible: !Config.Responsive.compact
                text: " | " + qsTr("open games: %1").arg(openGames)
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
    }

    // Accumulated HTML for the chat areas (avoids full-document reassignment)

    Connections {
        target: Lobby
        function onLobbyChatMentionDetected() {
            // TODO: play lobbychatnotify sound (requires SoundEffect + runtime path)
        }
    }


    // ── Preview / Demo sequence ────────────────────────────────────────────
    SequentialAnimation {
        id: previewSequence
        running: false

        // 1. Lobby sichtbar — kurz einatmen
        PauseAnimation { duration: 600 }

        // 2. Spielerliste einblenden (slide from left)
        ScriptAction { script: lobbyPage.showingPlayerList = true }
        PauseAnimation { duration: 1200 }

        // 3. Spielerliste schließen
        ScriptAction { script: lobbyPage.showingPlayerList = false }
        PauseAnimation { duration: 700 }

        // 4. Erstes Spiel selektieren + Game-Info einblenden (slide from right)
        ScriptAction {
            script: {
                var game = (typeof Lobby !== "undefined" && Lobby
                            && Lobby.gameListModel
                            && Lobby.gameListModel.rowCount() > 0)
                    ? Lobby.gameListModel.get(0)
                    : ({ gameName: "My Online Game", gameId: 1,
                         playerCount: 3, maxPlayers: 10 })
                lobbyPage.selectedGame = game
                lobbyPage.showingGameInfo  = true
            }
        }
        PauseAnimation { duration: 1500 }

        // 5. Game-Info schließen
        ScriptAction {
            script: {
                lobbyPage.showingGameInfo = false
                lobbyPage.selectedGame    = null
            }
        }
        PauseAnimation { duration: 500 }

        // 6. CUT — zurück zur vorherigen Seite
        ScriptAction { script: StackView.view.pop() }
    }

    // ── Bestätigungs-Popups für Spiel-Kontextaktionen ────────────────────────
    ConfirmPopup {
        id: reportGamePopup
        onConfirmed: {
            if (Lobby && lobbyPage.selectedGame)
                Lobby.reportGameName(lobbyPage.selectedGame.gameId)
        }
    }

    ConfirmPopup {
        id: closeGamePopup
        onConfirmed: {
            if (Lobby && lobbyPage.selectedGame)
                Lobby.adminCloseGame(lobbyPage.selectedGame.gameId)
        }
    }

    // ── Passwort-Popup für private Spiele ────────────────────────────────────
    Popup {
        id: joinPasswordPopup
        anchors.centerIn: parent
        modal: true
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property int pendingGameId: 0

        background: Rectangle {
            color: Config.StaticData.palette.secondary.col700
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: Math.min(lobbyPage.width * 0.85, 360)

            Label {
                Layout.fillWidth: true
                text: qsTr("Privates Spiel")
                color: Config.StaticData.palette.secondary.col100
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Bitte das Passwort eingeben, um beizutreten.")
                color: Config.StaticData.palette.secondary.col300
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            TextField {
                id: joinPasswordField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: qsTr("Passwort …")
                font.family: Config.StaticData.loadedFont.font.family
                color: Config.StaticData.palette.secondary.col100
                background: Rectangle {
                    radius: 6
                    color: Config.StaticData.palette.secondary.col600
                    border.color: joinPasswordField.activeFocus
                        ? Config.Theme.withAlpha(Config.Theme.colorAccent, 0.85)
                        : Config.StaticData.palette.secondary.col400
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: 140 } }
                }
                placeholderTextColor: Config.StaticData.palette.secondary.col400
                Keys.onReturnPressed: joinPasswordPopup.doJoin()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CustomButton {
                    text: qsTr("Abbrechen")
                    Layout.fillWidth: true
                    onClicked: joinPasswordPopup.close()
                }
                CustomButton {
                    text: qsTr("Beitreten")
                    Layout.fillWidth: true
                    onClicked: joinPasswordPopup.doJoin()
                }
            }
        }

        function doJoin() {
            Lobby.joinGame(pendingGameId, joinPasswordField.text)
            joinPasswordField.clear()
            close()
        }
        onOpened: {
            joinPasswordField.clear()
            joinPasswordField.forceActiveFocus()
        }
    }

    // ── Eingehende Spiel-Einladung (Invite-Only-Spiele) ────────────────────
    // Ja/Nein-Popup analog zum Qt-Widgets-Client. Schließen ohne Auswahl
    // (Escape) = ablehnen, damit der Einladende eine Antwort erhält.
    Popup {
        id: inviteGamePopup
        anchors.centerIn: parent
        modal: true
        padding: 20
        closePolicy: Popup.CloseOnEscape

        property int inviteGameId: 0
        property string inviteGameName: ""
        property string inviteFromName: ""
        property bool answered: false

        background: Rectangle {
            color: Config.StaticData.palette.secondary.col700
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: Math.min(lobbyPage.width * 0.85, 360)

            Label {
                Layout.fillWidth: true
                text: qsTr("Game invitation")
                color: Config.StaticData.palette.secondary.col100
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("You have been invited to the game <b>%1</b> by <b>%2</b>.<br>Would you like to join this game?")
                      .arg(inviteGamePopup.inviteGameName)
                      .arg(inviteGamePopup.inviteFromName)
                textFormat: Text.RichText
                color: Config.StaticData.palette.secondary.col200
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                CustomButton {
                    text: qsTr("Decline")
                    Layout.fillWidth: true
                    onClicked: inviteGamePopup.respond(false)
                }
                CustomButton {
                    text: qsTr("Join")
                    Layout.fillWidth: true
                    onClicked: inviteGamePopup.respond(true)
                }
            }
        }

        function respond(accepted) {
            console.log("[INVITE QML] respond: accepted=" + accepted + " gameId=" + inviteGameId)
            answered = true
            if (accepted)
                Lobby.acceptGameInvitation(inviteGameId)
            else
                Lobby.rejectGameInvitation(inviteGameId, 0)   // 0 = DENY_GAME_INVITATION_NO
            close()
        }
        onOpened: {
            console.log("[INVITE QML] popup opened: gameId=" + inviteGameId + " game=" + inviteGameName + " from=" + inviteFromName)
        }
        onClosed: {
            console.log("[INVITE QML] popup closed: answered=" + answered + " gameId=" + inviteGameId)
            // Ohne Auswahl geschlossen (Escape) → ablehnen, damit Server- und
            // Pending-State sauber zurückgesetzt werden.
            if (!answered)
                Lobby.rejectGameInvitation(inviteGameId, 0)
        }
    }

    Connections {
        target: Lobby
        function onGameInvitationReceived(gameId, gameName, fromName) {
            console.log("[INVITE QML] onGameInvitationReceived: gameId=" + gameId + " game=" + gameName + " from=" + fromName)
            // Slide-in-Panels einklappen, damit das Popup im Compact-Mode
            // nicht verdeckt wird und korrekt bedienbar ist.
            lobbyPage.showingPlayerList = false
            lobbyPage.showingGameInfo = false
            inviteGamePopup.inviteGameId = gameId
            inviteGamePopup.inviteGameName = gameName
            inviteGamePopup.inviteFromName = fromName
            inviteGamePopup.answered = false
            inviteGamePopup.open()
        }
    }

    Component.onCompleted: {
        console.log("LobbyPage loaded")
        console.log("My player name:", Lobby.myPlayerName)
        console.log("Player model count:", Lobby.playerListModel.rowCount())
        console.log("Game model count:", Lobby.gameListModel.rowCount())
        // Bereits vorhandenen Chat-Verlauf wiederherstellen (z.B. nach Rückkehr
        // aus einem Spiel).
    }
}
