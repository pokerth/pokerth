import QtQuick
import QtQuick.Effects

import "../config" as Config

// Gewinner-Hand (z.B. "Full House") – nur während des Showdowns.
// Bewusst als eigenständiges Element (NICHT in den Community-Cards), damit es
// unabhängig von deren z/Scale immer ÜBER den Spielerboxen liegt – in Hoch-
// UND Querformat. Positioniert knapp unter den (skalierten) Community Cards.
Rectangle {
    id: winHandBadge

    // Bezugselement (Community-Cards) für die vertikale Position.
    property Item community: null
    property bool wide: false
    property real communityScale: 1.0

    z: 50   // über Boxen (z:1), unter den Overlays (z:150)
    visible: (typeof GameTable !== "undefined" && GameTable)
             ? GameTable.winningHandText !== "" : false
    anchors.horizontalCenter: parent.horizontalCenter
    // Abstand zur Kartenreihe identisch zum Pot-Badge oben (Portrait 6,
    // Querformat 8 – jeweils · communityScale). Setzt direkt am (skalierten)
    // Mittelpunkt der Community-Cards an, folgt damit deren Zentrierung in
    // Hoch- UND Querformat.
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

    // Gleicher weicher Schein wie das Pot-Badge – hier in Gold passend
    // zum Rahmen, damit die Gewinner-Hand ebenso hervorgehoben wird.
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

    // Poppt beim Erscheinen der Gewinner-Hand – analog potPop.
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
