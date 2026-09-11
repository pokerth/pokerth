import QtQuick
import QtQuick.Controls

import "../config" as Config

SeatBox {
    id: root

    // Show the winner badge below instead of above the box – only sensible for the
    // topmost box (player 5) in portrait, otherwise it would bump into the top.
    property bool winnerBelow: false
    // The side on which the bet chip + the dealer/blind button are shown:
    // "top" | "bottom" | "left" | "right". The default is derived from 'up'.
    property string betSide: up ? "bottom" : "top"
    // A split display: the dealer/blind button LEFT of the box, the bet
    // RIGHT of the box – both vertically centred. For the topmost box in
    // landscapeCompact, whose badge would otherwise collide below with the pot badge.
    // It overrides betSide.
    property bool betSplit: false

    // Effective table scaling, seat data and base dimensions: see SeatBox.
    // Dynamic width: 2×hMargin(4) + AvatarCardRow.implicitWidth(avatarH+4+2·cardW+4)
    readonly property int _topRowH: bodyH - (wideLayout ? 44 : 28)
    readonly property int _cardW:   Math.round(_topRowH * 120 / 168)
    implicitWidth: 2 * 4 + _topRowH + 4 + 2 * _cardW + 4
    implicitHeight: 84 + betStripH

    // Knocked out (no money left) - a default of its own as opposed to the self box.
    readonly property bool isActive: seatData ? seatData.active : false
    // Hide the avatars of ignored players (bind against the base property),
    // unless DontHideAvatarsOfIgnored switches that off.
    hideIgnoredAvatar:
        playerIgnored
        && ((typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
                ? SettingsManager.readConfigInt("DontHideAvatarsOfIgnored") === 0 : true)

    // ── Context actions ──────────────────────────────────────────────────────
    // A right click (desktop) or a long press (touch) on an opponent box opens
    // a context menu with "ignore player", "unignore player", "show player
    // stats" and the player note – as in the Qt widgets client (MyAvatarLabel)
    // or the lobby player list (PlayerListItem). The actions only apply in a
    // network game: only there does seatData carry a playerId (for local
    // games/CPU opponents 0 → no menu).
    readonly property bool targetIsComputer:
        seatData && seatData.isComputer !== undefined ? seatData.isComputer : false
    readonly property int targetPlayerId:
        seatData && seatData.playerId !== undefined ? seatData.playerId : 0
    readonly property bool targetIsGuest:
        seatData && seatData.isGuest !== undefined ? seatData.isGuest : false
    readonly property bool targetIsSelf:
        targetPlayerId !== 0 && typeof Lobby !== "undefined" && Lobby && targetPlayerId === Lobby.myPlayerId
    readonly property bool playerIgnored: {
        var _rev = (typeof Lobby !== "undefined" && Lobby) ? Lobby.playerIgnoreListRevision : 0
        return (typeof Lobby !== "undefined" && Lobby && targetPlayerId !== 0)
            ? Lobby.isPlayerIgnored(targetPlayerId) : false
    }
    readonly property bool canIgnore: !targetIsGuest && !targetIsSelf && !playerIgnored
    readonly property bool canUnignore: !targetIsGuest && !targetIsSelf && playerIgnored
    readonly property bool canShowStats: !targetIsGuest
    // Report an avatar: only in an internet game and only if the player has set an
    // (existing) avatar – 1:1 as in the Qt widgets client
    // (MyAvatarLabel). seatData.avatar is only set when a file is present.
    readonly property bool canReportAvatar:
        !targetIsSelf
        && (typeof GameTable !== "undefined" && GameTable && GameTable.isInternetGameRunning())
        && !!(seatData && seatData.avatar && seatData.avatar !== "")
    readonly property bool hasContextActions:
        targetPlayerId !== 0 && !targetIsComputer
        && (canIgnore || canUnignore || canShowStats || canReportAvatar || canEditNote)

    // ── Player note and rating ───────────────────────────────────────────────
    // Your own, purely local note (stars + text) about a fellow player, stored
    // in the same config entry as in the Qt widgets client (see
    // SettingsManager::setPlayerNote). As there, only in an internet game: only there
    // is there a permanently registered account behind the name that a
    // name based note can stick to at all.
    readonly property bool canEditNote:
        !targetIsGuest && !targetIsSelf && !targetIsComputer
        && (typeof GameTable !== "undefined" && GameTable && GameTable.isInternetGameRunning())
    readonly property int playerRating: {
        var _rev = (typeof SettingsManager !== "undefined" && SettingsManager)
                   ? SettingsManager.playerNotesRevision : 0
        return (canEditNote && typeof SettingsManager !== "undefined" && SettingsManager)
            ? SettingsManager.playerRating(root.targetPlayerName) : 0
    }
    readonly property string playerNote: {
        var _rev = (typeof SettingsManager !== "undefined" && SettingsManager)
                   ? SettingsManager.playerNotesRevision : 0
        return (canEditNote && typeof SettingsManager !== "undefined" && SettingsManager)
            ? SettingsManager.playerNote(root.targetPlayerName) : ""
    }

    // Wide screen layout: the box is large enough for 2 line info (name + flag/cash).
    // It uses height >= 76 as a proxy for tableZone.wide (oppBaseHeight = wide ? 84 : 71).
    // Deliberately NOT Config.Responsive.landscape – the table zone can be wider than
    // high even when the whole window (incl. the toolbar) is portrait-like.
    // Subtract the base height: otherwise a portrait box (71 + the base) would
    // wrongly slip above the threshold of 76 and would get the 2 line
    // landscape footer.
    readonly property bool wideLayout: bodyH >= 76

    // Only show it when the seat is occupied
    visible: root.seatData !== null && root.seatData.name !== ""

    // Information density: whoever is out (no money left → !isActive) is clearly
    // darkened, whoever has only folded is pulled back subtly. That way the
    // active player and the hand still running stand out more clearly.
    opacity: !root.isActive ? Config.Theme.dimmedOpacity
           : (root.folded ? 0.72 : 1.0)
    Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }

    // ── Hauptbox ────────────────────────────────────────────────────────────────
    Rectangle {
        id: playerBox
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        // The body + the (unfolded) base. The rest of the reserved height
        // stays empty while the player has not bet anything.
        height: root.bodyH + betStrip.height
        color: "transparent"
        property int hMargin: 4

        // The active player is slightly "raised" → more depth/focus (a gentle transition).
        scale: root.isAtTurn ? 1.04 : 1.0
        transformOrigin: Item.Center
        Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

        // A card background with a subtle gradient + a soft drop shadow → the
        // box looks like a raised card instead of a flat surface.
        PlayerBoxBackground {}

        // Highlight: the active player gets a gold frame + a soft glow.
        PlayerTurnGlow { active: root.isAtTurn }

        // Avatar + cards: AvatarCardRow guarantees cardH == topRowH (no
        // rounding difference). Spacings: 4 px on the left, 4 px avatar↔cards,
        // 4 px between the cards, 4 px on the right (= the implicitWidth formula above).
        AvatarCardRow {
            id: cardRow
            x: playerBox.hMargin
            y: 4
            height: root.wideLayout ? (root.bodyH - 44) : (root.bodyH - 28)

            cardRenderScale: root.cardRenderScale
            card0: root.card0
            card1: root.card1
            fade0: root.fade0
            fade1: root.fade1
            avatarSource: root.avatarSource
            folded: root.folded
            playerActive: root.isActive
        }

        // Portrait: name + (note badge) + stack on one line. Anchors instead of fixed
        // halves, so that the badge only takes the room it really needs
        // and the name elides exactly that much earlier.
        Item {
            visible: !root.wideLayout
            width: parent.width - 2 * playerBox.hMargin
            height: 15
            x: playerBox.hMargin
            y: root.bodyH - height - 4

            AppText {
                anchors.left: parent.left
                anchors.right: compactBadge.visible ? compactBadge.left : compactStack.left
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.seatData && root.seatData.name !== "" ? root.seatData.name : "---"
            }

            PlayerNoteBadge {
                id: compactBadge
                anchors.right: compactStack.left
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                rating: root.playerRating
                note: root.playerNote
                glyphSize: 10
            }

            AppText {
                id: compactStack
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.pixelSize: 12
                font.bold: true
                text: root.seatData && root.seatData.name !== "" ? "$" + root.seatData.stack : ""
            }
        }

        // Widescreen: Name + Flagge + Stack 2-zeilig
        Item {
            id: infoBar
            visible: root.wideLayout
            width: parent.width - 2 * playerBox.hMargin
            height: 36
            x: playerBox.hMargin
            y: root.bodyH - height - 4

            AppText {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.right: wideBadge.visible ? wideBadge.left : parent.right
                anchors.rightMargin: wideBadge.visible ? 4 : 2
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.pixelSize: 15
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.seatData && root.seatData.name !== "" ? root.seatData.name : "---"
            }

            // The note/rating goes into the NAME LINE, not into the lower line: there
            // the flag (22+6) and the stack (up to ~55 px with six
            // digits) already stand – together with the badge that would be more than the 106 px
            // inner width of the box (oppBaseWidth 114 − 2×hMargin). Up here
            // it only competes with the name, which elides anyway.
            PlayerNoteBadge {
                id: wideBadge
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 2
                rating: root.playerRating
                note: root.playerNote
                glyphSize: 13
            }

            Image {
                visible: root.countryCode !== ""
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                width: 22
                height: 15
                source: root.countryCode !== ""
                    ? "qrc:/resources/cflags/" + root.countryCode + ".svg" : ""
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            AppText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.pixelSize: 15
                font.bold: true
                text: root.seatData && root.seatData.name !== "" ? "$" + root.seatData.stack : ""
            }
        }

        // The bet base at the lower box edge (seat style "inset"). 1 px inside
        // the frame of PlayerBoxBackground, so that its border stays visible.
        PlayerBetStrip {
            id: betStrip
            open: root.stripOpen
            amount: root.bet
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        // Winner highlight: a golden frame (it does NOT cover the cards) +
        // a "WINNER" badge. The badge is above the box by default; only the topmost box
        // (winnerBelow) shows it below, otherwise it would bump into the screen edge.
        // A child of playerBox (not of root): the frame should enclose the BODY,
        // not the empty base height reserved below it.
        PlayerWinnerOverlay {
            active: root.isWinner
            below: root.winnerBelow
        }
    }

    // Action display (fold/check/call/bet/raise/all-in) – centred above the
    // hole cards in the normal player boxes.
    PlayerActionBadge {
        id: actionBadge
        visible: root.actionText !== "" && !root.isWinner
        action: root.action
        label: root.actionText
        z: 18

        readonly property real cardsCenterX: playerBox.hMargin + cardRow.cardsCenterX
        readonly property real cardsCenterY: cardRow.y + cardRow.height / 2
        x: cardsCenterX - width / 2
        y: cardsCenterY - height / 2
    }

    // Action timeout: a slim progress bar at the place of the action
    // badge while this seat is to act (it counts down over the timeout duration).
    PlayerTimeoutBar {
        id: timeoutBar
        readonly property bool atTurn: (typeof GameTable !== "undefined" && GameTable)
                                       && GameTable.timeoutSeatId === root.seatIndex
        active: atTurn
        visible: atTurn && !root.isWinner && root.actionText === ""
        width: 44
        height: 9
        z: 18
        x: actionBadge.cardsCenterX - width / 2
        y: actionBadge.cardsCenterY - height / 2
    }

    // The bet (chip + amount) + the dealer/small/big blind button – grouped.
    // Top/bottom centre (betSide top/bottom): the full box width; the bet centred,
    // the button right-aligned with a 6px outer margin – identical to the self box.
    // Sides (betSide left/right): the button below the bet, both vertically centred.
    Item {
        id: betGroup
        visible: (root.bet > 0 && !root.betInset) || root.buttonVisible
        z: 25

        readonly property bool split: root.betSplit
        readonly property bool horizontal: !split && (root.betSide === "bottom" || root.betSide === "top")
        // In the style "inset" the bet sits in the box base – the group then
        // only carries the dealer/blind puck.
        readonly property real betW: (root.bet > 0 && !root.betInset) ? betRow.width : 0
        readonly property real betH: (root.bet > 0 && !root.betInset) ? betRow.height : 0
        readonly property real btnW: root.buttonVisible ? buttonImg.width : 0
        readonly property real btnH: root.buttonVisible ? buttonImg.height : 0

        width: (horizontal || split) ? playerBox.width : Math.max(betW, btnW)
        height: horizontal ? Math.max(betH, btnH) : playerBox.height

        x: split ? 0
         : root.betSide === "right" ? playerBox.width + 8
         : root.betSide === "left"  ? -width - 8
         : 0
        y: split ? 0
         : root.betSide === "bottom" ? playerBox.height + 7
         : root.betSide === "top"    ? -height - 7
         : (playerBox.height - height) / 2

        BetChip {
            id: betRow
            visible: root.bet > 0 && !root.betInset
            amount: root.bet
            textColor: "#f0f0f0"
            // split: the bet to the right NEXT TO the box; otherwise centred inside.
            x: betGroup.split ? betGroup.width + 8 : (betGroup.width - width) / 2
            y: (betGroup.height - height) / 2
        }

        // Dealer/blind button – split: to the left NEXT TO the box; horizontal:
        // right-aligned 6px from the box edge; sides: the lower slot.
        BlindButtonImage {
            id: buttonImg
            visible: root.buttonVisible
            button: root.button
            x: betGroup.split
               ? -width - 8
               : betGroup.horizontal
               ? (betGroup.width - width - 6)
               : (root.betSide === "right" ? 0 : (betGroup.width - width))
            // At the sides (betSide left/right) the puck sat in the LOWER slot, because
            // the bet stood above it. If the bet sits in the base, the
            // slot is free → the puck goes vertically centred next to the box.
            y: (betGroup.horizontal || betGroup.split || root.betInset)
               ? (betGroup.height - height) / 2
               : (betGroup.height * 5 / 6 - height / 2)
        }
    }

    // ── Right click context menu (desktop only) ──────────────────────────────
    // It only intercepts the right mouse button; left clicks/hover fall through to the
    // elements below. It only appears when the seat carries a
    // real online fellow player (hasContextActions).
    MouseArea {
        // Only above the box body – the base height reserved below it is
        // empty table, no context menu may open there.
        id: contextArea
        anchors.fill: playerBox
        z: 30
        enabled: root.hasContextActions
        acceptedButtons: Qt.RightButton
        onClicked: (mouse) => contextMenu.popup(mouse.x, mouse.y)

        // Touch has no right mouse button: there a long press opens
        // the same menu. Deliberately limited to touch devices, so that a
        // held left click with the mouse still triggers nothing.
        TapHandler {
            acceptedDevices: PointerDevice.TouchScreen
            enabled: root.hasContextActions
            onLongPressed: contextMenu.popup(point.position.x, point.position.y)
        }
    }

    // A uniformly styled menu entry (a dark theme, it collapses when invisible).
    component CtxItem: MenuItem {
        height: visible ? implicitHeight : 0
        contentItem: AppText {
            text: parent.text
            color: parent.enabled
                   ? (parent.highlighted ? Config.Theme.colorAccent : Config.Theme.colorTextPrimary)
                   : Config.Theme.colorTextMuted
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }
        background: Rectangle {
            color: parent.highlighted ? Config.StaticData.palette.secondary.col600 : "transparent"
        }
    }

    Menu {
        id: contextMenu

        // Adjust the width to the widest visible entry (at least 180).
        // Necessary because the ListView contentItem of the menu reports no
        // implicitWidth – without this the width would be determined by the background alone and
        // longer (also translated) labels such as "Report inappropriate avatar"
        // would be cut off.
        implicitWidth: {
            var w = 180
            for (var i = 0; i < count; ++i) {
                var it = itemAt(i)
                if (it && it.visible)
                    w = Math.max(w, it.implicitWidth)
            }
            return w
        }

        // A dark theme matching the table surface.
        background: Rectangle {
            implicitWidth: 180
            color: Config.Theme.colorBox
            border.width: 1
            border.color: Config.StaticData.palette.secondary.col500
            radius: Config.Theme.radiusSmall
        }

        CtxItem {
            text: qsTr("Ignore player")
            visible: root.canIgnore
            onTriggered: root.confirmIgnore()
        }
        CtxItem {
            text: qsTr("Unignore player")
            visible: root.canUnignore
            onTriggered: root.confirmUnignore()
        }
        CtxItem {
            text: qsTr("Show player stats")
            visible: root.canShowStats
            onTriggered: { if (typeof Lobby !== "undefined" && Lobby) Lobby.showPlayerStats(root.targetPlayerId) }
        }
        CtxItem {
            text: qsTr("Report inappropriate avatar")
            visible: root.canReportAvatar
            onTriggered: root.confirmReportAvatar()
        }
        CtxItem {
            text: qsTr("Note about player ...")
            visible: root.canEditNote
            onTriggered: notePopup.openFor(root.targetPlayerName)
        }
    }

    PlayerNoteDialog { id: notePopup }

    readonly property string targetPlayerName: root.seatData ? (root.seatData.name || "") : ""

    // Confirmation before ignoring a player (an accidental click).
    function confirmIgnore() {
        ignorePopup.openWith(
            qsTr("Ignore player"),
            qsTr("Are you sure you want to ignore \"%1\"?").arg(root.targetPlayerName),
            qsTr("Ignore player"))
    }

    // Confirmation before unignoring a player.
    function confirmUnignore() {
        unignorePopup.openWith(
            qsTr("Unignore player"),
            qsTr("Are you sure you want to unignore \"%1\"?").arg(root.targetPlayerName),
            qsTr("Unignore player"))
    }

    // Confirmation before reporting an inappropriate avatar (a port of the
    // confirmation from MyAvatarLabel::reportBadAvatar).
    function confirmReportAvatar() {
        reportAvatarPopup.openWith(
            qsTr("Report inappropriate avatar"),
            qsTr("Are you sure you want to report the avatar of \"%1\" as inappropriate?").arg(root.targetPlayerName),
            qsTr("Report"))
    }

    ConfirmPopup {
        id: ignorePopup
        onConfirmed: { if (typeof Lobby !== "undefined" && Lobby) Lobby.ignorePlayer(root.targetPlayerId) }
    }

    ConfirmPopup {
        id: unignorePopup
        onConfirmed: { if (typeof Lobby !== "undefined" && Lobby) Lobby.unignorePlayer(root.targetPlayerId) }
    }

    ConfirmPopup {
        id: reportAvatarPopup
        onConfirmed: { if (typeof GameTable !== "undefined" && GameTable) GameTable.reportAvatar(root.seatIndex) }
    }
}
