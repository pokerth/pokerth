import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: root

    property bool up: false
    property int maxAvatarSize: 60

    // Eigene Spielerdaten aus GameTable (Sitz 0 = Human Player)
    readonly property var selfData: (typeof GameTable !== "undefined" && GameTable && GameTable.players.length > 0)
        ? GameTable.players[0] : null

    readonly property int card0: selfData && selfData.card0 !== undefined ? selfData.card0 : -1
    readonly property int card1: selfData && selfData.card1 !== undefined ? selfData.card1 : -1
    readonly property bool isMyTurn: selfData ? selfData.myTurn : false
    // Am Zug: lokal über myTurn, im Netzwerk-Spiel über den Action-Timeout
    // (timeoutSeatId === 0). Beides, damit der Highlight in BEIDEN Modi erscheint.
    readonly property bool isAtTurn: root.isMyTurn
        || ((typeof GameTable !== "undefined" && GameTable) ? GameTable.timeoutSeatId === 0 : false)
    readonly property bool isWinner: typeof GameTable !== "undefined" && GameTable && GameTable.winnerSeatIds.indexOf(0) !== -1
    readonly property int button: selfData && selfData.button !== undefined ? selfData.button : 0
    readonly property int bet: selfData && selfData.bet !== undefined ? selfData.bet : 0

    // Letzte Aktion (0=keine,1=Fold,2=Check,3=Call,4=Bet,5=Raise,6=All-In)
    readonly property int action: selfData && selfData.action !== undefined ? selfData.action : 0
    readonly property string actionText: {
        switch (root.action) {
        case 1: return qsTr("Fold")
        case 2: return qsTr("Check")
        case 3: return qsTr("Call")
        case 4: return qsTr("Bet")
        case 5: return qsTr("Raise")
        case 6: return qsTr("All-In")
        default: return ""
        }
    }

    // Ich habe gefoldet → eigene Karten durchscheinend (wie im Qt-Widgets-Client)
    readonly property bool folded: selfData && selfData.folded !== undefined ? selfData.folded : false
    // Spieler im Spiel? Wer kein Geld mehr für die nächste Hand hat, ist inaktiv.
    readonly property bool playerActive: selfData && selfData.active !== undefined ? selfData.active : true
    // Gesetzter Avatar (file://-URL) bzw. "" → Platzhalter
    readonly property string avatarSource: selfData && selfData.avatar !== undefined ? selfData.avatar : ""

    // Im Landscape-Modus: 2-zeiliger Info-Bereich wie bei den Gegnerboxen
    // (Name oben / Stack rechts unten). Im Portrait bleibt es 1-zeilig.
    readonly property bool twoLineInfo: Config.Responsive.landscape

    color: "transparent"

    // Informationsdichte: gefoldet → dezent zurücknehmen, raus aus dem Spiel →
    // deutlich abdunkeln (analog zu den Gegnerboxen).
    opacity: !root.playerActive ? 0.4 : (root.folded ? 0.78 : 1.0)
    Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }

    // Am Zug leicht „angehoben" (Tiefe/Fokus, sanfter Übergang).
    scale: root.isAtTurn ? 1.03 : 1.0
    transformOrigin: Item.Center
    Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

    // Hintergrund mit dezentem Verlauf + weichem Schlagschatten → angehobene Karte.
    Rectangle {
        anchors.fill: parent
        radius: 6
        opacity: 0.9
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.lighter("#394150", 1.18) }
            GradientStop { position: 1.0; color: "#1d222b" }
        }
        border.color: Qt.rgba(1, 1, 1, 0.06)
        border.width: 1

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#000000"
            shadowOpacity: 0.42
            shadowBlur: 0.9
            shadowVerticalOffset: 3
            shadowHorizontalOffset: 0
        }
    }

    // Highlight: gold Rahmen + weicher Glow wenn ich am Zug bin, mit Puls.
    // Rahmen als eigene Ebene OHNE Layer (immer sichtbar), Glow als separate
    // gelayerte Ebene – so bleibt der Rahmen sichtbar, auch wenn der MultiEffect
    // auf einem System nicht rendert.
    Item {
        id: turnGlow
        anchors.fill: parent
        anchors.margins: -2
        z: 10
        visible: root.isAtTurn

        SequentialAnimation on opacity {
            running: root.isAtTurn
            loops: Animation.Infinite
            NumberAnimation { from: 0.65; to: 1.0; duration: 750; easing.type: Easing.InOutSine }
            NumberAnimation { from: 1.0; to: 0.65; duration: 750; easing.type: Easing.InOutSine }
        }

        // Weicher Außen-Glow (gelayert) – optional.
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: 6
            border.color: "#FFD54A"
            border.width: 2
            layer.enabled: root.isAtTurn
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#FFD700"
                shadowOpacity: 0.9
                shadowBlur: 1.0
                shadowVerticalOffset: 0
                shadowHorizontalOffset: 0
            }
        }

        // Gold-Rahmen (immer sichtbar, KEIN Layer).
        Rectangle {
            anchors.fill: parent
            color: "transparent"
            radius: 6
            border.color: "#CCFFD54A"
            border.width: 2
        }
    }

    // ── Karten – zentriert über der Infozeile ────────────────────────────────
    // Horizontale Abstände einheitlich: linker Außenrand = Abstand Avatar↔Karten
    // = rechter Außenrand = hMargin. Gleiches Maß (4) wie bei den Gegnerboxen
    // (GamePlayerBox.hMargin), damit Außenränder visuell konsistent sind.
    readonly property int hMargin: 4
    // Vertikale Abstände einheitlich: oberer Außenrand = Abstand Karten↔Text
    // = unterer Außenrand = vMargin.
    readonly property int vMargin: 4

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
            card0: root.card0
            card1: root.card1
            avatarSource: root.avatarSource
            folded: root.folded
            playerActive: root.playerActive
        }
    }

    // ── Name + Stack – unterer Info-Bereich ─────────────────────────────────────
    // Portrait: 1-zeilig (Name links, Stack rechts), Landscape: 2-zeilig wie
    // Gegnerbox (Name oben, Stack rechts unten). Höhe 18 → 32 im Landscape.
    Item {
        id: bottomBar
        anchors.bottom: parent.bottom
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

            Text {
                width: (parent.width - parent.spacing) / 2
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.selfData && root.selfData.name !== "" ? root.selfData.name : qsTr("Du")
            }

            Text {
                width: (parent.width - parent.spacing) / 2
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
                text: root.selfData ? "$" + root.selfData.stack : "$0"
            }
        }

        // Landscape: 2-zeilig (identisch zur Gegnerbox im wideLayout)
        Item {
            visible: root.twoLineInfo
            width: parent.width
            height: parent.height

            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.rightMargin: 2
                height: 16
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.selfData && root.selfData.name !== "" ? root.selfData.name : qsTr("Du")
            }

            Text {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
                text: root.selfData ? "$" + root.selfData.stack : "$0"
            }
        }
    }

    // Action-Badge: zentriert über den Hole-Cards (identisch zu GamePlayerBox).
    Rectangle {
        id: actionBadge
        visible: root.actionText !== "" && !root.isWinner
        readonly property real cardsCenterX: cardsArea.x + cardRow.x + cardRow.cardsCenterX
        readonly property real cardsCenterY: cardsArea.y + cardsArea.height / 2
        x: cardsCenterX - width / 2
        y: cardsCenterY - height / 2
        width: actionLabel.width + 16
        height: 18
        radius: 9
        z: 26
        // Farbe je Aktion (gleiche Logik wie die Action-Buttons, nur dunkler).
        color: Config.Theme.actionBadgeColor(root.action)
        border.color: Config.Theme.actionBadgeBorder(root.action)
        border.width: 1
        transformOrigin: Item.Center
        Behavior on color { ColorAnimation { duration: 200 } }
        Behavior on border.color { ColorAnimation { duration: 200 } }

        // Pop beim Erscheinen oder Wechsel einer Aktion (Mikroanimation).
        onVisibleChanged: if (visible) selfBadgePop.restart()
        Connections {
            target: root
            function onActionChanged() { if (actionBadge.visible) selfBadgePop.restart() }
        }
        SequentialAnimation {
            id: selfBadgePop
            NumberAnimation { target: actionBadge; property: "scale"; from: 0.6; to: 1.12; duration: 110; easing.type: Easing.OutQuad }
            NumberAnimation { target: actionBadge; property: "scale"; to: 1.0; duration: 120; easing.type: Easing.OutBack }
        }

        Text {
            id: actionLabel
            anchors.centerIn: parent
            text: root.actionText
            color: "#eaf1ff"
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: 12
            font.bold: true
        }
    }

    // Action-Timeout: Fortschrittsbalken, ebenfalls zentriert über den Hole-Cards.
    Item {
        id: timeoutBar
        readonly property bool active: (typeof GameTable !== "undefined" && GameTable)
                                       && GameTable.timeoutSeatId === 0
        property real progress: 1.0
        visible: active && !root.isWinner && root.actionText === ""
        x: actionBadge.cardsCenterX - width / 2
        y: actionBadge.cardsCenterY - height / 2
        width: 56
        height: 7
        z: 26

        // Track (statisch): Kontur + Dropshadow.
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: Config.Theme.colorTimeoutTrack
            border.color: Qt.rgba(1, 1, 1, 0.55)
            border.width: 1
            layer.enabled: timeoutBar.visible
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#000000"
                shadowOpacity: 0.6
                shadowBlur: 0.7
                shadowVerticalOffset: 1
                shadowHorizontalOffset: 0
            }
        }

        // Füllung (animiert) ÜBER dem Track – NICHT im Layer, damit die Breiten-
        // Animation zuverlässig läuft. Gleiches Blau wie bei den Gegnern, heller.
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 1
            height: parent.height - 2
            radius: height / 2
            color: Config.Theme.colorTimeoutSelf
            width: (parent.width - 2) * timeoutBar.progress
        }

        onActiveChanged: {
            if (active) {
                progress = 1.0
                timeoutAnim.restart()
            } else {
                timeoutAnim.stop()
            }
        }
        NumberAnimation {
            id: timeoutAnim
            target: timeoutBar
            property: "progress"
            from: 1.0; to: 0.0
            duration: ((typeof GameTable !== "undefined" && GameTable) ? GameTable.timeoutSec : 0) * 1000
            easing.type: Easing.Linear
        }
    }

    // Einsatz (Chip + Betrag) + Dealer/Small-/Big-Blind-Button oberhalb der Box.
    // Einsatz zentriert, Button rechtsbündig mit Außenabstand.
    Item {
        id: betGroup
        visible: root.bet > 0 || root.button > 0
        z: 25
        width: root.width
        height: Math.max(root.bet > 0 ? betRow.height : 0,
                         root.button > 0 ? buttonImg.height : 0)
        x: 0
        y: -height - 6

        Row {
            id: betRow
            visible: root.bet > 0
            spacing: 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            transformOrigin: Item.Center
            onVisibleChanged: if (visible) betPopSelf.restart()
            SequentialAnimation {
                id: betPopSelf
                NumberAnimation { target: betRow; property: "scale"; from: 0.5; to: 1.15; duration: 110; easing.type: Easing.OutQuad }
                NumberAnimation { target: betRow; property: "scale"; to: 1.0; duration: 130; easing.type: Easing.OutBack }
            }
            Image {
                width: 16; height: 16
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:resources/chipStack.svg"
                fillMode: Image.PreserveAspectFit
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: "#eff1f5"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 11
                font.bold: true
                text: "$" + root.bet
            }
        }

        Image {
            id: buttonImg
            visible: root.button > 0
            width: 24; height: 24
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            fillMode: Image.PreserveAspectFit
            source: root.button === 1 ? "../resources/tableDealerPuck.svg"
                  : root.button === 2 ? "../resources/tableSmallBlind.svg"
                  : root.button === 3 ? "../resources/tableBigBlind.svg"
                  : ""
        }
    }

    // Winner-Hervorhebung: goldener Rahmen + Badge
    Rectangle {
        anchors.fill: parent
        visible: root.isWinner
        color: "transparent"
        radius: 6
        border.color: "#FFD700"
        border.width: 3
        z: 19

        layer.enabled: root.isWinner
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#FFD700"
            shadowOpacity: 1.0
            shadowBlur: 1.0
            shadowVerticalOffset: 0
            shadowHorizontalOffset: 0
        }
    }

    Rectangle {
        visible: root.isWinner
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.top
        anchors.bottomMargin: 3
        width: winnerLabel.width + 14
        height: 18
        radius: 9
        color: "#0d3d0d"
        border.color: "#FFD700"
        border.width: 1
        z: 30

        Text {
            id: winnerLabel
            anchors.centerIn: parent
            text: qsTr("WINNER")
            color: "#FFD700"
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: 10
            font.bold: true
        }
    }
}
