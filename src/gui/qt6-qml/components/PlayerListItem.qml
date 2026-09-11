import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

ItemDelegate {
    id: playerItem

    required property int index
    required property int collapseResetCounter
    required property var listView
    required property string searchFilter

    readonly property int playerListRevision: Lobby ? Lobby.playerListRevision : 0
    readonly property var playerEntry: {
        var _revision = playerListRevision
        return Lobby ? Lobby.playerListEntry(index) : ({})
    }
    readonly property int targetPlayerId: playerEntry.playerId || 0
    readonly property string displayName: playerEntry.playerName || ""
    readonly property bool adminPlayer: !!playerEntry.isAdmin
    readonly property bool guestPlayer: !!playerEntry.isGuest
    readonly property string playerCountryCode: playerEntry.countryCode || ""

    // Wide screen → actions as icons inline to the right of the name, no collapse.
    // Portrait → the existing expand/collapse with stacked buttons.
    readonly property bool wideLayout: Config.Responsive.landscape

    // ── Filter (Spielersuche) ────────────────────────────────────────
    readonly property bool matchesFilter: {
        var f = searchFilter.toLowerCase()
        return f.length === 0 || displayName.toLowerCase().includes(f)
    }

    width: listView.width
    visible: matchesFilter
    height: matchesFilter
            ? ((showInGameLine || showActionColumn) ? expandedHeight : rowHeight)
            : 0

    // The default padding of the ItemDelegate (basic style: 12px) pushes the content
    // down at a fixed row height of 30 → centre the name vertically.
    topPadding: 0
    bottomPadding: 0

    property bool expanded: false
    readonly property bool playerIgnored: {
        var _rev = Lobby ? Lobby.playerIgnoreListRevision : 0
        return Lobby ? Lobby.isPlayerIgnored(targetPlayerId) : false
    }
    readonly property int rowHeight: 30
    readonly property int actionButtonHeight: 24
    readonly property int actionSpacing: 3
    readonly property int actionCount: (canSendPm ? 1 : 0)
                                     + (canInvite ? 1 : 0)
                                     + (canIgnore ? 1 : 0)
                                     + (canUnignore ? 1 : 0)
                                     + (canShowPlayerStats ? 1 : 0)
                                     + (canAdminModerate ? 1 : 0)
    // Two lines of 13 px for the "currently playing in ..." info above the buttons.
    readonly property int inGameLineHeight: 26
    readonly property int actionsBlockHeight: (actionCount * actionButtonHeight)
                                            + (Math.max(0, actionCount - 1) * actionSpacing)
    readonly property int expandedHeight: rowHeight
                                        + (showInGameLine ? 5 + inGameLineHeight : 0)
                                        + (showActionColumn ? actionSpacing + actionsBlockHeight : 0)

    // ── What does the expanded area show? ────────────────────────────────────
    // Portrait: the stacked action buttons (the icon row is missing there).
    // Wide layout: the actions already sit as icons in the row – expanding
    // is only worthwhile on touch devices, which lack the hover tooltip with the
    // "currently playing in ..." info. Hence device detection (Qt.platform.os) instead of a
    // resolution heuristic: a tablet in landscape does have desktop geometry,
    // but just as little a mouse cursor as a phone.
    readonly property bool showActionColumn: !wideLayout && expanded && hasActions
    readonly property bool showInGameLine: expanded && (wideLayout ? Config.Responsive.isMobile
                                                                  : hasActions)
    readonly property bool expandable: wideLayout ? Config.Responsive.isMobile : hasActions

    readonly property bool isSelf: Lobby && targetPlayerId === Lobby.myPlayerId
    // gameListRevision as a reactive dependency: forces a re-evaluation
    // when players join or leave a game.
    readonly property bool canInvite: Lobby && Lobby.canInviteFromCurrentGame && !isSelf && !guestPlayer
        && (Lobby.gameListRevision >= 0 && !Lobby.isPlayerInAnyGame(targetPlayerId))
    readonly property bool canAdminModerate: Lobby && Lobby.isCurrentPlayerAdmin && !isSelf
    // Private message: guests are not allowed to chat at all on the server side – neither
    // as a sender nor as a RECIPIENT –, and the server discards PMs to players
    // sitting at a RUNNING table. Hide all three here, otherwise
    // only a "chat rejected" would come back instead of the message.
    // gameListRevision keeps the check reactive.
    readonly property bool canSendPm: Lobby && !isSelf && !Lobby.isMyPlayerGuest && !guestPlayer
        && (Lobby.gameListRevision >= 0 && !Lobby.isPlayerInRunningGame(targetPlayerId))
    readonly property bool canShowPlayerStats: !guestPlayer
    // "Currently playing in ..." info. Only query it if somebody sees it:
    // on the desktop when hovering over the name, on touch in the expanded area
    // (there is no hover there). gameListRevision as a reactive dependency,
    // in case the player joins or leaves a game meanwhile.
    readonly property string inGameName: {
        var _rev = Lobby ? Lobby.gameListRevision : 0
        return (Lobby && (nameHover.hovered || expanded))
            ? Lobby.playerInGameName(targetPlayerId) : ""
    }
    readonly property bool canIgnore: !isSelf && !guestPlayer && !playerIgnored
    readonly property bool canUnignore: !isSelf && !guestPlayer && playerIgnored
    readonly property bool hasActions: canSendPm || canInvite || canAdminModerate || canIgnore || canUnignore || canShowPlayerStats

    // ── Fixed icon columns (wide layout) ─────────────────────────────────────
    // So that every icon sits at the same x position in ALL rows, an icon that is
    // not available for this row keeps its place (PlayerActionIcon.active)
    // instead of falling out of the row. A column is only reserved at all
    // if the action comes into question for the LIST – the kickban
    // column for instance only for server admins, otherwise it would stay empty for everyone.
    // Ignore/unignore are mutually exclusive and share one column.
    readonly property bool slotPm: Lobby && !Lobby.isMyPlayerGuest
    readonly property bool slotInvite: Lobby && Lobby.canInviteFromCurrentGame
    readonly property bool slotIgnore: true
    readonly property bool slotStats: true
    readonly property bool slotAdmin: Lobby && Lobby.isCurrentPlayerAdmin

    readonly property color pmColor: Config.StaticData.chartColor(3, true)
    readonly property color inviteColor: Config.StaticData.chartColor(0, true)
    readonly property color ignoreColor: Config.StaticData.chartColor(8, true)
    readonly property color statsColor: Config.StaticData.chartColor(9, true)
    readonly property color banColor: Config.StaticData.chartColor(5, true)

    // The input popup deliberately lives in the page and not in the delegate:
    // a list update while typing would discard the delegate (and with it the
    // text).
    signal privateMessageRequested(int playerId, string playerName)

    // Confirmation before inviting a player into your own game.
    function confirmInvite() {
        invitePopup.openWith(
            qsTr("Invite to Game"),
            qsTr("Are you sure you want to invite \"%1\" to your game?").arg(displayName),
            qsTr("Invite"))
    }

    // Confirmation before ignoring a player (an accidental click).
    function confirmIgnore() {
        ignorePopup.openWith(
            qsTr("Ignore player"),
            qsTr("Are you sure you want to ignore \"%1\"?").arg(displayName),
            qsTr("Ignore player"))
    }

    // Confirmation before unignoring a player.
    function confirmUnignore() {
        unignorePopup.openWith(
            qsTr("Unignore player"),
            qsTr("Are you sure you want to unignore \"%1\"?").arg(displayName),
            qsTr("Unignore player"))
    }

    // Confirmation before the final kickban (admin) of a player.
    function confirmBan() {
        banPopup.openWith(
            qsTr("Total kickban"),
            qsTr("Are you sure you want to totally kickban \"%1\"?").arg(displayName),
            qsTr("Total kickban"))
    }

    onCollapseResetCounterChanged: {
        expanded = false
        listView.expandedPlayerIndex = -1
    }

    Connections {
        target: listView
        function onExpandedPlayerIndexChanged() {
            if (listView.expandedPlayerIndex !== playerItem.index)
                playerItem.expanded = false
        }
    }

    // When the layout switches to wide screen, close a possibly open expander
    // – otherwise the item container would stay unnecessarily tall on a resize.
    // On touch devices it stays open: there it still carries the
    // "currently playing in ..." line in the wide layout as well.
    onWideLayoutChanged: {
        if (wideLayout && !expandable) {
            expanded = false
            if (listView.expandedPlayerIndex === playerItem.index)
                listView.expandedPlayerIndex = -1
        }
    }

    Behavior on height {
        NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
    }
    
    contentItem: ColumnLayout {
        spacing: 0
        
        // Header row: flag + name + right-aligned expander
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.topMargin: 0
            Layout.bottomMargin: 0
            spacing: 5

            // Flag
            Image {
                visible: playerCountryCode !== ""
                source: playerCountryCode !== ""
                    ? "qrc:/resources/cflags/" + playerCountryCode.toLowerCase() + ".svg"
                        : ""
                Layout.preferredWidth: 18
                Layout.preferredHeight: 14
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            
            // Player name
            AppText {
                text: displayName
                font.pixelSize: listView.height > 100 ? 12 : 11
                color: Config.StaticData.palette.secondary.col200
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                elide: Text.ElideRight

                // Desktop hover tooltip: "plays in ..." / "does not play currently"
                // (the widget client shows the same info in the nick list context menu,
                // touch devices in the expanded area – see below).
                HoverHandler { id: nameHover }

                ToolTip.text: playerItem.inGameName !== ""
                              ? qsTr("%1 is playing in \"%2\".").arg(displayName).arg(playerItem.inGameName)
                              : qsTr("%1 is not playing at the moment.").arg(displayName)
                ToolTip.visible: nameHover.hovered && !Config.Responsive.isMobile
                                 && Config.Parameters.showTooltips
                ToolTip.delay: 400
            }

            // Wide screen: action icons inline, right-aligned – in fixed columns
            // (see the slot* properties above). The row stays even when
            // no action is available for this row, so that the columns
            // keep the same width across all rows.
            Row {
                id: wideActionsRow
                visible: playerItem.wideLayout
                spacing: 2
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                PlayerActionIcon {
                    visible: playerItem.slotPm
                    active: playerItem.canSendPm
                    source: "qrc:/resources/mail.svg"
                    baseColor: playerItem.pmColor
                    tooltipText: qsTr("Send private message")
                    onTriggered: playerItem.privateMessageRequested(playerItem.targetPlayerId,
                                                                   playerItem.displayName)
                }
                PlayerActionIcon {
                    visible: playerItem.slotInvite
                    active: playerItem.canInvite
                    source: "qrc:/resources/personAdd.svg"
                    baseColor: playerItem.inviteColor
                    tooltipText: qsTr("Invite to Game")
                    onTriggered: playerItem.confirmInvite()
                }
                // One column for both states: ignored ⇄ not ignored.
                PlayerActionIcon {
                    visible: playerItem.slotIgnore
                    active: playerItem.canIgnore || playerItem.canUnignore
                    source: playerItem.playerIgnored ? "qrc:/resources/checkCircle.svg"
                                                     : "qrc:/resources/block.svg"
                    baseColor: playerItem.ignoreColor
                    tooltipText: playerItem.playerIgnored ? qsTr("Unignore player")
                                                          : qsTr("Ignore player")
                    onTriggered: {
                        if (playerItem.playerIgnored)
                            playerItem.confirmUnignore()
                        else
                            playerItem.confirmIgnore()
                    }
                }
                PlayerActionIcon {
                    visible: playerItem.slotStats
                    active: playerItem.canShowPlayerStats
                    source: "qrc:/resources/barChart.svg"
                    baseColor: playerItem.statsColor
                    tooltipText: qsTr("Show player stats")
                    onTriggered: { if (Lobby) Lobby.showPlayerStats(playerItem.targetPlayerId) }
                }
                PlayerActionIcon {
                    visible: playerItem.slotAdmin
                    active: playerItem.canAdminModerate
                    source: "qrc:/resources/gavel.svg"
                    baseColor: playerItem.banColor
                    tooltipText: qsTr("Total kickban")
                    onTriggered: playerItem.confirmBan()
                }
            }

            // Portrait: expander caret (wide screen hides it).
            SvgIcon {
                id: expanderCaret
                source: "qrc:/resources/caretLeft.svg"
                rotation: expanded ? -180 : -90
                Behavior on rotation { NumberAnimation { duration: 150 } }
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                visible: playerItem.expandable
                // Colourising via layer.effect instead of a MultiEffect child: VectorImage
                // (Qt >= 6.8) is not a texture provider and must not be referenced via
                // source (it would render black/torn).
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: Config.Theme.colorTextMuted
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (playerItem.expandable) {
                            const opening = !playerItem.expanded
                            playerItem.listView.expandedPlayerIndex = opening ? playerItem.index : -1
                            playerItem.expanded = opening
                        }
                    }
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }
        
        // Portrait: Action-Buttons (aufgeklappt)
        ColumnLayout {
            visible: playerItem.showInGameLine || playerItem.showActionColumn
            Layout.fillWidth: true
            Layout.topMargin: 5
            spacing: 3

            // The touch counterpart to the desktop hover tooltip above the name: on
            // the phone there is no hover, so the same info stands here –
            // same wording, same source (playerItem.inGameName).
            AppText {
                visible: playerItem.showInGameLine
                Layout.fillWidth: true
                Layout.preferredHeight: playerItem.inGameLineHeight
                text: playerItem.inGameName !== ""
                      ? qsTr("%1 is playing in \"%2\".").arg(playerItem.displayName)
                                                         .arg(playerItem.inGameName)
                      : qsTr("%1 is not playing at the moment.").arg(playerItem.displayName)
                font.pixelSize: 10
                color: playerItem.inGameName !== "" ? playerItem.inviteColor
                                                    : Config.Theme.colorTextMuted
                // A fixed height for two lines: the line height goes into
                // expandedHeight, a dynamic wrap would cut off the
                // expanded area at the bottom.
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            // Send private message
            Button {
                text: qsTr("Send private message")
                visible: canSendPm && playerItem.showActionColumn
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                    color: parent.pressed ? Qt.darker(playerItem.pmColor, 1.35)
                           : parent.hovered ? playerItem.pmColor
                           : Qt.darker(playerItem.pmColor, 1.18)
                    radius: 3
                    border.width: 1
                    border.color: Qt.darker(playerItem.pmColor, 1.55)
                }

                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    playerItem.privateMessageRequested(playerItem.targetPlayerId,
                                                       playerItem.displayName)
                    playerItem.expanded = false
                }
            }

            // Invite to game
            Button {
                text: qsTr("Invite to Game")
                visible: canInvite && playerItem.showActionColumn
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10
                
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
                
                background: Rectangle {
                    color: parent.pressed ? Qt.darker(playerItem.inviteColor, 1.35)
                           : parent.hovered ? playerItem.inviteColor
                           : Qt.darker(playerItem.inviteColor, 1.18)
                    radius: 3
                    border.width: 1
                    border.color: Qt.darker(playerItem.inviteColor, 1.55)
                }
                
                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    playerItem.confirmInvite()
                    playerItem.expanded = false
                }
            }

            // Ignore player
            Button {
                text: qsTr("Ignore player")
                visible: canIgnore && playerItem.showActionColumn
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                      color: parent.pressed ? Qt.darker(playerItem.ignoreColor, 1.35)
                          : parent.hovered ? playerItem.ignoreColor
                          : Qt.darker(playerItem.ignoreColor, 1.18)
                    radius: 3
                    border.width: 1
                      border.color: Qt.darker(playerItem.ignoreColor, 1.55)
                }

                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    playerItem.confirmIgnore()
                    playerItem.expanded = false
                }
            }

            // Unignore player
            Button {
                text: qsTr("Unignore player")
                visible: canUnignore && playerItem.showActionColumn
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                      color: parent.pressed ? Qt.darker(playerItem.ignoreColor, 1.35)
                          : parent.hovered ? playerItem.ignoreColor
                          : Qt.darker(playerItem.ignoreColor, 1.18)
                    radius: 3
                    border.width: 1
                      border.color: Qt.darker(playerItem.ignoreColor, 1.55)
                }

                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    playerItem.confirmUnignore()
                    playerItem.expanded = false
                }
            }

            // Show player stats (widget parity)
            Button {
                text: qsTr("Show player stats")
                visible: canShowPlayerStats && playerItem.showActionColumn
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }

                background: Rectangle {
                      color: parent.pressed ? Qt.darker(playerItem.statsColor, 1.35)
                          : parent.hovered ? playerItem.statsColor
                          : Qt.darker(playerItem.statsColor, 1.18)
                    radius: 3
                    border.width: 1
                      border.color: Qt.darker(playerItem.statsColor, 1.55)
                }

                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (Lobby) Lobby.showPlayerStats(targetPlayerId)
                    playerItem.expanded = false
                }
            }
            
            // Admin action (widget parity)
            Button {
                text: qsTr("Total kickban")
                visible: canAdminModerate && playerItem.showActionColumn
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                font.pixelSize: 10
                
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
                
                background: Rectangle {
                    color: parent.pressed ? Qt.darker(playerItem.banColor, 1.35)
                           : parent.hovered ? playerItem.banColor
                           : Qt.darker(playerItem.banColor, 1.18)
                    radius: 3
                    border.width: 1
                    border.color: Qt.darker(playerItem.banColor, 1.55)
                }
                
                contentItem: AppText {
                    text: parent.text
                    color: "white"
                    font.pixelSize: parent.font.pixelSize
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    playerItem.confirmBan()
                    playerItem.expanded = false
                }
            }
        }
    }
    
    background: Rectangle {
        color: playerItem.hovered
               ? Config.Theme.colorHover
               : "transparent"
        radius: 3
        Behavior on color { ColorAnimation { duration: 130 } }
    }

    ConfirmPopup {
        id: invitePopup
        onConfirmed: { if (Lobby) Lobby.invitePlayer(playerItem.targetPlayerId) }
    }

    ConfirmPopup {
        id: ignorePopup
        onConfirmed: { if (Lobby) Lobby.ignorePlayer(playerItem.targetPlayerId) }
    }

    ConfirmPopup {
        id: unignorePopup
        onConfirmed: { if (Lobby) Lobby.unignorePlayer(playerItem.targetPlayerId) }
    }

    ConfirmPopup {
        id: banPopup
        onConfirmed: { if (Lobby) Lobby.adminBanPlayer(playerItem.targetPlayerId) }
    }
}
