import QtQuick
import QtQuick.Effects

import "../config" as Config

// Avatar + two hole-cards in a fixed-height row.
// cardH == height exactly (no aspect-ratio rounding gap vs. avatar height).
// Set `height` from the parent; the component reports `implicitWidth`.
Item {
    id: root

    property int card0: -1
    property int card1: -1
    property string avatarSource: ""
    property bool folded: false
    property bool playerActive: true
    // Avatar height cap (px). Default: uncapped (opponent boxes).
    // Self-box passes 60 to stay within the cardsArea.
    property int maxAvatarSize: 9999

    // Avatar and card dimensions — all derived from the item height.
    readonly property int cardH: height
    readonly property int cardW: height > 0 ? Math.round(height * 120 / 168) : 0
    readonly property int avatarSize: Math.min(height, maxAvatarSize)

    // X-coordinate of the cards group centre, relative to this item's origin.
    // Used by parent components for badge / timeout-bar positioning.
    readonly property real cardsCenterX: avatarSize + 4 + (cardW * 2 + 4) / 2

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
            color: Config.StaticData.palette.secondary.col700
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
            // Raus aus dem Spiel → Avatar entsättigen.
            layer.enabled: !root.playerActive
            layer.effect: MultiEffect { saturation: -1.0 }
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
            CardImage { anchors.fill: parent; cardIndex: root.card0 }
        }

        Rectangle {
            x: root.cardW + 4; y: 0
            width: root.cardW
            height: root.cardH
            color: "transparent"
            // flipDelay staffelt das Austeilen: zweite Karte dreht 80 ms später
            CardImage { anchors.fill: parent; cardIndex: root.card1; flipDelay: 80 }
        }
    }
}
