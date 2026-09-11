import QtQuick
import QtQuick.Effects

import "../config" as Config

// Community cards (flop/turn/river) + pot badge in the centre of the table.
// Size = the card row; position/scaling/z is set by the caller (anchors +
// scale), so that the cards stay centred depending on the table layout.
Item {
    id: root

    // Landscape? Controls the distance of the pot badge to the card row.
    property bool wide: false

    // Effective table scaling of this card row (communityScale × zoom), passed on
    // to the cards so that their SVG raster hits the real screen size
    // (see CardImage.renderScale).
    property real cardRenderScale: 1.0

    // True while any board slot is currently revealing new cards
    // (stagger/backside/flip). The action bar locks the buttons on it –
    // during reveal animations no action is possible.
    readonly property bool dealing: slot0.revealing || slot1.revealing
                                    || slot2.revealing || slot3.revealing
                                    || slot4.revealing

    width: cardRow.width
    height: cardRow.height
    transformOrigin: Item.Center

    // Inline component for a single board card slot.
    // Card aspect ratio 120:168 (≈0.714).
    //
    // Deal sequence (as in the widget client):
    //   1. placeholder frame visible (initial state)
    //   2. isDealt → after the stagger pause: _displayIndex = -1  (the backside appears)
    //   3. short pause (like dealCardsSpeed in the widget client)
    //   4. _displayIndex = actualCardIndex  → CardImage.onIsBackChanged fires the flip
    component CommunitySlot: Item {
        id: slot
        property int boardIndex: 0
        width: 46; height: 64

        // Showdown spotlight: if this board card does not belong to the winning hand,
        // it is dimmed to 25 % opacity (setting "fade out animation
        // for loser cards", config key ShowFadeOutCardsAnimation).
        // Data: GameHandler.boardCardFade (5 bools per board card).
        readonly property bool fadeLosingCards:
            (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
                ? SettingsManager.readConfigInt("ShowFadeOutCardsAnimation") !== 0 : true
        readonly property bool faded: {
            if (!fadeLosingCards)
                return false
            var f = (typeof GameTable !== "undefined" && GameTable) ? GameTable.boardCardFade : null
            return (f && boardIndex < f.length) ? f[boardIndex] === true : false
        }
        opacity: faded ? 0.25 : 1.0
        Behavior on opacity { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }

        // Is this slot currently revealing? (stagger + backside + flip). While it is
        // true, the action bar locks the buttons. With its closing pause, dealAnim
        // also covers the ~470 ms CardImage flip animation.
        readonly property bool revealing: dealAnim.running

        readonly property bool isDealt: {
            var cnt = (typeof GameTable !== "undefined" && GameTable)
                      ? GameTable.boardCardCount : 0
            return boardIndex < cnt
        }

        readonly property int actualCardIndex: {
            var cards = (typeof GameTable !== "undefined" && GameTable)
                        ? GameTable.boardCards : null
            return (cards && boardIndex < cards.length) ? cards[boardIndex] : -1
        }

        // -2 = placeholder (nothing yet), -1 = backside, 0-51 = front side
        property int _displayIndex: -2

        // Placeholder frame: only visible while no card is shown yet
        Rectangle {
            anchors.fill: parent
            visible: slot._displayIndex === -2
            radius: 4
            color: Qt.rgba(0, 0, 0, 0.30)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.38)
        }

        // Card: backside (cardIndex -1) or front side (cardIndex 0-51)
        CardImage {
            anchors.fill: parent
            visible: slot._displayIndex !== -2
            cardIndex: slot._displayIndex >= 0 ? slot._displayIndex : -1
            renderScale: root.cardRenderScale
        }

        onIsDealtChanged: {
            if (isDealt) {
                dealAnim.restart()
            } else {
                dealAnim.stop()
                slot._displayIndex = -2
            }
        }

        // Stagger → show the backside → flip to the front side (a ScriptAction sets
        // _displayIndex, which triggers CardImage.onIsBackChanged and thereby the flip).
        SequentialAnimation {
            id: dealAnim
            // Flop staggered (0 / 140 / 280 ms); turn/river without a delay
            PauseAnimation { duration: slot.boardIndex < 3 ? slot.boardIndex * 220 : 0 }
            ScriptAction   { script: { slot._displayIndex = -1 } }
            // Pause: the backside is visible (analogous to dealCardsSpeed in the widget client)
            PauseAnimation { duration: 160 }
            // Now turn to the front side → the CardImage flip animation fires
            ScriptAction   { script: { slot._displayIndex = slot.actualCardIndex } }
            // Wait for the flip duration (CardImage: 170 + 300 ms) so that `revealing`
            // stays true until the end of the reveal animation.
            PauseAnimation { duration: 480 }
        }
    }

    // Soft glow behind the community cards → focus on the
    // centre of the table (subtle, warm).
    Rectangle {
        anchors.centerIn: cardRow
        width: cardRow.width + 80
        height: cardRow.height + 54
        radius: height / 2
        color: Qt.rgba(1.0, 0.93, 0.72, 0.12)
        z: -1
        layer.enabled: Config.Theme.effectsEnabled
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1.0
            blurMax: 48
            autoPaddingEnabled: true
        }
    }

    // 5 Slots: Flop (0-2) | Turn (3) | River (4)
    Row {
        id: cardRow
        anchors.centerIn: parent
        spacing: 3

        CommunitySlot { id: slot0; boardIndex: 0 }
        CommunitySlot { id: slot1; boardIndex: 1 }
        CommunitySlot { id: slot2; boardIndex: 2 }

        Item { width: 8; height: 1 }

        CommunitySlot { id: slot3; boardIndex: 3 }

        Item { width: 8; height: 1 }

        CommunitySlot { id: slot4; boardIndex: 4 }
    }

    // The pot prominently in the centre of the table (above the cards): chip icon +
    // amount with a golden glow. Pops when the pot grows (micro animation).
    Item {
        id: potBadge
        anchors.horizontalCenter: cardRow.horizontalCenter
        anchors.bottom: cardRow.top
        // The same distance to the card row as the winning hand badge
        // below it; portrait more compact (6) than landscape (8). Scales
        // with oppScale, being inside communityArea.
        anchors.bottomMargin: root.wide ? 8 : 6
        visible: (typeof GameTable !== "undefined" && GameTable) ? GameTable.totalPot > 0 : false
        width: potRow.width + 12
        height: 20
        transformOrigin: Item.Center

        // Background with a golden glow. A layer of its own so that the MultiEffect
        // renders only the pill (not the chip/text) – otherwise the text and the
        // puck would be overlaid by the glow and get blurry when scaled up (oppScale).
        Rectangle {
            anchors.fill: parent
            radius: 12
            color: Qt.rgba(0, 0, 0, 0.62)
            border.color: Config.Theme.colorAccent
            border.width: 1

            layer.enabled: Config.Theme.effectsEnabled
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Config.Theme.colorAccent
                shadowOpacity: 0.45
                shadowBlur: 0.9
                shadowVerticalOffset: 0
            }
        }

        // The chip + the amount lie above the glow and are rendered directly
        // (as vectors, without a layer texture) → they stay razor sharp.
        Row {
            id: potRow
            anchors.centerIn: parent
            z: 1
            spacing: 4
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 14; height: 14
                source: "../resources/chipStack.svg"
                fillMode: Image.PreserveAspectFit
            }
            AppText {
                anchors.verticalCenter: parent.verticalCenter
                text: "$" + (GameTable ? GameTable.totalPot : 0)
                color: Config.Theme.colorAccent
                font.pixelSize: 11
                font.bold: true
                font.letterSpacing: 0.3
            }
        }

        SequentialAnimation {
            id: potPop
            NumberAnimation { target: potBadge; property: "scale"; from: 1.0; to: 1.18; duration: 110; easing.type: Easing.OutQuad }
            NumberAnimation { target: potBadge; property: "scale"; to: 1.0; duration: 170; easing.type: Easing.OutBack }
        }
        Connections {
            target: (typeof GameTable !== "undefined") ? GameTable : null
            function onTotalPotChanged() {
                if (GameTable && GameTable.totalPot > 0) potPop.restart()
            }
        }
    }
}
