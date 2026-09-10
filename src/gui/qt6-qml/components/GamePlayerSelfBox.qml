import QtQuick

import "../config" as Config

SeatBox {
    id: root

    property int maxAvatarSize: 60

    // Der eigene Sitz ist Sitz 0 - alle Ableitungen daraus liefert SeatBox.
    seatIndex: 0

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

    // Spieler im Spiel? Wer kein Geld mehr für die nächste Hand hat, ist inaktiv.
    // Eigener Default (true) gegenüber der Gegnerbox.
    readonly property bool playerActive: seatData && seatData.active !== undefined ? seatData.active : true

    // Im Landscape-Modus: 2-zeiliger Info-Bereich wie bei den Gegnerboxen
    // (Name oben / Stack rechts unten). Im Portrait bleibt es 1-zeilig.
    readonly property bool twoLineInfo: Config.Responsive.landscape

    // Einsatz-Sockel (betInset/betStripH) und Sockel-Aufklappen: siehe SeatBox.
    // Horizontale Abstände einheitlich: linker Außenrand = Abstand Avatar↔Karten
    // = rechter Außenrand = hMargin. Gleiches Maß (4) wie bei den Gegnerboxen
    // (GamePlayerBox.hMargin), damit Außenränder visuell konsistent sind.
    readonly property int hMargin: 4
    // Vertikale Abstände einheitlich: oberer Außenrand = Abstand Karten↔Text
    // = unterer Außenrand = vMargin.
    readonly property int vMargin: 4

    // Informationsdichte: gefoldet → dezent zurücknehmen, raus aus dem Spiel →
    // deutlich abdunkeln (analog zu den Gegnerboxen).
    opacity: !root.playerActive ? 0.4 : (root.folded ? 0.78 : 1.0)
    Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutQuad } }

    // Am Zug leicht „angehoben" (Tiefe/Fokus, sanfter Übergang).
    scale: root.isAtTurn ? 1.03 : 1.0
    transformOrigin: Item.Center
    Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }

    // ── Boxkörper (ohne den Einsatz-Sockel) ──────────────────────────────────
    // `root.height` ist die am Tisch permanent RESERVIERTE Höhe inklusive
    // Sockel. Der Körper sitzt oben darin, der Sockel klappt nach unten in den
    // freigehaltenen Rest auf. So wächst die Box nur bei tatsächlichem Einsatz,
    // ohne dass sich Tisch-Skalierung oder Nachbarboxen bewegen.
    Item {
        id: bodyBox
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: root.bodyH + betStrip.height

        // Hintergrund mit dezentem Verlauf + weichem Schlagschatten → angehobene Karte.
        PlayerBoxBackground {}

        // Highlight: gold Rahmen + weicher Glow wenn ich am Zug bin (Rahmen etwas
        // kräftiger als bei den Gegnerboxen).
        PlayerTurnGlow {
            active: root.isAtTurn
            borderWidth: 2
        }

        // ── Karten – zentriert über der Infozeile ────────────────────────────────
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
                cardRenderScale: root.cardRenderScale
                card0: root.card0
                card1: root.card1
                fade0: root.fade0
                fade1: root.fade1
                antiPeek: root.antiPeek
                showNetworkStatus: root.showPingState && root.pingState > 0
                networkStatusColor: root.pingColor
                networkPingAvg: (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingAvg : -1
                networkPingMin: (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingMin : -1
                networkPingMax: (typeof GameTable !== "undefined" && GameTable) ? GameTable.pingMax : -1
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
            anchors.bottom: betStrip.top
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

                AppText {
                    width: (parent.width - parent.spacing) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignLeft
                    color: "#eff1f5"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.3
                    elide: Text.ElideRight
                    text: root.seatData && root.seatData.name !== "" ? root.seatData.name : qsTr("Du")
                }

                AppText {
                    width: (parent.width - parent.spacing) / 2
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    color: Config.Theme.colorAccent
                    font.pixelSize: 15
                    font.bold: true
                    text: root.seatData ? "$" + root.seatData.stack : "$0"
                }
            }

            // Landscape: 2-zeilig (identisch zur Gegnerbox im wideLayout)
            Item {
                visible: root.twoLineInfo
                width: parent.width
                height: parent.height

                AppText {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    height: 16
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    color: "#eff1f5"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.3
                    elide: Text.ElideRight
                    text: root.seatData && root.seatData.name !== "" ? root.seatData.name : qsTr("Du")
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
                    text: root.seatData ? "$" + root.seatData.stack : "$0"
                }
            }
        }

        // Einsatz-Sockel am unteren Boxrand (Sitz-Stil "inset"). Klappt nur auf,
        // solange wirklich ein Einsatz steht; im Stil "classic" bleibt er dauerhaft
        // 0 px hoch – dann liegt sein oberer Rand auf bodyBox.bottom und der
        // Info-Bereich darüber sitzt exakt wie zuvor.
        PlayerBetStrip {
            id: betStrip
            open: root.stripOpen
            amount: root.bet
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }

        // Winner-Hervorhebung: goldener Rahmen + „WINNER"-Badge über der Box.
        // Kind von bodyBox: der Rahmen soll den KÖRPER umschließen, nicht die
        // darunter reservierte, leere Sockelhöhe.
        PlayerWinnerOverlay {
            active: root.isWinner
            gap: 3
            badgeHeight: 18
            badgeFontSize: 10
            hPadding: 14
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
        PlayerActionBadge {
            id: actionBadge
            visible: root.actionText !== "" && !root.isWinner
            action: root.action
            label: root.actionText
            hPadding: 16
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            z: 2
        }

        // Action-Timeout: Fortschrittsbalken horizontal zentriert über der Box,
        // exklusiv zum Badge. Gleiches Blau wie bei den Gegnern, heller.
        PlayerTimeoutBar {
            id: timeoutBar
            readonly property bool atTurn: (typeof GameTable !== "undefined" && GameTable)
                                           && GameTable.timeoutSeatId === root.seatIndex
            active: atTurn
            visible: atTurn && !root.isWinner && root.actionText === ""
            fillColor: Config.Theme.colorTimeoutSelf
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: 56
            height: 7
            z: 2
        }

        // Einsatz (Chip + Betrag): links neben dem Action-Indikator. Ist keiner
        // sichtbar, rückt der Einsatz rechtsbündig an den Boxrand.
        BetChip {
            id: betRow
            visible: root.bet > 0 && !root.betInset
            amount: root.bet
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: topStrip.betRightMargin
        }
    }

    // Dealer/Small-/Big-Blind-Button: rechts oben NEBEN der Box (außerhalb).
    BlindButtonImage {
        id: buttonImg
        visible: root.buttonVisible
        button: root.button
        anchors.left: parent.right
        anchors.leftMargin: 6
        anchors.top: parent.top
        z: 25
    }
}
