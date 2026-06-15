import QtQuick
import QtQuick.Effects

import "../config" as Config

// Gemeinschaftskarten (Flop/Turn/River) + Pott-Badge in der Tischmitte.
// Größe = Kartenreihe; Position/Skalierung/z setzt der Aufrufer (anchors +
// scale), damit die Karten je nach Tisch-Layout zentriert bleiben.
Item {
    id: root

    // Querformat? Steuert den Abstand des Pott-Badges zur Kartenreihe.
    property bool wide: false

    width: cardRow.width
    height: cardRow.height
    transformOrigin: Item.Center

    // Inline-Komponente für einen einzelnen Board-Card-Slot
    // Karten-Seitenverhältnis 120:168 (≈0,714) – Karte = Platzhalter (1:1)
    component CommunitySlot: Item {
        property int boardIndex: 0
        width: 46; height: 64

        readonly property bool isDealt: {
            var cnt = (typeof GameTable !== "undefined" && GameTable)
                      ? GameTable.boardCardCount : 0
            return boardIndex < cnt
        }

        // Platzhalter-Rahmen (immer sichtbar)
        Rectangle {
            anchors.fill: parent
            radius: 4
            color: Qt.rgba(0, 0, 0, 0.30)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.38)
        }

        // Aufgedeckte Karte – mit Einblend-Animation
        CardImage {
            id: faceCard
            anchors.fill: parent
            opacity: 0
            cardIndex: {
                var cards = (typeof GameTable !== "undefined" && GameTable)
                            ? GameTable.boardCards : null
                return (cards && boardIndex < cards.length) ? cards[boardIndex] : -1
            }
        }

        onIsDealtChanged: {
            if (isDealt) {
                cardReveal.start()
            } else {
                faceCard.opacity = 0
            }
        }

        SequentialAnimation {
            id: cardReveal
            // Flop-Karten staffeln (0 ms, 120 ms, 240 ms); Turn/River sofort
            PauseAnimation { duration: boardIndex < 3 ? boardIndex * 120 : 0 }
            NumberAnimation {
                target: faceCard
                property: "opacity"
                from: 0; to: 1
                duration: 260
                easing.type: Easing.OutQuad
            }
        }
    }

    // Weicher Lichtschein hinter den Gemeinschaftskarten → Fokus auf die
    // Tischmitte (dezent, warm).
    Rectangle {
        anchors.centerIn: cardRow
        width: cardRow.width + 80
        height: cardRow.height + 54
        radius: height / 2
        color: Qt.rgba(1.0, 0.93, 0.72, 0.12)
        z: -1
        layer.enabled: true
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

        CommunitySlot { boardIndex: 0 }
        CommunitySlot { boardIndex: 1 }
        CommunitySlot { boardIndex: 2 }

        Item { width: 8; height: 1 }

        CommunitySlot { boardIndex: 3 }

        Item { width: 8; height: 1 }

        CommunitySlot { boardIndex: 4 }
    }

    // Pot prominent in der Tischmitte (über den Karten): Chip-Icon +
    // Betrag mit goldenem Glow. Poppt bei Pot-Erhöhung (Mikroanimation).
    Rectangle {
        id: potBadge
        anchors.horizontalCenter: cardRow.horizontalCenter
        anchors.bottom: cardRow.top
        // Gleicher Abstand zur Kartenreihe wie das Winning-Hand-Badge
        // darunter; Portrait kompakter (6) als Querformat (8). Skaliert
        // mit oppScale, da innerhalb communityArea.
        anchors.bottomMargin: root.wide ? 8 : 6
        visible: (typeof GameTable !== "undefined" && GameTable) ? GameTable.totalPot > 0 : false
        width: potRow.width + 16
        height: 24
        radius: 12
        color: Qt.rgba(0, 0, 0, 0.62)
        border.color: Config.Theme.colorAccent
        border.width: 1
        transformOrigin: Item.Center

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: Config.Theme.colorAccent
            shadowOpacity: 0.45
            shadowBlur: 0.9
            shadowVerticalOffset: 0
        }

        Row {
            id: potRow
            anchors.centerIn: parent
            spacing: 4
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 16; height: 16
                source: "../resources/chipStack.svg"
                fillMode: Image.PreserveAspectFit
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "$" + (GameTable ? GameTable.totalPot : 0)
                color: Config.Theme.colorAccent
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 13
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
