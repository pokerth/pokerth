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
    // Showdown-Spotlight: einzelne Hole-Card abblenden, wenn sie nicht zum
    // Siegerblatt zählt (Daten aus GameHandler über seatData.fade0/fade1).
    property bool fade0: false
    property bool fade1: false
    property string avatarSource: ""
    property bool folded: false
    property bool playerActive: true

    // Anti-Peek: eigene Hole-Cards verdeckt halten (nur Self-Box, gesteuert über
    // die Einstellung AntiPeekMode). Aufgedeckt wird nur, solange der Spieler die
    // Karten „lüftet": Hover (Desktop) bzw. Drücken-und-Halten (Touch) – wie der
    // Qt-Widgets-Client (gameTableImpl::mouseOverFlipCards), momentan statt fest.
    property bool antiPeek: false
    readonly property bool _peeking: peekArea.enabled
                                     && (peekArea.containsMouse || peekArea.pressed)

    // Netzwerkstatus-Ampel in der Avatar-Ecke (nur Self-Box, Einstellung
    // ShowPingStateInAvatar). Leer/transparent → kein Punkt.
    property bool showNetworkStatus: false
    property color networkStatusColor: "transparent"

    // Einstellung „Ausblend-Animation für Verliererkarten" (Config-Key
    // ShowFadeOutCardsAnimation). Ist sie aus, bleiben alle Karten voll sichtbar.
    readonly property bool fadeLosingCards:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowFadeOutCardsAnimation") !== 0 : true
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

    // „Show"-Bestätigung: beide eigenen Hole-Cards umdrehen (Self-Box, beim Klick
    // auf „Karten zeigen"). Die zweite Karte erbt über flipDelay den Versatz.
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

        // Netzwerkstatus-Punkt (Ampel) unten rechts in der Avatar-Ecke.
        Rectangle {
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
            // Showdown: nicht zum Siegerblatt zählende Karte auf 25 % abblenden.
            opacity: (root.fade0 && root.fadeLosingCards) ? 0.25 : 1.0
            Behavior on opacity { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }
            CardImage { id: card0Img; anchors.fill: parent; cardIndex: root.card0 }
            // Anti-Peek-Abdeckung (Kartenrücken) über der echten Vorderseite.
            CardImage {
                anchors.fill: parent
                cardIndex: -1
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
            // flipDelay staffelt das Austeilen: zweite Karte dreht 80 ms später
            CardImage { id: card1Img; anchors.fill: parent; cardIndex: root.card1; flipDelay: 80 }
            CardImage {
                anchors.fill: parent
                cardIndex: -1
                readonly property bool covering: root.antiPeek && root.card1 >= 0 && !root._peeking
                visible: opacity > 0
                opacity: covering ? 1.0 : 0.0
                Behavior on opacity { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            }
        }

        // „Lüften"-Bereich über beiden Karten: deckt auf, solange gehovert/gedrückt.
        // Bei deaktiviertem Anti-Peek inert (enabled:false → Events fallen durch).
        MouseArea {
            id: peekArea
            anchors.fill: parent
            enabled: root.antiPeek && (root.card0 >= 0 || root.card1 >= 0)
            hoverEnabled: enabled
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }
}
