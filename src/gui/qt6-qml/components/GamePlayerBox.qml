import QtQuick

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

    // Effektive Tisch-Skalierung dieser Box (oppScale × Zoom), an die Karten
    // weitergereicht, damit ihr SVG-Raster die echte Bildschirmgröße trifft.
    property real cardRenderScale: 1.0

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
    readonly property string actionText: Config.StaticData.pokerActionWord(root.action, dontTranslatePokerTerms)

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
        PlayerBoxBackground {}

        // Highlight: aktiver Spieler bekommt einen gold Rahmen + weichen Glow.
        PlayerTurnGlow { active: root.isAtTurn }

        // Avatar + Karten: AvatarCardRow garantiert cardH == topRowH (keine
        // Rundungsdifferenz). Abstände: 4 px links, 4 px Avatar↔Karten,
        // 4 px zwischen den Karten, 4 px rechts (= implicitWidth-Formel oben).
        AvatarCardRow {
            id: cardRow
            x: playerBox.hMargin
            y: 4
            height: root.wideLayout ? (parent.height - 44) : (parent.height - 28)

            cardRenderScale: root.cardRenderScale
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

            AppText {
                width: parent.width / 2
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0.3
                elide: Text.ElideRight
                text: root.seatData && root.seatData.name !== "" ? root.seatData.name : "---"
            }

            AppText {
                width: parent.width / 2
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
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

            AppText {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.rightMargin: 2
                horizontalAlignment: Text.AlignLeft
                color: "#eff1f5"
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

            AppText {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                horizontalAlignment: Text.AlignRight
                color: Config.Theme.colorAccent
                font.pixelSize: 15
                font.bold: true
                text: root.seatData && root.seatData.name !== "" ? "$" + root.seatData.stack : ""
            }
        }

    }

    // Winner-Hervorhebung: goldener Rahmen (verdeckt die Karten NICHT) +
    // „WINNER"-Badge. Badge standardmäßig über der Box; nur die oberste Box
    // (winnerBelow) zeigt es unterhalb, sonst stieße es am Bildschirmrand an.
    PlayerWinnerOverlay {
        active: root.isWinner
        below: root.winnerBelow
    }

    // Aktions-Anzeige (Fold/Check/Call/Bet/Raise/All-In) – zentriert über den
    // Hole-Cards in den normalen Player-Boxen.
    PlayerActionBadge {
        id: actionBadge
        visible: root.actionText !== "" && !root.isWinner
        action: root.action
        label: root.actionText
        z: 18

        readonly property real cardsCenterX: playerBox.hMargin + cardRow.cardsCenterX
        readonly property real cardsCenterY: cardRow.y + cardRow.height / 2
        x: cardsCenterX - width / 2
        y: cardsCenterY - height / 2
    }

    // Action-Timeout: schlanker Fortschrittsbalken an der Stelle des Action-
    // Badges, solange dieser Sitz am Zug ist (zählt über die Timeout-Dauer runter).
    PlayerTimeoutBar {
        id: timeoutBar
        readonly property bool atTurn: (typeof GameTable !== "undefined" && GameTable)
                                       && GameTable.timeoutSeatId === root.seatIndex
        active: atTurn
        visible: atTurn && !root.isWinner && root.actionText === ""
        width: 44
        height: 9
        z: 18
        x: actionBadge.cardsCenterX - width / 2
        y: actionBadge.cardsCenterY - height / 2
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

        BetChip {
            id: betRow
            visible: root.bet > 0
            amount: root.bet
            textColor: "#f0f0f0"
            // split: Einsatz rechts NEBEN der Box; sonst innerhalb zentriert.
            x: betGroup.split ? betGroup.width + 8 : (betGroup.width - width) / 2
            y: (betGroup.height - height) / 2
        }

        // Dealer/Blind-Button – split: links NEBEN der Box; horizontal:
        // rechtsbündig 6px vom Boxrand; Seiten: unterer Slot.
        BlindButtonImage {
            id: buttonImg
            visible: root.buttonVisible
            button: root.button
            x: betGroup.split
               ? -width - 8
               : betGroup.horizontal
               ? (betGroup.width - width - 6)
               : (root.betSide === "right" ? 0 : (betGroup.width - width))
            y: (betGroup.horizontal || betGroup.split)
               ? (betGroup.height - height) / 2
               : (betGroup.height * 5 / 6 - height / 2)
        }
    }
}
