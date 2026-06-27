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
    // Showdown-Spotlight: eigene Hole-Card abblenden, wenn sie nicht zum
    // Siegerblatt zählt (vom GameHandler gesetzt).
    readonly property bool fade0: selfData && selfData.fade0 !== undefined ? selfData.fade0 : false
    readonly property bool fade1: selfData && selfData.fade1 !== undefined ? selfData.fade1 : false
    // Anti-Peek (Config-Key AntiPeekMode): eigene Hole-Cards verdeckt halten,
    // nur per Hover/Drücken kurz aufdecken. readConfigInt ist nicht reaktiv –
    // greift ab der nächsten Instanziierung der Self-Box (Spielstart).
    readonly property bool antiPeek:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("AntiPeekMode") !== 0 : false

    // Netzwerkstatus-Ampel (Config-Key ShowPingStateInAvatar): nur am eigenen
    // Avatar und nur, sobald echte Ping-Daten vorliegen (pingState > 0).
    readonly property bool showPingState:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowPingStateInAvatar") !== 0 : false
    readonly property int pingState:
        (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingState : 0
    readonly property color pingColor: pingState === 1 ? "#43a047"   // grün
                                     : pingState === 2 ? "#fbc02d"   // gelb
                                     : pingState === 3 ? "#e53935"   // rot
                                     : "transparent"
    readonly property bool isMyTurn: selfData ? selfData.myTurn : false
    // Am Zug: lokal über myTurn, im Netzwerk-Spiel über den Action-Timeout
    // (timeoutSeatId === 0). Beides, damit der Highlight in BEIDEN Modi erscheint.
    readonly property bool isAtTurn: root.isMyTurn
        || ((typeof GameTable !== "undefined" && GameTable) ? GameTable.timeoutSeatId === 0 : false)
    readonly property bool isWinner: typeof GameTable !== "undefined" && GameTable && GameTable.winnerSeatIds.indexOf(0) !== -1
    readonly property int button: selfData && selfData.button !== undefined ? selfData.button : 0
    readonly property int bet: selfData && selfData.bet !== undefined ? selfData.bet : 0
    // Einstellung „Symbole für Small/Big Blind anzeigen" (Config-Key
    // ShowBlindButtons). Dealer-Button (1) immer; Small-(2)/Big-Blind (3)
    // abschaltbar – wie im Qt-Widgets-Client.
    readonly property bool showBlindButtons:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowBlindButtons") !== 0 : true
    readonly property bool buttonVisible:
        button === 1 || ((button === 2 || button === 3) && showBlindButtons)

    // Letzte Aktion (0=keine,1=Fold,2=Check,3=Call,4=Bet,5=Raise,6=All-In)
    readonly property int action: selfData && selfData.action !== undefined ? selfData.action : 0
    // Einstellung „Internationale Pokerausdrücke nicht übersetzen" (Config-Key
    // DontTranslateInternationalPokerStringsFromStyle): Aktions-Begriffe fest auf
    // Englisch statt lokalisiert. qsTr()-Literale bleiben für die Extraktion.
    readonly property bool dontTranslatePokerTerms:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
    readonly property string actionText: {
        switch (root.action) {
        case 1: return dontTranslatePokerTerms ? "Fold"   : qsTr("Fold")
        case 2: return dontTranslatePokerTerms ? "Check"  : qsTr("Check")
        case 3: return dontTranslatePokerTerms ? "Call"   : qsTr("Call")
        case 4: return dontTranslatePokerTerms ? "Bet"    : qsTr("Bet")
        case 5: return dontTranslatePokerTerms ? "Raise"  : qsTr("Raise")
        case 6: return dontTranslatePokerTerms ? "All-In" : qsTr("All-In")
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

        layer.enabled: Config.Theme.effectsEnabled
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

        // Puls nur bei aktivierten Effekten – sonst läuft eine Endlos-Animation,
        // die die GESAMTE Szene mit 60 fps neu zeichnen lässt.
        SequentialAnimation on opacity {
            running: Config.Theme.effectsEnabled && root.isAtTurn
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
            layer.enabled: Config.Theme.effectsEnabled && root.isAtTurn
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
            fade0: root.fade0
            fade1: root.fade1
            antiPeek: root.antiPeek
            showNetworkStatus: root.showPingState && root.pingState > 0
            networkStatusColor: root.pingColor
            avatarSource: root.avatarSource
            folded: root.folded
            playerActive: root.playerActive
        }
    }

    // Klick auf „Karten zeigen" → eigene Hole-Cards umdrehen als Bestätigung,
    // dass man wirklich zeigt (analog Widget-Client: showHoleCards → Flip).
    Connections {
        target: (typeof GameTable !== "undefined") ? GameTable : null
        function onMyCardsShown() { cardRow.playShowFlip() }
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

    // ── Strip OBERHALB der Box: Action-Indikator (Badge bzw. Timeout-Balken)
    //    rechtsbündig, Einsatz links davon. So werden die eigenen Hole-Cards
    //    nicht mehr verdeckt (waren zuvor mittig über den Karten platziert).
    Item {
        id: topStrip
        z: 26
        width: root.width
        height: 18
        x: 0
        y: -height - 6

        // Rechter Abstand des Einsatzes zum Boxrand, damit er links neben dem
        // sichtbaren Indikator sitzt: am rechtsbündigen Action-Badge bzw. – beim
        // BB/SB (Timeout läuft) – links neben dem horizontal zentrierten Balken.
        readonly property real betRightMargin:
              actionBadge.visible ? actionBadge.width + 8
            : timeoutBar.visible  ? (width / 2 + timeoutBar.width / 2 + 8)
            : 0

        // Action-Badge: rechtsbündig über der Box.
        Rectangle {
            id: actionBadge
            visible: root.actionText !== "" && !root.isWinner
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: actionLabel.width + 16
            height: 18
            radius: 9
            z: 2
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

        // Action-Timeout: Fortschrittsbalken horizontal zentriert über der Box,
        // exklusiv zum Badge.
        Item {
            id: timeoutBar
            readonly property bool active: (typeof GameTable !== "undefined" && GameTable)
                                           && GameTable.timeoutSeatId === 0
            property real progress: 1.0
            visible: active && !root.isWinner && root.actionText === ""
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: 56
            height: 7
            z: 2

            // Track (statisch): Kontur + Dropshadow.
            Rectangle {
                anchors.fill: parent
                radius: height / 2
                color: Config.Theme.colorTimeoutTrack
                border.color: Qt.rgba(1, 1, 1, 0.55)
                border.width: 1
                layer.enabled: Config.Theme.effectsEnabled && timeoutBar.visible
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

        // Einsatz (Chip + Betrag): links neben dem Action-Indikator. Ist keiner
        // sichtbar, rückt der Einsatz rechtsbündig an den Boxrand.
        Row {
            id: betRow
            visible: root.bet > 0
            spacing: 2
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: topStrip.betRightMargin
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
    }

    // Dealer/Small-/Big-Blind-Button: rechts oben NEBEN der Box (außerhalb).
    Image {
        id: buttonImg
        visible: root.buttonVisible
        width: 24; height: 24
        anchors.left: parent.right
        anchors.leftMargin: 6
        anchors.top: parent.top
        z: 25
        fillMode: Image.PreserveAspectFit
        source: root.button === 1 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.dealerPuck !== "") ? StyleProvider.dealerPuck : "../resources/tableDealerPuck.svg")
              : root.button === 2 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.smallBlindPuck !== "") ? StyleProvider.smallBlindPuck : "../resources/tableSmallBlind.svg")
              : root.button === 3 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.bigBlindPuck !== "") ? StyleProvider.bigBlindPuck : "../resources/tableBigBlind.svg")
              : ""
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

        layer.enabled: Config.Theme.effectsEnabled && root.isWinner
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
