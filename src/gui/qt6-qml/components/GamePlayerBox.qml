import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts

import "../config" as Config

Item {
    id: root

    property bool up: false
    property int seatIndex: 0
    // Winner-Badge unterhalb statt oberhalb der Box anzeigen – nur für die oberste
    // Box (Player 5) im Hochformat sinnvoll, sonst würde es oben anstoßen.
    property bool winnerBelow: false
    // Seite, auf der Einsatz-Chip + Dealer/Blind-Button angezeigt werden:
    // "top" | "bottom" | "left" | "right". Default leitet sich aus 'up' ab.
    property string betSide: up ? "bottom" : "top"
    // Geteilte Anzeige: Dealer/Blind-Button LINKS neben der Box, Einsatz
    // RECHTS neben der Box – beides vertikal mittig. Für die oberste Box im
    // landscapeCompact, deren Badge sonst unterhalb mit dem Pot-Badge
    // kollidiert. Übersteuert betSide.
    property bool betSplit: false

    // Dynamische Breite: 2×hMargin(4) + AvatarCardRow.implicitWidth(avatarH+4+2·cardW+4)
    readonly property int _topRowH: height - (wideLayout ? 44 : 28)
    readonly property int _cardW:   Math.round(_topRowH * 120 / 168)
    implicitWidth: 2 * 4 + _topRowH + 4 + 2 * _cardW + 4
    implicitHeight: 84

    // Spielerdaten aus GameTable
    readonly property var seatData: (typeof GameTable !== "undefined" && GameTable && GameTable.players.length > seatIndex)
        ? GameTable.players[seatIndex] : null

    readonly property int card0: seatData && seatData.card0 !== undefined ? seatData.card0 : -1
    readonly property int card1: seatData && seatData.card1 !== undefined ? seatData.card1 : -1
    // Showdown-Spotlight: einzelne Hole-Card des Gewinners abblenden, wenn sie
    // nicht zum Siegerblatt zählt (vom GameHandler gesetzt).
    readonly property bool fade0: seatData && seatData.fade0 !== undefined ? seatData.fade0 : false
    readonly property bool fade1: seatData && seatData.fade1 !== undefined ? seatData.fade1 : false
    readonly property bool isMyTurn: seatData ? seatData.myTurn : false
    // Aktiver Spieler (am Zug): lokal über seatData.myTurn (Engine setzt
    // getMyTurn()), im Netzwerk-Spiel über den Action-Timeout (timeoutSeatId) –
    // dort ist myTurn clientseitig nicht gesetzt. Beides berücksichtigen, damit
    // der Highlight-Rahmen in BEIDEN Spielarten erscheint.
    readonly property bool isAtTurn: root.isMyTurn
        || ((typeof GameTable !== "undefined" && GameTable) ? GameTable.timeoutSeatId === root.seatIndex : false)
    readonly property bool isActive: seatData ? seatData.active : false
    readonly property bool isWinner: typeof GameTable !== "undefined" && GameTable && GameTable.winnerSeatIds.indexOf(root.seatIndex) !== -1
    readonly property int button: seatData && seatData.button !== undefined ? seatData.button : 0
    readonly property int bet: seatData && seatData.bet !== undefined ? seatData.bet : 0
    // Einstellung „Symbole für Small/Big Blind anzeigen" (Config-Key
    // ShowBlindButtons). Wie im Qt-Widgets-Client wird der Dealer-Button (1)
    // immer gezeigt, nur Small-Blind (2) und Big-Blind (3) sind abschaltbar.
    readonly property bool showBlindButtons:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowBlindButtons") !== 0 : true
    readonly property bool buttonVisible:
        button === 1 || ((button === 2 || button === 3) && showBlindButtons)
    // Spieler hat gefoldet → Karten durchscheinend (wie im Qt-Widgets-Client)
    readonly property bool folded: seatData && seatData.folded !== undefined ? seatData.folded : false
    // Gesetzter Avatar (file://-URL) bzw. "" → Platzhalter
    readonly property string avatarSource: seatData && seatData.avatar !== undefined ? seatData.avatar : ""

    // Letzte Aktion dieses Spielers (0=keine,1=Fold,2=Check,3=Call,4=Bet,5=Raise,6=All-In)
    readonly property int action: seatData && seatData.action !== undefined ? seatData.action : 0
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

    // Länderflagge: Lookup über gamePlayersInGame – identisch zu GameWaitPage,
    // wo es zuverlässig funktioniert. playerListRevision erzwingt Reaktivität.
    readonly property string countryCode: {
        if (typeof Lobby === "undefined" || !Lobby || !root.seatData) return ""
        var _p = Lobby.playerListRevision
        var _g = Lobby.gameListRevision
        var pname = root.seatData.name
        if (!pname) return ""
        var gp = Lobby.gamePlayersInGame(Lobby.currentGameId)
        for (var i = 0; i < gp.length; i++)
            if (gp[i].playerName === pname) return gp[i].countryCode || ""
        return ""
    }
    // Widescreen-Layout: Box ist groß genug für 2-zeilige Info (Name + Flagge/Cash).
    // Nutzt height >= 76 als Proxy für tableZone.wide (oppBaseHeight = wide ? 84 : 71).
    // Bewusst NICHT Config.Responsive.landscape – die Tablezone kann breiter als
    // hoch sein, auch wenn das Gesamtfenster (inkl. Toolbar) hochformat-mäßig ist.
    readonly property bool wideLayout: height >= 76

    // Nur anzeigen wenn der Sitz besetzt ist
    visible: root.seatData !== null && root.seatData.name !== ""

    // Informationsdichte: wer raus ist (kein Geld mehr → !isActive) wird deutlich
    // abgedunkelt, wer nur gefoldet hat dezent zurückgenommen. So heben sich der
    // aktive Spieler und die noch laufende Hand klarer hervor.
    opacity: !root.isActive ? Config.Theme.dimmedOpacity
           : (root.folded ? 0.72 : 1.0)
    Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }

    // ── Hauptbox ────────────────────────────────────────────────────────────────
    Rectangle {
        id: playerBox
        anchors.fill: parent
        color: "transparent"
        property int hMargin: 4

        // Aktiver Spieler leicht „angehoben" → mehr Tiefe/Fokus (sanfter Übergang).
        scale: root.isAtTurn ? 1.04 : 1.0
        transformOrigin: Item.Center
        Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

        // Karten-Hintergrund mit dezentem Verlauf + weichem Schlagschatten → die
        // Box wirkt als angehobene Karte statt als flache Fläche.
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

        // Highlight: aktiver Spieler bekommt einen gold Rahmen + weichen Glow,
        // mit ruhigem Puls. WICHTIG: der Rahmen liegt als eigene Ebene OHNE Layer
        // vor, der weiche Glow als separate gelayerte Ebene dahinter. So bleibt
        // der Rahmen sichtbar, selbst wenn der MultiEffect-Glow auf einem System
        // nicht rendert (war zuvor in EINEM gelayerten Rechteck → bei Layer-
        // Problemen verschwand der Rahmen mit).
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

            // Weicher Außen-Glow (gelayert) – reine Eye-Candy, optional.
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                radius: 6
                border.color: "#FFD54A"
                border.width: 1
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
                border.width: 1
            }
        }

        // Avatar + Karten: AvatarCardRow garantiert cardH == topRowH (keine
        // Rundungsdifferenz). Abstände: 4 px links, 4 px Avatar↔Karten,
        // 4 px zwischen den Karten, 4 px rechts (= implicitWidth-Formel oben).
        AvatarCardRow {
            id: cardRow
            x: playerBox.hMargin
            y: 4
            height: root.wideLayout ? (parent.height - 44) : (parent.height - 28)

            card0: root.card0
            card1: root.card1
            fade0: root.fade0
            fade1: root.fade1
            avatarSource: root.avatarSource
            folded: root.folded
            playerActive: root.isActive
        }

        // Portrait: Name + Stack einzeilig
        Row {
            visible: !root.wideLayout
            width: parent.width - 2 * playerBox.hMargin
            height: 15
            x: playerBox.hMargin
            y: parent.height - height - 4

            Text {
                width: parent.width / 2
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.seatData && root.seatData.name !== "" ? root.seatData.name : "---"
            }

            Text {
                width: parent.width / 2
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                font.bold: true
                text: root.seatData && root.seatData.name !== "" ? "$" + root.seatData.stack : ""
            }
        }

        // Widescreen: Name + Flagge + Stack 2-zeilig
        Item {
            id: infoBar
            visible: root.wideLayout
            width: parent.width - 2 * playerBox.hMargin
            height: 36
            x: playerBox.hMargin
            y: parent.height - height - 4

            Text {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.rightMargin: 2
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.seatData && root.seatData.name !== "" ? root.seatData.name : "---"
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

            Text {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 15
                font.bold: true
                text: root.seatData && root.seatData.name !== "" ? "$" + root.seatData.stack : ""
            }
        }

        // Winner-Hervorhebung: goldener Rahmen – verdeckt die Karten NICHT
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
    }

    // WINNER-Badge: standardmäßig über der Box; nur die oberste Box (Player 5)
    // zeigt es unterhalb (oben würde es am Bildschirmrand anstoßen). Etwas mehr
    // vertikaler Abstand zur Box.
    Rectangle {
        visible: root.isWinner
        anchors.horizontalCenter: parent.horizontalCenter
        // Vertikal über bzw. unter der Box per explizitem y – ein bedingter
        // anchors-Wechsel mit `undefined` ist fragil (Anchor fällt weg → Badge
        // landet mittig in der Box). Unterhalb (winnerBelow) bzw. oberhalb.
        y: root.winnerBelow ? (parent.height + 6) : (-height - 6)
        width: winnerLabel.width + 12
        height: 16
        radius: 8
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
            font.pixelSize: 9
            font.bold: true
        }
    }

    // Aktions-Anzeige (Fold/Check/Call/Bet/Raise/All-In) – zentriert über den
    // Hole-Cards in den normalen Player-Boxen.
    Rectangle {
        id: actionBadge
        visible: root.actionText !== "" && !root.isWinner
        width: actionLabel.width + 14
        height: 18
        radius: 9
        // Farbe je Aktion (gleiche Logik wie die Action-Buttons, nur dunkler).
        color: Config.Theme.actionBadgeColor(root.action)
        border.color: Config.Theme.actionBadgeBorder(root.action)
        border.width: 1
        z: 18
        transformOrigin: Item.Center
        Behavior on color { ColorAnimation { duration: 200 } }
        Behavior on border.color { ColorAnimation { duration: 200 } }

        // Pop beim Erscheinen oder Wechsel einer Aktion (Mikroanimation).
        onVisibleChanged: if (visible) badgePop.restart()
        Connections {
            target: root
            function onActionChanged() { if (actionBadge.visible) badgePop.restart() }
        }
        SequentialAnimation {
            id: badgePop
            NumberAnimation { target: actionBadge; property: "scale"; from: 0.6; to: 1.12; duration: 110; easing.type: Easing.OutQuad }
            NumberAnimation { target: actionBadge; property: "scale"; to: 1.0; duration: 120; easing.type: Easing.OutBack }
        }

        readonly property real cardsCenterX: playerBox.hMargin + cardRow.cardsCenterX
        readonly property real cardsCenterY: cardRow.y + cardRow.height / 2
        x: cardsCenterX - width / 2
        y: cardsCenterY - height / 2

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

    // Action-Timeout: schlanker Fortschrittsbalken an der Stelle des Action-
    // Badges, solange dieser Sitz am Zug ist (zählt über die Timeout-Dauer runter).
    Item {
        id: timeoutBar
        readonly property bool active: (typeof GameTable !== "undefined" && GameTable)
                                       && GameTable.timeoutSeatId === root.seatIndex
        property real progress: 1.0
        visible: active && !root.isWinner && root.actionText === ""
        width: 44
        height: 9
        z: 18
        x: actionBadge.cardsCenterX - width / 2
        y: actionBadge.cardsCenterY - height / 2

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

        // Füllung (animiert) ÜBER dem Track – bewusst NICHT im Layer, damit die
        // Breiten-Animation zuverlässig läuft.
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 1
            height: parent.height - 2
            radius: height / 2
            color: Config.Theme.colorTimeout
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

    // Einsatz (Chip + Betrag) + Dealer/Small-/Big-Blind-Button – gruppiert.
    // Oben/unten-Mitte (betSide top/bottom): volle Boxbreite; Einsatz zentriert,
    // Button rechtsbündig mit 6px Außenabstand – identisch zur Self-Box.
    // Seiten (betSide left/right): Button unter dem Einsatz, beides vertikal zentriert.
    Item {
        id: betGroup
        visible: root.bet > 0 || root.buttonVisible
        z: 25

        readonly property bool split: root.betSplit
        readonly property bool horizontal: !split && (root.betSide === "bottom" || root.betSide === "top")
        readonly property real betW: root.bet > 0 ? betRow.width : 0
        readonly property real betH: root.bet > 0 ? betRow.height : 0
        readonly property real btnW: root.buttonVisible ? buttonImg.width : 0
        readonly property real btnH: root.buttonVisible ? buttonImg.height : 0

        width: (horizontal || split) ? playerBox.width : Math.max(betW, btnW)
        height: horizontal ? Math.max(betH, btnH) : playerBox.height

        x: split ? 0
         : root.betSide === "right" ? playerBox.width + 8
         : root.betSide === "left"  ? -width - 8
         : 0
        y: split ? 0
         : root.betSide === "bottom" ? playerBox.height + 7
         : root.betSide === "top"    ? -height - 7
         : (playerBox.height - height) / 2

        Row {
            id: betRow
            visible: root.bet > 0
            spacing: 2
            // split: Einsatz rechts NEBEN der Box; sonst innerhalb zentriert.
            x: betGroup.split ? betGroup.width + 8 : (betGroup.width - width) / 2
            y: (betGroup.height - height) / 2
            transformOrigin: Item.Center
            // Chip „poppt" beim Setzen rein (Mikroanimation).
            onVisibleChanged: if (visible) betPop.restart()
            SequentialAnimation {
                id: betPop
                NumberAnimation { target: betRow; property: "scale"; from: 0.5; to: 1.15; duration: 110; easing.type: Easing.OutQuad }
                NumberAnimation { target: betRow; property: "scale"; to: 1.0; duration: 130; easing.type: Easing.OutBack }
            }

            Image {
                width: 16
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:resources/chipStack.svg"
                fillMode: Image.PreserveAspectFit
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: "#f0f0f0"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 11
                font.bold: true
                text: "$" + root.bet
            }
        }

        // Dealer/Blind-Button – split: links NEBEN der Box; horizontal:
        // rechtsbündig 6px vom Boxrand; Seiten: unterer Slot.
        Image {
            id: buttonImg
            visible: root.buttonVisible
            width: 24
            height: 24
            fillMode: Image.PreserveAspectFit
            x: betGroup.split
               ? -width - 8
               : betGroup.horizontal
               ? (betGroup.width - width - 6)
               : (root.betSide === "right" ? 0 : (betGroup.width - width))
            y: (betGroup.horizontal || betGroup.split)
               ? (betGroup.height - height) / 2
               : (betGroup.height * 5 / 6 - height / 2)
            source: root.button === 1 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.dealerPuck !== "") ? StyleProvider.dealerPuck : "../resources/tableDealerPuck.svg")
                  : root.button === 2 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.smallBlindPuck !== "") ? StyleProvider.smallBlindPuck : "../resources/tableSmallBlind.svg")
                  : root.button === 3 ? ((typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.bigBlindPuck !== "") ? StyleProvider.bigBlindPuck : "../resources/tableBigBlind.svg")
                  : ""
        }
    }
}
