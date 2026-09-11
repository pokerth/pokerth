import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import "../config" as Config

// Avatar + two hole-cards in a fixed-height row.
// cardH == height exactly (no aspect-ratio rounding gap vs. avatar height).
// Set `height` from the parent; the component reports `implicitWidth`.
Item {
    id: root

    property int card0: -1
    property int card1: -1
    // Showdown spotlight: dim an individual hole card if it does not count
    // towards the winning hand (data from the GameHandler via seatData.fade0/fade1).
    property bool fade0: false
    property bool fade1: false
    property string avatarSource: ""
    property bool folded: false
    property bool playerActive: true

    // Anti-peek: keep your own hole cards covered (self box only, controlled via
    // the AntiPeekMode setting). They are only uncovered while the player "lifts"
    // the cards: hover (desktop) or press-and-hold (touch) – like the
    // Qt widgets client (gameTableImpl::mouseOverFlipCards), momentary instead of fixed.
    property bool antiPeek: false
    readonly property bool _peeking: peekArea.enabled
                                     && (peekArea.containsMouse || peekArea.pressed)

    // Network status light in the avatar corner (self box only, setting
    // ShowPingStateInAvatar). Empty/transparent → no dot.
    property bool showNetworkStatus: false
    property color networkStatusColor: "transparent"
    // Raw values of the last server response times (ms) for the mouseover overlay
    // at the network status dot (−1 = no data). Desktop only.
    property int networkPingAvg: -1
    property int networkPingMin: -1
    property int networkPingMax: -1

    // Setting "fade out animation for loser cards" (config key
    // ShowFadeOutCardsAnimation). If it is off, all cards stay fully visible.
    readonly property bool fadeLosingCards:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowFadeOutCardsAnimation") !== 0 : true
    // Avatar height cap (px). Default: uncapped (opponent boxes).
    // Self-box passes 60 to stay within the cardsArea.
    property int maxAvatarSize: 9999

    // Effective table scaling of the surrounding box (boxScale/oppScale × zoom),
    // passed through to the cards so that their SVG raster hits the real
    // screen pixel size (see CardImage.renderScale).
    property real cardRenderScale: 1.0

    // Avatar and card dimensions — all derived from the item height.
    readonly property int cardH: height
    readonly property int cardW: height > 0 ? Math.round(height * 120 / 168) : 0
    readonly property int avatarSize: Math.min(height, maxAvatarSize)

    // X-coordinate of the cards group centre, relative to this item's origin.
    // Used by parent components for badge / timeout-bar positioning.
    readonly property real cardsCenterX: avatarSize + 4 + (cardW * 2 + 4) / 2

    // "Show" confirmation: turn over both of your own hole cards (self box, on a
    // click on "show cards"). The second card inherits the offset via flipDelay.
    function playShowFlip() {
        card0Img.playShowFlip()
        card1Img.playShowFlip()
    }

    implicitWidth: avatarSize + 4 + cardW * 2 + 4

    // ── Avatar ──────────────────────────────────────────────────────────────────
    Rectangle {
        id: avatarBox
        x: 0
        anchors.verticalCenter: parent.verticalCenter
        width: root.avatarSize
        height: root.avatarSize

        Rectangle {
            anchors.fill: parent
            border.width: 1
            border.color: Config.StaticData.palette.secondary.col200
            color: Config.Theme.colorBox
            opacity: 0.9
            radius: 2
        }

        Image {
            anchors.fill: parent
            anchors.margins: 1
            fillMode: root.avatarSource !== "" ? Image.PreserveAspectCrop : Image.PreserveAspectFit
            source: root.avatarSource !== "" ? root.avatarSource : "qrc:resources/pokerth.svg"
            asynchronous: true
            cache: true
            // Out of the game → desaturate the avatar.
            layer.enabled: !root.playerActive
            layer.effect: MultiEffect { saturation: -1.0 }
        }

        // Network status dot (traffic light) at the bottom right of the avatar corner. On
        // the desktop a mouseover shows an overlay with the server response times
        // (avg/min/max ms) – analogous to the tooltip of the Qt widgets client.
        Rectangle {
            id: netDot
            visible: root.showNetworkStatus
            width: Math.max(6, Math.round(root.avatarSize * 0.22))
            height: width
            radius: width / 2
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 1
            color: root.networkStatusColor
            border.width: 1
            border.color: Qt.darker(root.networkStatusColor, 2.0)

            // More generous hover area than the small dot itself.
            MouseArea {
                id: netHover
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: !Config.Responsive.isMobile
                acceptedButtons: Qt.NoButton
            }

            ToolTip.visible: netHover.containsMouse
                             && !Config.Responsive.isMobile
                             && Config.Parameters.showTooltips
                             && root.networkPingAvg >= 0
            ToolTip.delay: 300
            ToolTip.text: qsTr("Server response times")
                          + "\n" + qsTr("Average: %1 ms").arg(root.networkPingAvg)
                          + "\n" + qsTr("Minimum: %1 ms").arg(root.networkPingMin)
                          + "\n" + qsTr("Maximum: %1 ms").arg(root.networkPingMax)
        }
    }

    // ── Hole-cards ───────────────────────────────────────────────────────────────
    // Hidden when player is eliminated; dimmed when folded.
    Item {
        id: cardsItem
        x: root.avatarSize + 4
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: root.cardW * 2 + 4

        visible: root.playerActive
        opacity: root.folded ? 0.3 : 1.0
        Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }

        Rectangle {
            x: 0; y: 0
            width: root.cardW
            height: root.cardH
            color: "transparent"
            // Showdown: dim a card not counting towards the winning hand to 25 %.
            opacity: (root.fade0 && root.fadeLosingCards) ? 0.25 : 1.0
            Behavior on opacity { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }
            CardImage { id: card0Img; anchors.fill: parent; cardIndex: root.card0; renderScale: root.cardRenderScale }
            // Anti-peek cover (card back) above the real front side.
            CardImage {
                anchors.fill: parent
                cardIndex: -1
                renderScale: root.cardRenderScale
                readonly property bool covering: root.antiPeek && root.card0 >= 0 && !root._peeking
                visible: opacity > 0
                opacity: covering ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            }
        }

        Rectangle {
            x: root.cardW + 4; y: 0
            width: root.cardW
            height: root.cardH
            color: "transparent"
            opacity: (root.fade1 && root.fadeLosingCards) ? 0.25 : 1.0
            Behavior on opacity { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }
            // flipDelay staggers the dealing: the second card turns 80 ms later
            CardImage { id: card1Img; anchors.fill: parent; cardIndex: root.card1; flipDelay: 80; renderScale: root.cardRenderScale }
            CardImage {
                anchors.fill: parent
                cardIndex: -1
                renderScale: root.cardRenderScale
                readonly property bool covering: root.antiPeek && root.card1 >= 0 && !root._peeking
                visible: opacity > 0
                opacity: covering ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            }
        }

        // "Lift" area above both cards: uncovers them while hovered/pressed.
        // With anti-peek disabled it is inert (enabled:false → events fall through).
        MouseArea {
            id: peekArea
            anchors.fill: parent
            enabled: root.antiPeek && (root.card0 >= 0 || root.card1 >= 0)
            hoverEnabled: enabled
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }
}
