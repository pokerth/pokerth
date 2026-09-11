import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config
import "../components"

Rectangle {
    id: gameWaitPage
    objectName: "gameWaitPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // Refresh when the player list or the game list changes
    readonly property int gameRev: Lobby ? Lobby.gameListRevision : 0
    readonly property int playerRev: Lobby ? Lobby.playerListRevision : 0

    readonly property var players: {
        var _g = gameRev; var _p = playerRev
        return (Lobby && Lobby.currentGameId) ? Lobby.gamePlayersInGame(Lobby.currentGameId) : []
    }
    readonly property var info: {
        var _g = gameRev
        return Lobby ? Lobby.currentGameInfo() : ({})
    }
    // Game admin (host/creator): isCurrentGameAdmin comes from the self join,
    // adminPlayerId from the game info (fallback/confirmation). Only they may
    // kick players – the server checks session->IsGameAdmin() (servergamestate).
    readonly property bool isGameAdmin: Lobby
        && (Lobby.isCurrentGameAdmin
            || (info.adminPlayerId !== undefined && info.adminPlayerId === Lobby.myPlayerId))
    // Admin = the game admin or a server admin reported by the server → may start.
    readonly property bool isAdmin: Lobby && (Lobby.isCurrentPlayerAdmin || isGameAdmin)
    readonly property bool isRanking: (info.gameType || 1) === 4
    // If a hand is already running (a rejoin waiting state), there is nothing to start.
    readonly property bool canStart: isAdmin && !isRanking && players.length >= 2
                                     && !(Lobby && Lobby.rejoinWaiting)
    readonly property bool canKick: isGameAdmin && !isRanking

    // ── Community "suggest" (ported from the legacy bbcbot) ──────────────────
    // Two kinds of audience may suggest, in an invite game (GameType 3) and
    // only with the community settings enabled:
    //   • the CREATOR of their own table – their type comes EXPLICITLY from the
    //     preset (Config.BotSuggest.createdSuggestType, set when creating);
    //   • every COMMUNITY ADMIN at a foreign table of their community (BBC
    //     step or WEC) – there the type is not known and is derived from the
    //     game settings (Config.BotSuggest.suggestTypeForGameInfo;
    //     the table name is freely editable and is deliberately NOT consulted).
    // The admin match (bbcadmins.txt / wecadmins.txt) runs asynchronously and
    // only once the purely local fingerprint already delivers a suggest type –
    // at all other tables the feature costs no request.
    readonly property bool communitySuggestEnabled: Config.Parameters.showCommunityContent
        && Config.Parameters.showCommunitySuggest
        && (info.gameType || 1) === 3
    // The type of your own (created) table or the type recognised from the settings.
    readonly property string ownSuggestType:
        (isGameAdmin && Config.BotSuggest.isSuggestType(Config.BotSuggest.createdSuggestType))
        ? Config.BotSuggest.createdSuggestType : ""
    readonly property string tableSuggestType: communitySuggestEnabled
        ? Config.BotSuggest.suggestTypeForGameInfo(info) : ""
    // For foreign admins every recognised community type (BBC step,
    // WEC) is enabled; Config.BotSuggest picks the responsible admin list by the type.
    readonly property bool needsCommunityAdminCheck: communitySuggestEnabled
        && ownSuggestType.length === 0
        && Config.BotSuggest.isSuggestType(tableSuggestType)
        && !(Lobby && Lobby.isMyPlayerGuest)
    property bool communityAdmin: false
    readonly property string effectiveSuggestType: ownSuggestType.length > 0
        ? ownSuggestType : (communityAdmin ? tableSuggestType : "")
    readonly property bool canSuggest: communitySuggestEnabled
                                       && effectiveSuggestType.length > 0
    property bool suggestBusy: false

    // A mere change of the type (step ⇄ WEC) has to re-match as well: the two
    // communities keep separate admin lists.
    onNeedsCommunityAdminCheckChanged: resolveCommunityAdmin()
    onTableSuggestTypeChanged: resolveCommunityAdmin()
    Component.onCompleted: resolveCommunityAdmin()

    // It is only started on a change to "foreign community table". The cache and
    // the failure throttling live in Config.BotSuggest, so entering again
    // usually costs no network.
    function resolveCommunityAdmin() {
        // An admin flag set earlier only applies to the type it was
        // determined for – otherwise a BBC admin would enable the button at a
        // foreign WEC table (and the other way round).
        communityAdmin = false
        if (!needsCommunityAdminCheck || !Lobby)
            return
        var gameId = Lobby.currentGameId
        var type = tableSuggestType
        Config.BotSuggest.isCommunityAdmin(type, Lobby.myPlayerName, function(isAdmin) {
            // A late hit after a table change must not enable the button.
            if (Lobby && Lobby.currentGameId === gameId
                && gameWaitPage.tableSuggestType === type)
                gameWaitPage.communityAdmin = isAdmin
        })
    }

    function runSuggest() {
        if (!Lobby || suggestBusy || effectiveSuggestType.length === 0)
            return
        suggestBusy = true
        Config.BotSuggest.suggestForType(effectiveSuggestType,
            Lobby.idlePlayerNames(), Lobby.playingPlayerEntries(),
            function(ok, message) {
                gameWaitPage.suggestBusy = false
                // Only show it locally at the trigger (like the PM reply of the
                // bbcbot to the requester) – do not broadcast it into the lobby.
                if (ok && message.length > 0)
                    Lobby.postLocalChatNote(message)
            })
    }

    // NTF_NET_REMOVED_ON_REQUEST (socket_msg.h) – selbst angefordertes Verlassen
    readonly property int removedOnRequest: 202

    // Confirmation before the game admin throws a player out of the open game.
    function confirmKick(playerId, playerName) {
        kickPopup.targetPlayerId = playerId
        kickPopup.openWith(
            qsTr("Kick player"),
            qsTr("Are you sure you want to kick \"%1\" from the game?").arg(playerName),
            qsTr("Kick"))
    }

    // Portrait-mode overlay state
    property bool showingPlayerList: false
    property int playerListCollapseResetCounter: 0
    property bool showingGameList: false
    property int gameListCollapseResetCounter: 0

    function resetPlayerListDelegates() {
        playerListCollapseResetCounter += 1
        waitPagePlayerPanelList.currentIndex = -1
        waitPagePlayerSidebarList.currentIndex = -1
    }

    // A helper analogous to LobbyPage
    function gameTypeIconSource(gameType) {
        if (gameType === 2) return "../resources/userSquare.svg"
        if (gameType === 3) return "../resources/users.svg"
        if (gameType === 4) return "../resources/chipStack.svg"
        return "../resources/user.svg"
    }


    Connections {
        target: Lobby
        function onRemovedFromGame(reason) {
            // console.log("[NAV] GameWaitPage.onRemovedFromGame | reason:", reason, "| depth before:", mainStackView.depth, "| currentItem:", mainStackView.currentItem ? (mainStackView.currentItem.objectName || mainStackView.currentItem.toString()) : "null")
            if (reason === gameWaitPage.removedOnRequest) {
                var lobby = mainStackView.find(function(item) {
                    return item && item.objectName === "lobbyPage"
                })
                if (lobby)
                    mainStackView.pop(lobby)
                else
                    mainStackView.pop()
            } else {
                mainStackView.pop()
            }
            // console.log("[NAV] GameWaitPage.onRemovedFromGame | depth after:", mainStackView.depth)
        }
        function onGameStarted() {
            // console.log("[NAV] GameWaitPage.onGameStarted → pushing GamePage")
            // Double push protection (the pattern of openTableStatsPage/onPlayerStats-
            // Requested): if gameStarted arrives a second time (rejoin/reconnect),
            // a second GamePage would otherwise lie in the stack. Its shortcuts are
            // disabled via topGamePage, but the table itself would be there twice.
            if (mainStackView.currentItem
                    && mainStackView.currentItem.objectName === "gamePage")
                return
            mainStackView.push("GamePage.qml")
        }
        function onReturnToWaitRoom() {
            // End of the game (server: WaitDialog): close the game table and go back
            // into the waiting room of this (possibly reopened) game. Pop up to
            // this GameWaitPage – that also covers the case that the SettingsPage
            // still lies above the game table.
            //
            // WITHOUT a transition animation (StackView.Immediate): with auto-leave active
            // the engine sends RemovedFromGame right after the WaitDialog →
            // onRemovedFromGame would pop a second time immediately. If the
            // first pop were still running as a transition, the StackView would discard the second
            // ("cannot pop while in transition") and you would stay stuck in the waiting room
            // instead of landing in the lobby list.
            // console.log("[NAV] GameWaitPage.onReturnToWaitRoom | depth before:", mainStackView.depth)
            if (mainStackView.currentItem !== gameWaitPage)
                mainStackView.pop(gameWaitPage, StackView.Immediate)
            // console.log("[NAV] GameWaitPage.onReturnToWaitRoom | depth after:", mainStackView.depth)
        }
        function onGameListFilterModeChanged() {
            if (gameListFilterPanel.currentIndex !== Lobby.gameListFilterMode)
                gameListFilterPanel.currentIndex = Lobby.gameListFilterMode
            if (gameListFilterSidebar.currentIndex !== Lobby.gameListFilterMode)
                gameListFilterSidebar.currentIndex = Lobby.gameListFilterMode
        }
        function onPlayerListFilterModeChanged() {
            if (playerListFilterCompact.currentIndex !== Lobby.playerListFilterMode)
                playerListFilterCompact.currentIndex = Lobby.playerListFilterMode
            if (playerListFilterWide.currentIndex !== Lobby.playerListFilterMode)
                playerListFilterWide.currentIndex = Lobby.playerListFilterMode
            gameWaitPage.resetPlayerListDelegates()
        }
    }

    // ── Compact: Player list panel (slides in from left) ─────────────────
    Rectangle {
        id: playerPanel
        width: gameWaitPage.width
        height: gameWaitPage.height
        y: 0
        x: gameWaitPage.showingPlayerList ? 0 : -width
        z: 3
        color: Config.Theme.colorBox
        visible: Config.Responsive.compact

        Behavior on x {
            NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
        }

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

                AppLabel {
                    text: qsTr("Players")
                    font.bold: true
                    font.pixelSize: 15
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 30
                    height: 30
                    radius: 4
                    color: closePanelArea.containsMouse
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
                        id: closePanelArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: gameWaitPage.showingPlayerList = false
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Config.StaticData.palette.secondary.col500
            }

            TextField {
                id: panelSearchField
                Layout.fillWidth: true
                placeholderText: qsTr("search for player ...")
                font.family: Config.StaticData.loadedFont.font.family
                color: Config.StaticData.palette.secondary.col200
                background: Rectangle {
                    color: Config.Theme.colorField
                    radius: 3
                }
                placeholderTextColor: Qt.lighter(Config.StaticData.palette.secondary.col200, 1.5)
            }

            ListView {
                id: waitPagePlayerPanelList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                property int expandedPlayerIndex: -1
                model: Lobby ? Lobby.playerListProxyModel : null

                delegate: PlayerListItem {
                    collapseResetCounter: gameWaitPage.playerListCollapseResetCounter
                    listView: waitPagePlayerPanelList
                    searchFilter: panelSearchField.text
                    onPrivateMessageRequested: (playerId, playerName) =>
                        mainWindow.openPrivateMessages(playerName)
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
                    if (Lobby && Lobby.playerListFilterMode !== currentIndex) {
                        Lobby.playerListFilterMode = currentIndex
                        gameWaitPage.resetPlayerListDelegates()
                    }
                }
            }
        }
    }

    // ── Compact: Game list panel (slides in from right) ───────────────────
    Rectangle {
        id: gameListPanel
        width: gameWaitPage.width
        height: gameWaitPage.height
        y: 0
        x: gameWaitPage.showingGameList ? 0 : gameWaitPage.width
        z: 3
        color: Config.Theme.colorBox
        visible: Config.Responsive.compact

        Behavior on x {
            NumberAnimation { duration: 260; easing.type: Easing.OutCubic }
        }

        Rectangle {
            anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
            width: 1
            color: Config.StaticData.palette.secondary.col500
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 6

            RowLayout {
                Layout.fillWidth: true

                AppLabel {
                    text: qsTr("Game List")
                    font.bold: true
                    font.pixelSize: 15
                    color: Config.StaticData.palette.secondary.col200
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 30
                    height: 30
                    radius: 4
                    color: closeGameListPanelArea.containsMouse
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
                        id: closeGameListPanelArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: gameWaitPage.showingGameList = false
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Config.StaticData.palette.secondary.col500
            }

            ComboBox {
                id: gameListFilterPanel
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
                    if (Lobby && Lobby.gameListFilterMode !== currentIndex)
                        Lobby.gameListFilterMode = currentIndex
                }
            }

            ListView {
                id: waitPageGamePanelList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: Lobby ? Lobby.gameListProxyModel : null

                delegate: GameListItem {
                    collapseResetCounter: gameWaitPage.gameListCollapseResetCounter
                    listView: waitPageGamePanelList
                    searchFilter: ""
                    gameRevision: gameWaitPage.gameRev
                }
            }
        }
    }

    // ── Main layout ────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Config.Theme.margin
        spacing: Config.Theme.spacing

        // ── Header ────────────────────────────────────────────────────────
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
                       : Config.Theme.colorBox
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
                    onClicked: gameWaitPage.showingPlayerList = !gameWaitPage.showingPlayerList
                }
            }

            AppLabel {
                text: qsTr("Game Info")
                font.bold: true
                font.pixelSize: 16
                color: Config.StaticData.palette.secondary.col200
                Layout.fillWidth: true
            }

            // As a spectator - and likewise after an accepted rejoin - the game is
            // already running: the server only puts us at the table at the beginning of the
            // next hand (then the GamePage is pushed).
            // Until then we wait here.
            //
            // The travelling dots replace the earlier ellipsis "…" at the
            // end of the text – both together would be redundant.
            Column {
                Layout.fillWidth: true
                spacing: 2

                AppLabel {
                    id: waitLabel
                    text: (Lobby && Lobby.rejoinWaiting)
                          ? qsTr("Waiting for the start of the next hand to rejoin the game")
                          : (Lobby && Lobby.isSpectating)
                            ? qsTr("Spectating — waiting for the next hand")
                            : qsTr("Waiting for players")
                    color: Config.StaticData.palette.secondary.col300
                    font.pixelSize: 12
                }

                // The same "spinner" as in the splash (PreLoader): the universal
                // ProgressBar draws travelling dots in indeterminate mode.
                // Only as wide as the text above it, in its colour – that way it looks
                // like a lively underline.
                ProgressBar {
                    indeterminate: true
                    width: waitLabel.implicitWidth
                    Universal.accent: Config.StaticData.palette.secondary.col300
                }
            }

            // Compact: game list toggle button (top-right)
            Rectangle {
                visible: Config.Responsive.compact
                implicitWidth: 38
                implicitHeight: 38
                radius: 5
                color: gameListToggleArea.containsMouse
                       ? Config.StaticData.palette.secondary.col600
                       : Config.Theme.colorBox
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1

                Image {
                    anchors.centerIn: parent
                    width: 24
                    height: 24
                    source: "../resources/threeLines.svg"
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
                    id: gameListToggleArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: gameWaitPage.showingGameList = !gameWaitPage.showingGameList
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Config.StaticData.palette.secondary.col500
        }

        // ── Body: wide screen = three resizable columns (player list |
        // game info/chat | game list); compact = only the middle column ──
        SplitView {
            id: waitBodySplit
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal
            handle: ResizeHandle { horizontal: true }

            // Wide: player sidebar (left) – initial 1:2:1 ratio (1/4 width)
            Rectangle {
                visible: !Config.Responsive.compact
                SplitView.preferredWidth: waitBodySplit.width / 4
                SplitView.minimumWidth: 160
                color: Config.Theme.colorPanel
                radius: 5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5

                    AppLabel {
                        text: qsTr("Connected Players")
                        font.bold: true
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ListView {
                        id: waitPagePlayerSidebarList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        property int expandedPlayerIndex: -1
                        model: Lobby ? Lobby.playerListProxyModel : null

                        delegate: PlayerListItem {
                            collapseResetCounter: gameWaitPage.playerListCollapseResetCounter
                            listView: waitPagePlayerSidebarList
                            searchFilter: ""
                            onPrivateMessageRequested: (playerId, playerName) =>
                                mainWindow.openPrivateMessages(playerName)
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
                            if (Lobby && Lobby.playerListFilterMode !== currentIndex) {
                                Lobby.playerListFilterMode = currentIndex
                                gameWaitPage.resetPlayerListDelegates()
                            }
                        }
                    }
                }
            }

            // Main content column
            ColumnLayout {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 300
                spacing: Config.Theme.spacing

                // The game info/player list and the chat are resizable vertically
                // (a minimum height of 1/3 each). The action buttons stay fixed below them.
                SplitView {
                    id: waitContentSplit
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Vertical
                    handle: ResizeHandle { horizontal: false }

                // ── Game details card (game info + player list) ────────────
                // It can be shrunk in favour of the chat; minimum height 1/3.
                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: waitContentSplit.height / 3
                    color: Config.Theme.colorPanel
                    radius: 6

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        // Name
                        AppLabel {
                            text: info.name || ""
                            font.bold: true
                            font.pixelSize: 15
                            color: Config.StaticData.palette.secondary.col100
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        // 2-column info grid
                        GridLayout {
                            columns: 2
                            rowSpacing: 6
                            columnSpacing: 14
                            Layout.fillWidth: true

                            // Players | Type
                            AppLabel {
                                text: qsTr("Players: %1 / %2")
                                      .arg(players.length).arg(info.maxPlayers || 0)
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            RowLayout {
                                spacing: 5
                                Layout.fillWidth: true
                                Image {
                                    Layout.preferredWidth: 14
                                    Layout.preferredHeight: 14
                                    source: gameWaitPage.gameTypeIconSource(info.gameType || 1)
                                    sourceSize: Qt.size(28, 28)
                                    smooth: true; antialiasing: true
                                    layer.enabled: true
                                    layer.effect: MultiEffect {
                                        colorization: 1.0
                                        colorizationColor: Config.StaticData.palette.secondary.col300
                                    }
                                }
                                AppLabel {
                                    text: qsTr("Type: %1").arg(Lobby ? Lobby.gameTypeText(info.gameType || 1) : "")
                                    font.pixelSize: 13
                                    color: Config.StaticData.palette.secondary.col200
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }

                            // Small blind | Start cash
                            AppLabel {
                                text: qsTr("Small blind: %1").arg(info.firstSmallBlind || 0)
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                            AppLabel {
                                text: qsTr("Start cash: %1").arg(info.startMoney || 0)
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }

                            // Blinds interval | Blinds raise mode
                            AppLabel {
                                text: {
                                    var mode = info.raiseIntervalMode || 1
                                    if (mode === 1)
                                        return qsTr("Blinds raise interval: %1 hands").arg(info.raiseEveryHands || 0)
                                    return qsTr("Blinds raise interval: %1 minutes").arg(info.raiseEveryMinutes || 0)
                                }
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            AppLabel {
                                text: qsTr("Blinds raise mode: %1").arg((info.raiseMode || 1) === 1
                                      ? qsTr("double blinds") : qsTr("manual blinds order"))
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            // Action timeout | Hand delay
                            AppLabel {
                                text: qsTr("Action time: %1 sec").arg(info.playerActionTimeoutSec || 0)
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                            AppLabel {
                                text: qsTr("Hand delay: %1 sec").arg(info.delayBetweenHandsSec || 0)
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                        }

                        // ── Spielerliste ─────────────────────────────────
                        AppLabel {
                            text: qsTr("Players in game (%1)").arg(players.length)
                            font.bold: true
                            font.pixelSize: 13
                            color: Config.StaticData.palette.secondary.col100
                        }

                        ListView {
                            id: playerList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            Layout.minimumHeight: 0
                            clip: true
                            model: gameWaitPage.players
                            spacing: 4
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar {
                                policy: playerList.contentHeight > playerList.height + 4
                                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                            }

                            delegate: Rectangle {
                                required property var modelData
                                readonly property bool gameAdmin: !!modelData.isGameAdmin
                                width: playerList.width
                                height: 32
                                radius: 4
                                color: gameAdmin ? Config.Theme.colorGameAdminRow
                                                 : Config.Theme.colorPanelRow

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 6
                                    // Room for the scrollbar when it is visible.
                                    anchors.rightMargin: playerList.contentHeight > playerList.height + 4 ? 14 : 6
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

                                    AppText {
                                        text: modelData.playerName || ""
                                        font.pixelSize: 12
                                        color: Config.StaticData.palette.secondary.col200
                                        font.bold: false
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }

                                    // Highlight the table admin (host) – as in the widget client
                                    GameAdminBadge {
                                        visible: gameAdmin
                                        Layout.alignment: Qt.AlignVCenter
                                    }

                                    // The game admin may throw players out before the
                                    // game starts (as in the widget client game lobby);
                                    // not themselves, of course.
                                    PlayerActionIcon {
                                        visible: gameWaitPage.canKick
                                                 && modelData.playerId !== Lobby.myPlayerId
                                        source: "qrc:/resources/personRemove.svg"
                                        baseColor: Config.StaticData.chartColor(5, true)
                                        tooltipText: qsTr("Kick player")
                                        Layout.alignment: Qt.AlignVCenter
                                        onTriggered: gameWaitPage.confirmKick(
                                            modelData.playerId,
                                            modelData.playerName || "")
                                    }
                                }
                            }
                        }
                    }
                }

                // ── Game chat ──────────────────────────────────────────────
                // Resizable vertically (minimum height 1/3); the initial value ~1/3. The
                // emoji picker no longer enlarges the box but
                // shrinks the message list within the chosen height.
                Rectangle {
                    SplitView.preferredHeight: waitContentSplit.height / 3
                    SplitView.minimumHeight: waitContentSplit.height / 3
                    color: Config.Theme.colorPanel
                    radius: 5
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 5

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            AppLabel {
                                text: qsTr("Lobby Chat")
                                font.bold: true
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }

                            // Suggests matching idle players for your own BBC/WEC invite
                            // game; the result appears in the chat.
                            Rectangle {
                                visible: gameWaitPage.canSuggest
                                implicitHeight: 26
                                implicitWidth: suggestContent.implicitWidth + 16
                                radius: 4
                                opacity: gameWaitPage.suggestBusy ? 0.5 : 1.0
                                color: suggestArea.containsMouse
                                       ? Config.StaticData.palette.secondary.col600
                                       : Config.Theme.colorPanelRow
                                border.width: 1
                                border.color: Config.StaticData.palette.secondary.col500

                                RowLayout {
                                    id: suggestContent
                                    anchors.centerIn: parent
                                    spacing: 5

                                    Image {
                                        Layout.preferredWidth: 15
                                        Layout.preferredHeight: 15
                                        source: "../resources/personAdd.svg"
                                        sourceSize: Qt.size(30, 30)
                                        smooth: true
                                        antialiasing: true
                                        layer.enabled: true
                                        layer.effect: MultiEffect {
                                            colorization: 1.0
                                            colorizationColor: Config.StaticData.palette.secondary.col200
                                        }
                                    }

                                    AppText {
                                        text: qsTr("Suggest players")
                                        font.pixelSize: 12
                                        color: Config.StaticData.palette.secondary.col200
                                    }
                                }

                                MouseArea {
                                    id: suggestArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    enabled: !gameWaitPage.suggestBusy
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: gameWaitPage.runSuggest()
                                }
                            }
                        }

                        ChatBox {
                            id: waitChatBox
                            historyStore: Config.StaticData.lobbyChatHistory
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            chatModel: (typeof Lobby !== "undefined" && Lobby) ? Lobby.chatLog : []
                            chatTranslator: (typeof Lobby !== "undefined" && Lobby) ? Lobby.chatTranslator : null
                            // This chat is the lobby chat → complete against the full
                            // (unfiltered) lobby player list,
                            // not only against the players sitting at the table.
                            nickList: {
                                var _r = (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerListRevision : 0
                                return (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerNickList() : []
                            }
                            inputEnabled: !(Lobby && Lobby.isMyPlayerGuest)
                            placeholder: (Lobby && Lobby.isMyPlayerGuest)
                                         ? qsTr("Guests cannot chat")
                                         : qsTr("Type your message...")
                            // A 2 row inline picker (little room in the card)
                            pickerInlineHeight: 2 * 38 + 2 * 6
                            onSendRequested: (text) => {
                                if (typeof Lobby !== "undefined" && Lobby)
                                    Lobby.sendChatMessage(text)
                            }
                        }
                    }
                }
                } // waitContentSplit

                // ── Aktionen ──────────────────────────────────────────────
                RowLayout {
                    visible: gameWaitPage.isAdmin && !gameWaitPage.isRanking
                    Layout.fillWidth: true
                    spacing: 8

                    CheckBox {
                        id: fillCpuCheck
                        text: qsTr("Fill up with computer players")
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        checked: false
                        contentItem: Text {
                            text: fillCpuCheck.text
                            font: fillCpuCheck.font
                            color: Config.StaticData.palette.secondary.col200
                            leftPadding: fillCpuCheck.indicator.width + fillCpuCheck.spacing
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    CustomButton {
                        // During the rejoin synchronization the server waits
                        // for us; leaving in this window would block the
                        // running hand (the widgets client locks the
                        // leave button at the same place).
                        enabled: !(Lobby && Lobby.rejoinWaiting)
                        text: qsTr("Leave Game")
                        Layout.fillWidth: true
                        onClicked: {
                            if (Lobby) Lobby.leaveGame()
                        }
                    }

                    CustomButton {
                        visible: gameWaitPage.isAdmin && !gameWaitPage.isRanking
                        enabled: gameWaitPage.canStart
                        text: qsTr("Start Game")
                        Layout.fillWidth: true
                        onClicked: {
                            if (Lobby) Lobby.startGame(fillCpuCheck.checked)
                        }
                    }
                }
            }

            // Wide: game list (right column) – initial 1:2:1 ratio (1/4 width)
            Rectangle {
                visible: !Config.Responsive.compact
                SplitView.preferredWidth: waitBodySplit.width / 4
                SplitView.minimumWidth: 160
                color: Config.Theme.colorPanel
                radius: 5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5

                    AppLabel {
                        text: qsTr("Game List")
                        font.bold: true
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ComboBox {
                        id: gameListFilterSidebar
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
                            if (Lobby && Lobby.gameListFilterMode !== currentIndex)
                                Lobby.gameListFilterMode = currentIndex
                        }
                    }

                    ListView {
                        id: waitPageGameSidebarList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: Lobby ? Lobby.gameListProxyModel : null

                        delegate: GameListItem {
                            collapseResetCounter: gameWaitPage.gameListCollapseResetCounter
                            listView: waitPageGameSidebarList
                            searchFilter: ""
                            gameRevision: gameWaitPage.gameRev
                        }
                    }
                }
            }
        }
    }

    ConfirmPopup {
        id: kickPopup
        property int targetPlayerId: 0
        onConfirmed: { if (Lobby) Lobby.kickPlayer(targetPlayerId) }
    }
}
