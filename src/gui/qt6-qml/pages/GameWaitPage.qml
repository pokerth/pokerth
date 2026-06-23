import QtQuick
import QtQuick.Controls
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

    // Refresh wenn sich Spielerliste oder Spielliste ändern
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
    // Admin = entweder vom Server gemeldeter Server-Admin oder ich bin der
    // Spiel-Admin (Host/Ersteller) → darf starten. isCurrentGameAdmin kommt vom
    // Selbst-Beitritt, adminPlayerId aus der Spiel-Info (Fallback/Bestätigung).
    readonly property bool isAdmin: Lobby
        && (Lobby.isCurrentPlayerAdmin
            || Lobby.isCurrentGameAdmin
            || (info.adminPlayerId !== undefined && info.adminPlayerId === Lobby.myPlayerId))
    readonly property bool isRanking: (info.gameType || 1) === 4
    readonly property bool canStart: isAdmin && !isRanking && players.length >= 2

    // NTF_NET_REMOVED_ON_REQUEST (socket_msg.h) – selbst angefordertes Verlassen
    readonly property int removedOnRequest: 202

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

    // Helfer analog zu LobbyPage
    function gameTypeIconSource(gameType) {
        if (gameType === 2) return "../resources/userSquare.svg"
        if (gameType === 3) return "../resources/users.svg"
        if (gameType === 4) return "../resources/chipStack.svg"
        return "../resources/user.svg"
    }


    Connections {
        target: Lobby
        function onRemovedFromGame(reason) {
            console.log("[NAV] GameWaitPage.onRemovedFromGame | reason:", reason, "| depth before:", mainStackView.depth, "| currentItem:", mainStackView.currentItem ? (mainStackView.currentItem.objectName || mainStackView.currentItem.toString()) : "null")
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
            console.log("[NAV] GameWaitPage.onRemovedFromGame | depth after:", mainStackView.depth)
        }
        function onGameStarted() {
            console.log("[NAV] GameWaitPage.onGameStarted → pushing GamePage")
            mainStackView.push("GamePage.qml")
        }
        function onReturnToWaitRoom() {
            // Spielende (Server: WaitDialog): den Gametable schließen und zurück
            // in den Warteraum dieses (ggf. wieder geöffneten) Spiels. Bis zu
            // diesem GameWaitPage poppen – das deckt auch den Fall ab, dass über
            // dem Gametable noch die SettingsPage liegt.
            //
            // OHNE Übergangsanimation (StackView.Immediate): Bei aktivem Auto-Leave
            // sendet die Engine direkt nach dem WaitDialog noch RemovedFromGame →
            // onRemovedFromGame würde sofort ein zweites Mal poppen. Liefe der
            // erste Pop noch als Transition, verwürfe StackView den zweiten
            // ("cannot pop while in transition") und man bliebe im Warteraum
            // hängen statt in der Lobbyliste zu landen.
            console.log("[NAV] GameWaitPage.onReturnToWaitRoom | depth before:", mainStackView.depth)
            if (mainStackView.currentItem !== gameWaitPage)
                mainStackView.pop(gameWaitPage, StackView.Immediate)
            console.log("[NAV] GameWaitPage.onReturnToWaitRoom | depth after:", mainStackView.depth)
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
        color: Config.StaticData.palette.secondary.col700
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
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.3)
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
                    visible: (panelSearchField.text.length === 0 ||
                             displayName.toLowerCase().includes(panelSearchField.text.toLowerCase()))
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
        color: Config.StaticData.palette.secondary.col700
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

                Label {
                    text: qsTr("Game List")
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
                    onClicked: gameWaitPage.showingPlayerList = !gameWaitPage.showingPlayerList
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

            Label {
                text: qsTr("Waiting for players …")
                color: Config.StaticData.palette.secondary.col300
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
            }

            // Compact: game list toggle button (top-right)
            Rectangle {
                visible: Config.Responsive.compact
                implicitWidth: 38
                implicitHeight: 38
                radius: 5
                color: gameListToggleArea.containsMouse
                       ? Config.StaticData.palette.secondary.col600
                       : Config.StaticData.palette.secondary.col700
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

        // ── Body: widescreen = drei resizable Spalten (Spielerliste |
        // Game-Info/Chat | Spielliste); compact = nur die Mittelspalte ──
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
                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                radius: 5

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

                // Game-Info/Spielerliste und Chat sind vertikal resizable
                // (Min-Höhe je 1/3). Die Aktions-Buttons bleiben darunter fix.
                SplitView {
                    id: waitContentSplit
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Vertical
                    handle: ResizeHandle { horizontal: false }

                // ── Game details card (Game-Info + Spielerliste) ───────────
                // Kann zugunsten des Chats verkleinert werden; Min-Höhe 1/3.
                Rectangle {
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: waitContentSplit.height / 3
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 6

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10

                        // Name
                        Label {
                            text: info.name || ""
                            font.family: Config.StaticData.loadedFont.font.family
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
                            Label {
                                text: qsTr("Players: %1 / %2")
                                      .arg(players.length).arg(info.maxPlayers || 0)
                                font.family: Config.StaticData.loadedFont.font.family
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
                                Label {
                                    text: qsTr("Type: %1").arg(Lobby ? Lobby.gameTypeText(info.gameType || 1) : "")
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: 13
                                    color: Config.StaticData.palette.secondary.col200
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }

                            // Small blind | Start cash
                            Label {
                                text: qsTr("Small blind: %1").arg(info.firstSmallBlind || 0)
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("Start cash: %1").arg(info.startMoney || 0)
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }

                            // Blinds interval | Blinds raise mode
                            Label {
                                text: {
                                    var mode = info.raiseIntervalMode || 1
                                    if (mode === 1)
                                        return qsTr("Blinds raise interval: %1 hands").arg(info.raiseEveryHands || 0)
                                    return qsTr("Blinds raise interval: %1 minutes").arg(info.raiseEveryMinutes || 0)
                                }
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            Label {
                                text: qsTr("Blinds raise mode: %1").arg((info.raiseMode || 1) === 1
                                      ? qsTr("double blinds") : qsTr("manual blinds order"))
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            // Action timeout | Hand delay
                            Label {
                                text: qsTr("Action time: %1 sec").arg(info.playerActionTimeoutSec || 0)
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("Hand delay: %1 sec").arg(info.delayBetweenHandsSec || 0)
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 13
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                        }

                        // ── Spielerliste ─────────────────────────────────
                        Label {
                            text: qsTr("Players in game (%1)").arg(players.length)
                            font.family: Config.StaticData.loadedFont.font.family
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
                                width: playerList.width
                                height: 32
                                radius: 4
                                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.1)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 6
                                    // Platz für die Scrollbar, wenn sie sichtbar ist.
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

                                    Text {
                                        text: modelData.playerName || ""
                                        font.family: Config.StaticData.loadedFont.font.family
                                        font.pixelSize: 12
                                        color: Config.StaticData.palette.secondary.col200
                                        font.bold: false
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }
                }

                // ── Game-Chat ──────────────────────────────────────────────
                // Vertikal resizable (Min-Höhe 1/3); Startwert ~1/3. Der
                // Emoji-Picker vergrößert nicht mehr die Box, sondern
                // verkleinert die Nachrichtenliste innerhalb der gewählten Höhe.
                Rectangle {
                    SplitView.preferredHeight: waitContentSplit.height / 3
                    SplitView.minimumHeight: waitContentSplit.height / 3
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5
                    clip: true

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

                        ChatBox {
                            id: waitChatBox
                            historyStore: Config.StaticData.lobbyChatHistory
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            chatModel: (typeof Lobby !== "undefined" && Lobby) ? Lobby.chatLog : []
                            // Dieser Chat ist der Lobby-Chat → gegen die volle
                            // (ungefilterte) Lobby-Spielerliste vervollständigen,
                            // nicht nur gegen die am Tisch sitzenden Spieler.
                            nickList: {
                                var _r = (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerListRevision : 0
                                return (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerNickList() : []
                            }
                            inputEnabled: !(Lobby && Lobby.isMyPlayerGuest)
                            placeholder: (Lobby && Lobby.isMyPlayerGuest)
                                         ? qsTr("Guests cannot chat")
                                         : qsTr("Type your message...")
                            // 2-zeiliger Inline-Picker (wenig Platz in der Karte)
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
                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                radius: 5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5

                    Label {
                        text: qsTr("Game List")
                        font.family: Config.StaticData.loadedFont.font.family
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
}
