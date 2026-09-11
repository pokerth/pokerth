import QtQuick
import QtQuick.Effects

import "../config" as Config

// Winning hand (e.g. "full house") – only during the showdown.
// Deliberately a standalone element (NOT inside the community cards), so that
// it always sits ABOVE the player boxes independently of their z/scale – in
// portrait AND landscape. Positioned just below the (scaled) community cards.
Rectangle {
    id: winHandBadge

    // Reference element (community cards) for the vertical position.
    property Item community: null
    property bool wide: false
    property real communityScale: 1.0

    z: 50   // above the boxes (z:1), below the overlays (z:150)
    visible: (typeof GameTable !== "undefined" && GameTable)
             ? GameTable.winningHandText !== "" : false
    anchors.horizontalCenter: parent.horizontalCenter
    // Distance to the card row identical to the pot badge above (portrait 6,
    // landscape 8 – each · communityScale). It starts from the (scaled)
    // centre of the community cards and thus follows their centring in
    // portrait AND landscape.
    y: community
       ? community.y + community.height / 2
         + (community.height * community.scale) / 2
         + (wide ? 8 : 6) * community.scale
       : 0
    width: winHandLabel.implicitWidth + 18
    height: Math.max(17, Math.round(22 * communityScale))
    radius: height / 2
    color: Qt.rgba(0.05, 0.24, 0.05, 0.92)
    border.color: "#FFD700"
    border.width: 1
    transformOrigin: Item.Center

    // Same soft glow as the pot badge – here in gold to match the
    // frame, so that the winning hand is highlighted just as much.
    layer.enabled: Config.Theme.effectsEnabled
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#FFD700"
        shadowOpacity: 0.45
        shadowBlur: 0.9
        shadowVerticalOffset: 0
    }

    AppText {
        id: winHandLabel
        anchors.centerIn: parent
        text: (typeof GameTable !== "undefined" && GameTable)
              ? GameTable.winningHandText : ""
        color: "#FFD700"
        font.pixelSize: Math.max(9, Math.round(12 * winHandBadge.communityScale))
        font.bold: true
    }

    // Pops when the winning hand appears – analogous to potPop.
    SequentialAnimation {
        id: winHandPop
        NumberAnimation { target: winHandBadge; property: "scale"; from: 1.0; to: 1.18; duration: 110; easing.type: Easing.OutQuad }
        NumberAnimation { target: winHandBadge; property: "scale"; to: 1.0; duration: 170; easing.type: Easing.OutBack }
    }
    Connections {
        target: (typeof GameTable !== "undefined") ? GameTable : null
        function onWinningHandTextChanged() {
            if (GameTable && GameTable.winningHandText !== "") winHandPop.restart()
        }
    }
}
