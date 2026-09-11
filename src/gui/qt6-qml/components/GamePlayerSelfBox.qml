import QtQuick

import "../config" as Config

SeatBox {
    id: root

    property int maxAvatarSize: 60

    // Your own seat is seat 0 - all derivations from it are delivered by SeatBox.
    seatIndex: 0

    // Anti-peek (config key AntiPeekMode): keep your own hole cards covered,
    // uncover them briefly only by hovering/pressing. readConfigInt is not reactive –
    // it takes effect from the next instantiation of the self box (game start).
    readonly property bool antiPeek:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("AntiPeekMode") !== 0 : false

    // Network status light (config key ShowPingStateInAvatar): only at your own
    // avatar and only once real ping data is available (pingState > 0).
    readonly property bool showPingState:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowPingStateInAvatar") !== 0 : false
    readonly property int pingState:
        (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingState : 0
    readonly property color pingColor: pingState === 1 ? "#43a047"   // green
                                     : pingState === 2 ? "#fbc02d"   // yellow
                                     : pingState === 3 ? "#e53935"   // red
                                     : "transparent"

    // Player in the game? Whoever has no money left for the next hand is inactive.
    // A default of its own (true) as opposed to the opponent box.
    readonly property bool playerActive: seatData && seatData.active !== undefined ? seatData.active : true

    // In landscape mode: a 2 line info area as with the opponent boxes
    // (name at the top / stack at the bottom right). In portrait it stays 1 line.
    readonly property bool twoLineInfo: Config.Responsive.landscape

    // Bet base (betInset/betStripH) and unfolding the base: see SeatBox.
    // Horizontal spacings uniform: left outer margin = distance avatar↔cards
    // = right outer margin = hMargin. The same measure (4) as with the opponent boxes
    // (GamePlayerBox.hMargin), so that the outer margins are visually consistent.
    readonly property int hMargin: 4
    // Vertical spacings uniform: upper outer margin = distance cards↔text
    // = lower outer margin = vMargin.
    readonly property int vMargin: 4

    // Information density: folded → pull back subtly, out of the game →
    // darken clearly (analogous to the opponent boxes).
    opacity: !root.playerActive ? 0.4 : (root.folded ? 0.78 : 1.0)
    Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }

    // Slightly "raised" when to act (depth/focus, a gentle transition).
    scale: root.isAtTurn ? 1.03 : 1.0
    transformOrigin: Item.Center
    Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

    // ── Box body (without the bet base) ──────────────────────────────────────
    // `root.height` is the height permanently RESERVED at the table including
    // the base. The body sits at the top of it, the base unfolds downwards into the
    // rest that is kept free. That way the box only grows with an actual bet,
    // without the table scaling or the neighbouring boxes moving.
    Item {
        id: bodyBox
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.bodyH + betStrip.height

        // Background with a subtle gradient + a soft drop shadow → a raised card.
        PlayerBoxBackground {}

        // Highlight: a gold frame + a soft glow when I am to act (the frame a bit
        // stronger than with the opponent boxes).
        PlayerTurnGlow {
            active: root.isAtTurn
            borderWidth: 2
        }

        // ── Cards – centred above the info line ──────────────────────────────────
        Item {
            id: cardsArea
            anchors.top: parent.top
            anchors.topMargin: root.vMargin
            anchors.bottom: bottomBar.top
            anchors.bottomMargin: root.vMargin
            anchors.left: parent.left
            anchors.leftMargin: root.hMargin
            anchors.right: parent.right
            anchors.rightMargin: root.hMargin

            AvatarCardRow {
                id: cardRow
                anchors.centerIn: parent
                height: parent.height
                maxAvatarSize: root.maxAvatarSize
                cardRenderScale: root.cardRenderScale
                card0: root.card0
                card1: root.card1
                fade0: root.fade0
                fade1: root.fade1
                antiPeek: root.antiPeek
                showNetworkStatus: root.showPingState && root.pingState > 0
                networkStatusColor: root.pingColor
                networkPingAvg: (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingAvg : -1
                networkPingMin: (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingMin : -1
                networkPingMax: (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingMax : -1
                avatarSource: root.avatarSource
                folded: root.folded
                playerActive: root.playerActive
            }
        }

        // Click on "show cards" → turn over your own hole cards as a confirmation
        // that you really do show them (analogous to the widget client: showHoleCards → flip).
        Connections {
            target: (typeof GameTable !== "undefined") ? GameTable : null
            function onMyCardsShown() { cardRow.playShowFlip() }
        }

        // ── Name + stack – the lower info area ──────────────────────────────────────
        // Portrait: 1 line (name on the left, stack on the right), landscape: 2 lines as in the
        // opponent box (name at the top, stack at the bottom right). Height 18 → 32 in landscape.
        Item {
            id: bottomBar
            anchors.bottom: betStrip.top
            anchors.bottomMargin: root.vMargin
            anchors.left: parent.left
            anchors.leftMargin: root.hMargin
            anchors.right: parent.right
            anchors.rightMargin: root.hMargin
            height: root.twoLineInfo ? 32 : 18

            // Portrait: 1-zeilig
            Row {
                visible: !root.twoLineInfo
                width: parent.width
                height: parent.height
                spacing: 5

                AppText {
                    width: (parent.width - parent.spacing) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignLeft
                    color: "#eff1f5"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.3
                    elide: Text.ElideRight
                    text: root.seatData && root.seatData.name !== "" ? root.seatData.name : qsTr("Du")
                }

                AppText {
                    width: (parent.width - parent.spacing) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    color: Config.Theme.colorAccent
                    font.pixelSize: 15
                    font.bold: true
                    text: root.seatData ? "$" + root.seatData.stack : "$0"
                }
            }

            // Landscape: 2 lines (identical to the opponent box in wideLayout)
            Item {
                visible: root.twoLineInfo
                width: parent.width
                height: parent.height

                AppText {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    height: 16
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    color: "#eff1f5"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.3
                    elide: Text.ElideRight
                    text: root.seatData && root.seatData.name !== "" ? root.seatData.name : qsTr("Du")
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
                    text: root.seatData ? "$" + root.seatData.stack : "$0"
                }
            }
        }

        // Bet base at the lower box edge (seat style "inset"). It only unfolds
        // while there really is a bet; in the style "classic" it stays permanently
        // 0 px high – then its upper edge lies on bodyBox.bottom and the
        // info area above it sits exactly as before.
        PlayerBetStrip {
            id: betStrip
            open: root.stripOpen
            amount: root.bet
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        // Winner highlight: a golden frame + a "WINNER" badge above the box.
        // A child of bodyBox: the frame should enclose the BODY, not the
        // empty base height reserved below it.
        PlayerWinnerOverlay {
            active: root.isWinner
            gap: 3
            badgeHeight: 18
            badgeFontSize: 10
            hPadding: 14
        }
    }

    // ── Strip ABOVE the box: action indicator (badge or timeout bar)
    //    right-aligned, the bet to the left of it. That way your own hole cards
    //    are no longer covered (previously it was placed centred above the cards).
    Item {
        id: topStrip
        z: 26
        width: root.width
        height: 18
        x: 0
        y: -height - 6

        // Right distance of the bet to the box edge, so that it sits to the left of the
        // visible indicator: at the right-aligned action badge or – for the
        // BB/SB (the timeout is running) – to the left of the horizontally centred bar.
        readonly property real betRightMargin:
              actionBadge.visible ? actionBadge.width + 8
            : timeoutBar.visible  ? (width / 2 + timeoutBar.width / 2 + 8)
            : 0

        // Action badge: right-aligned above the box.
        PlayerActionBadge {
            id: actionBadge
            visible: root.actionText !== "" && !root.isWinner
            action: root.action
            label: root.actionText
            hPadding: 16
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            z: 2
        }

        // Action timeout: a progress bar horizontally centred above the box,
        // exclusive with the badge. The same blue as with the opponents, brighter.
        PlayerTimeoutBar {
            id: timeoutBar
            readonly property bool atTurn: (typeof GameTable !== "undefined" && GameTable)
                                           && GameTable.timeoutSeatId === root.seatIndex
            active: atTurn
            visible: atTurn && !root.isWinner && root.actionText === ""
            fillColor: Config.Theme.colorTimeoutSelf
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: 56
            height: 7
            z: 2
        }

        // Bet (chip + amount): to the left of the action indicator. If none is
        // visible, the bet moves right-aligned to the box edge.
        BetChip {
            id: betRow
            visible: root.bet > 0 && !root.betInset
            amount: root.bet
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: topStrip.betRightMargin
        }
    }

    // Dealer/small/big blind button: at the top right NEXT TO the box (outside).
    BlindButtonImage {
        id: buttonImg
        visible: root.buttonVisible
        button: root.button
        anchors.left: parent.right
        anchors.leftMargin: 6
        anchors.top: parent.top
        z: 25
    }
}
