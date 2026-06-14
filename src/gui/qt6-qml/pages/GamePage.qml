import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.VectorImage
import QtQuick.Window

import "../components"
import "../config" as Config

Rectangle {
    id: gamePage
    objectName: "gamePage"
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    color: "transparent"

    function applyPlayingMode(index) {
        if (!actionBar)
            return
        actionBar.playingMode = index
        // Falls bereits mein Zug: gewählten Auto-Modus ausführen – aber
        // VERZÖGERT (Qt.callLater), niemals synchron. Diese Funktion läuft u.a.
        // aus dem activated-Handler der Modus-ComboBox bzw. aus einem Shortcut.
        // fold()/call() verändert sofort den Spielzustand und löst ein erneutes
        // myTurnChanged + Re-Layout der ActionBar (inkl. dieser ComboBox) aus;
        // synchron mitten im Klick-/Signal-Handler führte das zu Re-Entrancy
        // (lokales Spiel fror ein, Netzwerk-Spiel stürzte ab).
        if (GameTable && GameTable.myTurn)
            Qt.callLater(gamePage.runAutoAction)
    }

    // Auto-Modus-Aktion im nächsten Event-Loop-Durchlauf ausführen. Der Zustand
    // wird erneut geprüft, da er sich seit der Planung geändert haben kann
    // (z.B. Zug bereits vorbei). Qt.callLater dedupliziert Mehrfachaufrufe.
    function runAutoAction() {
        if (!actionBar || !GameTable || !GameTable.myTurn)
            return
        if (actionBar.playingMode === 2) {            // Auto Check/Fold
            if (actionBar.canCheck) GameTable.call()
            else GameTable.fold()
        } else if (actionBar.playingMode === 1) {     // Auto Check/Call
            GameTable.call()
        }
    }

    function toggleLogOverlay() {
        if (!tableZone)
            return
        tableZone.showLog = !tableZone.showLog
        if (tableZone.showLog && !tableZone.wide)
            tableZone.showChat = false
    }

    function toggleChatOverlay() {
        if (!tableZone)
            return
        tableZone.showChat = !tableZone.showChat
        if (tableZone.showChat && !tableZone.wide)
            tableZone.showLog = false
    }

    function toggleFullscreenMode() {
        var win = gamePage.Window.window
        if (!win)
            return
        if (win.visibility === Window.FullScreen)
            win.visibility = Window.Windowed
        else
            win.visibility = Window.FullScreen
    }

    // ── Emoji-Reaktionen (Port aus dem Web-Client) ───────────────────────────
    // Gesendet wird über den Spiel-Chat mit der Web-Client-Konvention
    // "/emoji 🎉"; empfangene Reaktionen fängt GameHandler::appendChat ab und
    // meldet sie über reactionReceived. Eigene Reaktionen werden sofort lokal
    // abgespielt – das Server-Echo wird per Zeitfenster dedupliziert.
    property string _lastOwnReactionEmoji: ""
    property double _lastOwnReactionTime: 0

    // Emoji-Reaktionen können in den Einstellungen deaktiviert werden
    // (Config-Key "DisableEmojiReactions"). Da readConfigInt() nicht reaktiv
    // ist, wird der Wert beim Erscheinen der Seite neu eingelesen – die
    // Einstellungen liegen als eigene StackView-Seite darüber, beim Zurück-
    // kehren wird activated() ausgelöst.
    property bool emojiReactionsEnabled: true
    function refreshEmojiReactionsEnabled() {
        emojiReactionsEnabled = SettingsManager
            ? SettingsManager.readConfigInt("DisableEmojiReactions") === 0 : true
    }
    Component.onCompleted: refreshEmojiReactionsEnabled()
    StackView.onActivated: refreshEmojiReactionsEnabled()

    function sendReaction(emoji) {
        tableZone.showReactions = false
        _lastOwnReactionEmoji = emoji
        _lastOwnReactionTime = Date.now()
        playReactionAtSeat(0, emoji)
        if (GameTable)
            GameTable.sendChat("/emoji " + emoji)
    }

    function playReactionAtSeat(seatIdx, emoji) {
        var px, py
        if (seatIdx <= 0) {
            px = selfBox.x + selfBox.width / 2
            py = selfBox.y + selfBox.height / 2
                 - (selfBox.height * tableZone.boxScale) / 2 - 6
        } else {
            var slot = tableZone.slotForSeat(seatIdx)
            if (!slot) return
            px = tableZone.width * slot.x
            py = tableZone.height * slot.y + slot.nudge
                 - (tableZone.oppBaseHeight * tableZone.boxScale) / 2 - 6
        }
        // Die Animation steigt ~200 px auf – bei Sitzen nahe der Tisch-
        // Oberkante tiefer starten, sonst wird sie oben abgeschnitten.
        py = Math.max(py, 205)
        reactionFx.play(emoji, px, py)
    }

    // Alle belegten Sitznamen (für die Tab-Nick-Vervollständigung im Chat).
    function gameNickList() {
        var nicks = []
        if (typeof GameTable !== "undefined" && GameTable)
            for (var i = 0; i < GameTable.players.length; i++)
                if (GameTable.players[i].name !== "")
                    nicks.push(GameTable.players[i].name)
        return nicks
    }

    Connections {
        target: GameTable
        function onReactionReceived(playerName, emoji) {
            if (!gamePage.emojiReactionsEnabled)
                return
            console.log("[REACT] received from", playerName, "->", emoji)
            var players = GameTable.players
            var idx = -1
            for (var i = 0; i < players.length; i++)
                if (players[i].name !== "" && players[i].name === playerName) { idx = i; break }
            // Echo der eigenen, bereits lokal abgespielten Reaktion unterdrücken.
            if (idx <= 0
                && emoji === gamePage._lastOwnReactionEmoji
                && Date.now() - gamePage._lastOwnReactionTime < 3000)
                return
            if (idx < 0) {
                // Absender nicht am Tisch gefunden (z. B. Zuschauer):
                // über der Tischmitte abspielen.
                reactionFx.play(emoji, tableZone.width / 2, tableZone.communityCenterY - 40)
                return
            }
            gamePage.playReactionAtSeat(idx, emoji)
        }
    }

    // ── F-Tasten-Belegung der Gametable-Actions (1:1 aus dem Qt-Widgets-Client) ──
    // F1–F4 lösen Fold/Call-Check/Bet-Raise/All-In aus; die Reihenfolge dreht sich
    // bei AlternateFKeysUserActionMode (Einstellung "F-Tasten umkehren"):
    //   normal:    F1 Fold · F2 Call/Check · F3 Bet/Raise · F4 All-In
    //   alternate: F1 All-In · F2 Bet/Raise · F3 Call/Check · F4 Fold
    // F5 deckt die eigenen Karten auf, F6/F7/F8 schalten den Spielmodus.
    readonly property bool fKeysAlternate:
        (typeof SettingsManager !== "undefined" && SettingsManager)
        ? SettingsManager.readConfigInt("AlternateFKeysUserActionMode") !== 0 : false

    function fKeyAction(which) {
        if (actionBar)
            actionBar.clickAction(which)
    }

    Shortcut {
        sequence: "Alt+L"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.toggleLogOverlay()
    }
    Shortcut {
        sequence: "Alt+C"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.toggleChatOverlay()
    }
    Shortcut {
        sequence: "Alt+F"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.applyPlayingMode(2)
    }
    Shortcut {
        sequence: "Alt+M"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.applyPlayingMode(0)
    }
    Shortcut {
        sequence: "Alt+K"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.applyPlayingMode(1)
    }
    Shortcut {
        sequence: "F11"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.toggleFullscreenMode()
    }

    // ── Gametable-Actions: F-Tasten (siehe fKeysAlternate oben) ──
    Shortcut {
        sequence: "F1"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "allin" : "fold")
    }
    Shortcut {
        sequence: "F2"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "raise" : "call")
    }
    Shortcut {
        sequence: "F3"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "call" : "raise")
    }
    Shortcut {
        sequence: "F4"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "fold" : "allin")
    }
    Shortcut {
        sequence: "F5"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: if (GameTable && GameTable.canShowCards) GameTable.showMyCards()
    }
    Shortcut {
        sequence: "F6"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.applyPlayingMode(0)   // Manuell
    }
    Shortcut {
        sequence: "F7"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.applyPlayingMode(2)   // Auto Check/Fold
    }
    Shortcut {
        sequence: "F8"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible
        onActivated: gamePage.applyPlayingMode(1)   // Auto Check/Call
    }

    // gameBackground (Diamanten-Muster) entfernt – nicht mehr benötigt.


    // ── Tisch-Layout (Hoch- & Querformat) ─────────────────────────────────────
    // Einheitlicher Aufbau für alle Fenstergrößen:
    //   Status-Leiste → großer Tisch (alle Spieler überlagert) → Action-Leiste.
    // Die Spieler-Slots ordnen sich je nach Tisch-Seitenverhältnis (hoch/breit)
    // automatisch um – kein separates Desktop-Layout mehr.
    ColumnLayout {
        id: portraitLayout
        anchors.fill: parent
        spacing: 0

        // 1. Status-Leiste: Spielphase | Pott | Hand-Nummer
        Rectangle {
            Layout.fillWidth: true
            // Im landscapeCompact knapper (28 statt 40) — schafft ~12 px mehr
            // tableZone-Höhe für die Halsketten-Ellipse.
            Layout.preferredHeight: Config.Responsive.landscapeCompact ? 28 : 40
            color: Qt.rgba(0, 0, 0, 0.78)

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                spacing: 0

                // Links: Pot-Info (1:1 wie Widget-Client links neben den Community-Cards)
                // "Total" = aufgelaufener Pot (getPot), "Bets" = laufende Einsätze dieser Runde (getSets)
                Column {
                    spacing: 0
                    Row {
                        spacing: 4
                        Text {
                            text: qsTr("Total:")
                            color: "#9e9e9e"
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                            font.weight: Font.Medium
                        }
                        Text {
                            text: "$%1".arg(GameTable ? GameTable.pot : 0)
                            color: "#99D500"
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                            font.bold: true
                        }
                    }
                    Row {
                        spacing: 4
                        Text {
                            text: qsTr("Bets:")
                            color: "#9e9e9e"
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: Config.Responsive.landscapeCompact ? 10 : 11
                            font.weight: Font.Medium
                        }
                        Text {
                            text: "$%1".arg(GameTable ? (GameTable.totalPot - GameTable.pot) : 0)
                            color: "#7aa800"
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: Config.Responsive.landscapeCompact ? 10 : 11
                            font.weight: Font.Medium
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Rechts: Phase + Game-ID + Hand-Nummer (1:1 wie Widget-Client rechts neben den Community-Cards)
                Column {
                    spacing: 0
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: GameTable ? GameTable.phaseText : qsTr("Preflop")
                        color: "#FFFFFF"
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: Config.Responsive.landscapeCompact ? 11 : 13
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.5
                    }
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 8
                        Text {
                            text: qsTr("Game: %1").arg(GameTable ? GameTable.gameId : 0)
                            color: "#9e9e9e"
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: Config.Responsive.landscapeCompact ? 9 : 11
                            font.weight: Font.Medium
                        }
                        Text {
                            text: qsTr("Hand: %1").arg(GameTable ? GameTable.handNumber : 1)
                            color: "#9e9e9e"
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: Config.Responsive.landscapeCompact ? 9 : 11
                            font.weight: Font.Medium
                        }
                    }
                }
            }
        }

        // 2. Tischzone: grüne Tischgrafik füllt gesamten Platz, alle Spieler überlagert
        Item {
            id: tableZone
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Grüne Tischgrafik füllt die gesamte Zone. Unten am Bild liegt der
            // hölzerne Tischrand → Crop am unteren Rand ausrichten, damit dieser
            // auch im breiten Querformat sichtbar bleibt (im Hochformat ohnehin).
            // Querformat: reicht hinter der geschrumpften Action-Box bis zum
            // unteren Bildschirmrand, damit dort kein dunkler Streifen bleibt.
            Image {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: parent.height + (tableZone.wide ? actionBar.height : 0)
                source: "../resources/tableGreen.png"
                fillMode: Image.PreserveAspectCrop
                verticalAlignment: Image.AlignBottom
                smooth: true
            }

            // Anzahl der besetzten Sitze
            readonly property int seatCount: {
                if (typeof GameTable === "undefined" || !GameTable) return 1
                var c = 0
                for (var i = 0; i < GameTable.players.length; i++)
                    if (GameTable.players[i].name !== "") c++
                return Math.max(c, 1)
            }

            // Maximale Spielerzahl seit Spielbeginn – wird für boxScale genutzt,
            // damit ausscheidende Spieler die Box-Größe NICHT verändern.
            // Nur nach oben angepasst (wenn neue Spieler dazukommen) oder
            // zurückgesetzt (wenn das Spiel endet, d. h. seatCount auf 1 fällt).
            property int _peakSeatCount: 1
            property bool _gameWasActive: false
            onSeatCountChanged: {
                if (seatCount > 1) {
                    if (seatCount > _peakSeatCount) {
                        _peakSeatCount = seatCount
                    }
                    _gameWasActive = true
                } else if (_gameWasActive) {
                    // Spiel beendet: Peak zurücksetzen, damit das nächste Spiel
                    // mit seiner eigenen Spielerzahl skaliert.
                    _peakSeatCount = 1
                    _gameWasActive = false
                }
            }

            // Breiter Tisch (Querformat) vs. hoher Tisch (Hochformat) – die
            // Spieler-Slots ordnen sich je nach Seitenverhältnis automatisch um.
            readonly property bool wide: width >= height

            // Gegner- und Self-Box wachsen im Querformat gemeinsam. Referenz ist
            // nicht nur die absolute Breite, sondern wie viel zusätzliche Breite
            // bei gleicher Höhe entsteht: dadurch reagieren die Boxen sichtbar
            // schneller beim Ziehen von Portrait nach Wide.
            // Basis-Maße: in Portrait ist die Gegner-Box NIEDRIGER (71 statt
            // 84), weil sonst der quadratische Avatar (= topRow.height) zu
            // breit wird und die Karten horizontal aus der cardsLane raus-
            // hängen. Mit 71 wird topRow ≈ 45 → 2 Karten + Avatar passen
            // bequem nebeneinander. In Landscape (84) ist der 2-zeilige
            // Footer 44 px, topRow = 40 → Avatar/Karten sichtbar größer.
            readonly property int oppBaseHeight: wide ? 84 : 71
            // Breite dynamisch: 2×hMargin + AvatarCardRow.implicitWidth.
            // AvatarCardRow: avatarH + gap(4) + 2·cardW + cardSpacing(4)
            //   Landscape: topRow=40, cardW=29 → 2×4 + 40 + 4 + 2×29 + 4 = 114
            //   Portrait : topRow=43, cardW=31 → 2×4 + 43 + 4 + 2×31 + 4 = 121
            readonly property int oppBaseWidth: {
                var rowH = oppBaseHeight - (wide ? 44 : 28)
                var cw   = Math.round(rowH * 120 / 168)
                return 2 * 4 + rowH + 4 + 2 * cw + 4
            }
            // selfBaseHeight im Wide auf 84 (= oppBaseHeight): die Self-Box muss
            // IMMER mindestens so groß sein wie die Gegnerboxen (beide skalieren
            // mit demselben boxScale → base gleich halten).
            // cardsArea.height = 84−12−32 = 40, selfBaseWidth = 114 = oppBaseWidth 114.
            readonly property int selfBaseHeight: wide ? 84 : 71
            // Self-Box-Breite dynamisch: identische Abstände wie Gegnerboxen.
            //   Compact  : cardsH=46, cardW=33, avW=46 → 2×4 + 46 + 4 + 2×33 + 4 = 128
            //   Landscape: cardsH=40, cardW=29, avW=40 → 2×4 + 40 + 4 + 2×29 + 4 = 114
            //   Portrait : cardsH=41, cardW=29, avW=41 → 2×4 + 41 + 4 + 2×29 + 4 = 115
            readonly property int selfBaseWidth: {
                var cH  = selfBaseHeight - 12 - (Config.Responsive.landscape ? 32 : 18)
                var cW  = Math.round(cH * 120 / 168)
                var avS = Math.min(cH, 60)
                return 2 * 4 + avS + 4 + cW * 2 + 4
            }
            readonly property real opponentGapBase: 10
            readonly property real opponentHorizontalGapBase: opponentGapBase * 2.8
            readonly property real selfGapBase: opponentGapBase * 2
            // Vertikales Sicherheits-Padding zwischen Bottom-Seats und Self-Box.
            readonly property real selfBadgeGapBase: 8
            readonly property real sideBadgeGapBase: 48
            readonly property int landscapeRowCount: seatCount <= 4 ? 1
                : seatCount <= 6 ? 2
                : seatCount <= 8 ? 3
                : 4
            readonly property real boxScale: {
                if (width <= 0 || height <= 0) return 1.0
                // Für die Skalierungsberechnung den Peak-Wert nutzen, damit
                // ausscheidende Spieler die Box-Größe nicht verändern.
                var oppCnt = _peakSeatCount - 1
                var s

                // Strategie: Box-Skala = MAXIMUM, das alle geometrischen
                // Constraints erfüllt. Dadurch füllen die Boxen den
                // verfügbaren Tisch optimal aus – breite Fenster bekommen
                // große Boxen (vertikale Reihenpassung als Obergrenze),
                // schmale Fenster bekommen automatisch kleinere Boxen
                // (Slot-Sichtbarkeit als Obergrenze). Kein künstlicher
                // ref/700-Boden mehr, der breite Fenster auf der kleineren
                // Dimension klein hielt.
                if (wide) {
                    // Landscape-Cap per Bisektion: maximaler boxScale, für den
                    // ALLE benachbarten Ellipsen-Sitzpaare entweder horizontal
                    // ODER vertikal voneinander getrennt bleiben.
                    //
                    // WICHTIG: jeder Probepunkt rechnet `radiusX`/`radiusY`
                    // mit den exakt gleichen s-abhängigen Formeln wie
                    // `buildLandscapeSlots()`. Frühere statische Schätzwerte
                    // (`sideMarginBase` ohne s-Faktor, `approxRy = 0.30`)
                    // unterschätzten den BL/BR-Pair-Bedarf bei großen s →
                    // bei 9–10 Spielern und sehr breitem Fenster überlappten
                    // die Boxen, obwohl die alte Cap-Formel noch grünes
                    // Licht gab.
                    var gap = 12
                    var selfWeightCap = 0.5
                    var stepDeg = oppCnt >= 1 ? 360 / (oppCnt + selfWeightCap) : 360
                    var firstAngle = 90 + (selfWeightCap * stepDeg + stepDeg) / 2

                    function feasibleAt(sTest) {
                        if (oppCnt < 2) return true
                        var sideMargin = Math.max(18, width * 0.025) + sideBadgeGapBase * sTest
                        var visualW = oppBaseWidth * sTest
                        var visualH = oppBaseHeight * sTest
                        var selfVisualH = selfBaseHeight * sTest
                        var gapY = Math.max(8, opponentGapBase * sTest)
                        // Im landscapeCompact ziehen wir die untere Ellipsen-
                        // Hälfte näher an die Self-Box: das verschafft den
                        // Seiten-Paaren mehr vertikalen Spielraum (sonst kleben
                        // Player 7↔8 / 2↔3 visuell zusammen). selfBadgeGapBase
                        // bleibt als Minimum, damit Bet-/Action-Badges
                        // unterhalb der Bottom-Reihe nicht ins Self-Avatar
                        // hineinragen.
                        var selfGapY = Config.Responsive.landscapeCompact
                            ? Math.max(8, selfBadgeGapBase * sTest * 0.5)
                            : selfBadgeGapBase * sTest
                        var radiusXpix = Math.max(0.22 * width,
                                                   0.5 * width - sideMargin - visualW / 2)
                        // Im Compact-Landscape ragt die Bet-Badge von Player 5
                        // (betSide="bottom") 39 Basis-Pixel unterhalb seiner Box heraus.
                        // Diesen Bereich aus dem verfügbaren Ellipsen-Radius herausrechnen,
                        // damit Community Cards nie durch eine Spielerbox verdeckt werden.
                        var topBadgeExt = Config.Responsive.landscapeCompact ? 39 : 0
                        var topYpix = (Config.Responsive.landscapeCompact ? 0 : 4)
                                      + visualH / 2 + topBadgeExt * sTest
                        var bottomYpix = height - 4 - selfVisualH - selfGapY - visualH / 2
                        // Wie buildLandscapeSlots(): radiusY = nur (bottomY-topY)/2.
                        // Kein Max mit (visualH + gapY*2.2): das würde den
                        // Vertikalradius künstlich aufblasen → Bisection würde
                        // grünes Licht geben, das Layout zeichnet aber Boxen
                        // über die obere tableZone-Kante hinaus.
                        var radiusYpix = (bottomYpix - topYpix) / 2
                        if (radiusYpix <= 0 || radiusXpix <= 0) return false
                        // Repliziert buildLandscapeSlots().point() vollständig –
                        // lowerSquash + topCosSquash + sideGravity/yShift.
                        // Ohne alle drei Korrekturen überschätzt die Bisection
                        // den Paarabstand und lässt ein zu großes boxScale durch.
                        var lowerSquashCap   = Config.Responsive.landscapeCompact ? 0.2 : 1.0
                        var topCosSquash     = 1.4
                        var sideGravity      = 0.25
                        var gravityUpperOnly = Config.Responsive.landscapeCompact
                        var lowerGravity     = Config.Responsive.landscapeCompact ? 0.0 : 0.15
                        // Spiegelt die compact-Absenkung unterer Seiten-Sitze aus
                        // buildLandscapeSlots().point() – sonst unterschätzt die
                        // Bisection den vertikalen Paarabstand und cappt zu früh.
                        var centerYpix    = (topYpix + bottomYpix) / 2
                        var maxBottomYpix = (height - 4 - selfVisualH) + selfVisualH * 0.35 - visualH / 2
                        var vMaxLowerP    = radiusYpix > 0 ? (maxBottomYpix - centerYpix) / radiusYpix : 1.0
                        var selfClearXpix = selfBaseWidth * sTest / 2 + visualW / 2 + 12
                        function slotVec(deg) {
                            var rad  = deg * Math.PI / 180
                            var sinV = Math.sin(rad)
                            var cosV = Math.cos(rad)
                            var sinOrig = sinV
                            if (sinV > 0 && lowerSquashCap !== 1.0)
                                sinV = Math.pow(sinV, lowerSquashCap)
                            if (sinV <= 0 && Math.abs(cosV) > 1e-9)
                                cosV = (cosV < 0 ? -1 : 1) * Math.pow(Math.abs(cosV), topCosSquash)
                            var vFactor = sinV
                                        + ((!gravityUpperOnly || sinV <= 0) ? sideGravity * Math.abs(cosV) : 0)
                                        + (sinV > 0 ? lowerGravity * sinV : 0)
                            if (vFactor > 1.0) vFactor = 1.0
                            if (Config.Responsive.landscapeCompact && sinV > 0
                                && Math.abs(radiusXpix * cosV) > selfClearXpix
                                && vMaxLowerP > vFactor)
                                vFactor = vFactor + (vMaxLowerP - vFactor) * sinOrig
                            return [cosV, vFactor]
                        }
                        // Bet-Badges auf beiden Seiten einrechnen (chip+text+Abstand).
                        // Ohne diesen Aufschlag erlaubt die Bisection zu große scales
                        // und die Einsatz-Anzeige reicht in die Nachbarbox hinein.
                        var xNeeded = sTest * (oppBaseWidth + sideBadgeGapBase) + gap
                        var yNeeded = sTest * oppBaseHeight + gap
                        for (var iPair = 1; iPair < oppCnt; iPair++) {
                            var d1 = firstAngle + (iPair - 1) * stepDeg
                            var d2 = d1 + stepDeg
                            var v1 = slotVec(d1)
                            var v2 = slotVec(d2)
                            var dcos = Math.abs(v1[0] - v2[0])
                            var dsin = Math.abs(v1[1] - v2[1])
                            if (dcos * radiusXpix < xNeeded
                                && dsin * radiusYpix < yNeeded)
                                return false
                        }
                        return true
                    }

                    // Gemeinsames Limit für Gegnerboxen, Self-Box und Community-Badges:
                    // 1.4 verhindert zu große Schrift und Bet-Überlappungen bei
                    // Vollbild/maximiert; compact bleibt bei 1.9 (breiter, flacher).
                    var lo = 0.55, hi = Config.Responsive.landscapeCompact ? 1.7 : 1.4
                    if (oppCnt < 2) {
                        s = hi
                    } else if (!feasibleAt(lo)) {
                        s = lo
                    } else {
                        for (var iter = 0; iter < 14; iter++) {
                            var mid = (lo + hi) / 2
                            if (feasibleAt(mid)) lo = mid
                            else hi = mid
                        }
                        s = lo
                    }
                } else {
                    // Portrait-Cap per Bisektion (analog Wide-Screen). Die
                    // Slot-Positionen sind in Portrait statisch (slotPosPortrait),
                    // daher gehen sie hier nur als Konstanten in die feasibility-
                    // Probe ein. Constraints:
                    //   • Wand links/rechts:  Seitenspalten x=0.15/0.85
                    //   • Wand oben:          TC bei y=0.075
                    //   • Wand unten:         Bottom-Reihe (L_bottom/R_bottom
                    //                         bei y=0.785) darf die Self-Box
                    //                         (bottomMargin=20) nicht berühren.
                    //   • Paar-Trennung:      benachbarte Sitze in
                    //                         slotSeqPortrait[oppCnt] müssen
                    //                         entweder horizontal ODER vertikal
                    //                         genug Abstand zueinander haben.
                    // Der frühere statische Cap konnte den Self-Box-Wandabstand
                    // nicht modellieren; in breitem Portrait überlappten Bottom-
                    // Reihe und Self-Box potentiell.
                    var gapP = 8
                    var seqP = slotSeqPortrait[oppCnt] || []
                    var posP = slotPosPortrait

                    function feasibleAtP(sTest) {
                        if (sTest <= 0) return false
                        var visualW = oppBaseWidth * sTest
                        var visualH = oppBaseHeight * sTest
                        var selfVisualH = selfBaseHeight * sTest

                        // Wand-Checks
                        if (visualW > 2 * (0.15 * width - 4)) return false
                        if (visualH > 2 * (0.075 * height - 4)) return false
                        // Self-Box vs. Bottom-Reihe (L_bottom/R_bottom bei oppCnt>=8).
                        // seatNudge=+14 für diese Slots wird berücksichtigt:
                        //   self_top    = height - 4 - selfVisualH  (scale-kompensierbares bottomMargin)
                        //   bottom_kant = 0.785*height + 14 + visualH/2
                        //   Abstand     = 0.215*height - 18 - selfVisualH - visualH/2
                        //   Constraint  = Abstand >= gapP  →  0.215*H - 26 - ... >= 0
                        if (oppCnt >= 8 && 0.215 * height - 26 - selfVisualH - visualH / 2 < gapP)
                            return false

                        // Paar-Trennung
                        if (seqP.length < 2) return true
                        var xNeeded = sTest * oppBaseWidth + gapP
                        var yNeeded = sTest * oppBaseHeight + gapP
                        for (var i = 0; i < seqP.length - 1; i++) {
                            var a = posP[seqP[i]]
                            var b = posP[seqP[i + 1]]
                            if (!a || !b) continue
                            var dxPix = Math.abs(a[0] - b[0]) * width
                            var dyPix = Math.abs(a[1] - b[1]) * height
                            if (dxPix < xNeeded && dyPix < yNeeded)
                                return false
                        }
                        return true
                    }

                    var loP = 0.55, hiP = 1.85
                    if (!feasibleAtP(loP)) {
                        s = loP
                    } else {
                        for (var iterP = 0; iterP < 14; iterP++) {
                            var midP = (loP + hiP) / 2
                            if (feasibleAtP(midP)) loP = midP
                            else hiP = midP
                        }
                        s = loP
                    }
                }

                // Lesbarkeits-Boden – Schrift/Karten skalieren mit, dürfen aber
                // nicht beliebig klein werden.
                return Math.max(0.55, s)
            }
            readonly property real oppScale: boxScale
            // Community-Karten-Skala:
            //   – Wide-Screen: 0.95·boxScale (gibt Sicherheits-Padding zu den
            //     Box-Badges).
            //   – Portrait: LANGSAMERES Wachstum als die opp-Boxen (Faktor 0.7)
            //     plus Floor 0.7 — die Community-Reihe ist bei kleinem Portrait
            //     also relativ größer und wächst bei breiteren Fenstern nur
            //     gedämpft mit. Adaptiver Cap stellt sicher, dass die Karten
            //     die Seitenspalten nicht horizontal berühren.
            readonly property real communityScale: {
                if (wide) return boxScale * 0.95
                var target = Math.max(0.7, boxScale * 0.7)
                var sideColRightEdge = 0.15 * width + oppBaseWidth * boxScale / 2
                var maxCommunityHalfW = width / 2 - sideColRightEdge - 4
                var maxCommunityW = Math.max(0, maxCommunityHalfW * 2)
                var maxScale = maxCommunityW / 250
                return Math.max(0.55, Math.min(target, maxScale))
            }

            // ── Lupe: Zoom + Pan der Gegnerzone (compact-only) ──────────────────
            property bool  zoomActive: false
            readonly property real zoomFactor: 2.0
            property real  _zoomPanX: 0
            property real  _zoomPanY: 0
            // Schwenk-Animation beim Loslassen/Zurücksetzen; deaktiviert während
            // des aktiven Drags, damit der Finger ohne Verzögerung verfolgt wird.
            Behavior on _zoomPanX {
                enabled: !zoomPanner.active
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }
            Behavior on _zoomPanY {
                enabled: !zoomPanner.active
                NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
            }

            // Feste Slot-Positionen (Mittelpunkt der Box als Anteil 0..1 der Zone).
            // Hochformat: 3 oben, Rest an den Seiten nach unten.
            // Vertikale Anordnung mit identischen Innen-Gaps oben/unten
            // (TL↔L_upper = L_lower↔L_bottom = 0.135). Die Mitte zwischen
            // L_upper und L_lower (0.305) bleibt absichtlich größer und ist
            // reserviert für den Community-Karten-Bereich. So sind Player
            // 1↔2 und Player 3↔4 (bzw. 8↔9 und 6↔7) jeweils identisch
            // beabstandet (User-Wunsch Symmetrie).
            // Spalten-x bei 0.14 (statt 0.15) damit Boxen in mittleren
            // Portrait-Größen etwas weiter außen sitzen.
            readonly property var slotPosPortrait: ({
                "L_bottom": [0.15, 0.785],
                "L_lower":  [0.15, 0.65],
                "L_upper":  [0.15, 0.345],
                "TL":       [0.15, 0.21],
                "TC":       [0.50, 0.075],
                "TR":       [0.85, 0.21],
                "R_upper":  [0.85, 0.345],
                "R_lower":  [0.85, 0.65],
                "R_bottom": [0.85, 0.785]
            })
            // Querformat: Slot-Abstände werden aus visueller Boxgröße,
            // Spieleranzahl und Self-Abstand berechnet statt als offene Ellipse
            // fest verdrahtet. Horizontaler und vertikaler Gegner-Abstand werden getrennt begrenzt;
            // zur Self-Box bleibt bewusst mehr Luft.
            readonly property var slotPosLandscape: buildLandscapeSlots()
            readonly property var slotPos: wide ? slotPosLandscape : slotPosPortrait

            function buildLandscapeSlots() {
                var s = boxScale
                var visualW = oppBaseWidth * s
                var visualH = oppBaseHeight * s
                var selfVisualH = selfBaseHeight * s
                var sideMargin = Math.max(18, width * 0.025) + sideBadgeGapBase * s
                var wantedGapY = opponentGapBase * s
                var gapY = Math.max(8, wantedGapY)
                // Im landscapeCompact: halbierte selfGapY (s. Bisection-Comment).
                var selfGapY = Config.Responsive.landscapeCompact
                    ? Math.max(8, selfBadgeGapBase * s * 0.5)
                    : selfBadgeGapBase * s
                var sideX = (sideMargin + visualW / 2) / Math.max(width, 1)
                // radiusX so groß wie möglich (Seiten-Sitze landen am Rand).
                // Top-Trio passt durch den boxScale-Cap (siehe boxScale oben)
                // automatisch in dieses Bogenstück, ohne dass wir radiusX hier
                // weiter aufblasen müssen (sonst rutschen Seiten-Sitze raus).
                var radiusX = Math.max(0.22, 0.5 - sideX)
                // Top- und Bottom-Rand bewusst klein – die offene Ellipse
                // soll möglichst viel vertikalen Platz beanspruchen, damit
                // bei mittlerer Skalierung Player 2↔3 (L↔TLo) genug Luft
                // bekommen.
                // Compact: oberste Box bündig an die Tisch-Oberkante (0 statt 4) –
                // schafft Luft zwischen ihrem Bet-Badge und dem Pot-Badge.
                var topY = ((Config.Responsive.landscapeCompact ? 0 : 4) + visualH / 2) / Math.max(height, 1)
                var selfTop = height - 4 - selfVisualH
                var bottomY = (selfTop - selfGapY - visualH / 2) / Math.max(height, 1)
                var centerY = (topY + bottomY) / 2
                // radiusY STRIKT auf das verfügbare Bahn-Stück begrenzen:
                // sonst kann das Top-Slot-Center bei `centerY - radiusY` unter
                // den `topY`-Wert rutschen → Box-Visual wird über die tableZone
                // hinaus gezeichnet und überlappt die Status-Bar. Falls der
                // resultierende radiusY zu klein für die nötige Paartrennung
                // ist, sorgt der boxScale-Cap (Bisection) dafür, dass die
                // Boxen kleiner werden.
                var radiusY = (bottomY - topY) / 2

                // lowerSquash (compact only): sin>0-Spieler via sin^0.3 nach
                // bottomY gedrückt.
                //
                // sideGravity: Zusatz-Y proportional zu |cos| → Seitenspieler
                // (|cos|→1) nach unten, TC (cos=0) bleibt. In compact-Mode nur
                // für die obere Hälfte (sinV≤0), da lowerSquash die untere Hälfte
                // bereits stark pusht und ein doppelter Push bottomY übersteigen
                // würde.
                //
                // topCosSquash: obere Hälfte (sinV≤0) nutzt |cos|^topCosSquash →
                // TL/TR (cos≈±0.62) horizontal näher an TC, reine Seitenspieler
                // (cos≈±0.97) kaum verändert.
                var lowerSquash        = Config.Responsive.landscapeCompact ? 0.2  : 1.0
                var sideGravity        = 0.25
                var topCosSquash       = 1.4
                var gravityUpperOnly   = Config.Responsive.landscapeCompact
                // Untere Sitze (sinV>0, v. a. die Bottom-Boxen bem2/danielv) werden
                // im normalen Landscape zusätzlich proportional zu sin Richtung
                // bottomY gezogen – sonst sitzen sie zu hoch und zu nah an ihren
                // oberen Nachbarn. Begrenzung auf bottomY (vFactor ≤ 1) hält den
                // selfGapY-Abstand zur Self-Box ein. Im compact-Mode übernimmt
                // das bereits lowerSquash.
                var lowerGravity       = Config.Responsive.landscapeCompact ? 0.0 : 0.15
                // Compact: Die Ecken links/rechts neben der Self-Box sind frei.
                // Untere Seiten-Sitze, die horizontal an der Self-Box vorbeigehen,
                // dürfen deshalb etwas unter bottomY absinken – das entzerrt die
                // Seiten-Paare (z. B. Player 2↔3 / 7↔8) vertikal. Maximal bis die
                // Box-Unterkante 35 % in die Self-Box-Höhe hineinragt: tiefer
                // (bis zur Self-Unterkante) zerstört die Ellipsen-Optik, weil die
                // Gegner dann auf/unter Self-Niveau liegen.
                var maxBottomY = (selfTop + selfVisualH * 0.35 - visualH / 2) / Math.max(height, 1)
                var vMaxLower  = radiusY > 0 ? (maxBottomY - centerY) / radiusY : 1.0
                var selfClearX = (selfBaseWidth * s / 2 + visualW / 2 + 12) / Math.max(width, 1)
                function point(degrees) {
                    var radians = degrees * Math.PI / 180
                    var sinV = Math.sin(radians)
                    var cosV = Math.cos(radians)
                    var sinOrig = sinV
                    if (sinV > 0 && lowerSquash !== 1.0)
                        sinV = Math.pow(sinV, lowerSquash)
                    if (sinV <= 0 && cosV !== 0)
                        cosV = (cosV < 0 ? -1 : 1) * Math.pow(Math.abs(cosV), topCosSquash)
                    var vFactor = sinV
                                + ((!gravityUpperOnly || sinV <= 0) ? sideGravity * Math.abs(cosV) : 0)
                                + (sinV > 0 ? lowerGravity * sinV : 0)
                    if (vFactor > 1.0) vFactor = 1.0   // nie unter bottomY (Self-Box)
                    // Graduell Richtung vMaxLower absenken, gewichtet mit dem
                    // ORIGINAL-sin: die untersten Sitze (BL/BR, sin≈0.88) sinken
                    // fast voll ab, die darüber (sin≈0.40) nur teilweise – ein
                    // einheitliches vMaxLower setzte alle auf dieselbe Höhe
                    // (flache Linie statt Ellipsenbogen).
                    if (Config.Responsive.landscapeCompact && sinV > 0
                        && Math.abs(radiusX * cosV) > selfClearX
                        && vMaxLower > vFactor)
                        vFactor = vFactor + (vMaxLower - vFactor) * sinOrig
                    return [0.5 + radiusX * cosV, centerY + radiusY * vFactor]
                }

                // Kreis öffnet sich nach oben:
                //   – TL/TR bei 230°/310° (statt 240°/300°) → mehr horizontaler
                //     Abstand zur TC, TL/TR rücken in y-Richtung etwas tiefer.
                //   – TLo/TRo bei 200°/340° (statt 205°/335°) → fast vertikal
                //     mit L/R, dadurch mehr y-Abstand zur TL/TR.
                // BL/BR auf 120°/60° (statt 125°/55°): sin steigt 0.819→0.866
                // → Bottom-Sitze rücken ca. 5 % von radiusY weiter Richtung
                // Self-Box, der vertikale Leerraum unter ihnen schrumpft.
                // Halsketten-Modell: Self ist eine "größere Perle" am unteren
                // Bodenpunkt der Ellipse mit angularer Gewichtung relativ zu
                // einer Gegner-"Perle" (selfWeight). Die N Gegner verteilen
                // sich GLEICHMÄSSIG auf den restlichen Bogen.
                //
                // selfWeight steuert, wie viel angulare Bogenlänge die Self
                // beansprucht. Kleiner = Gegner rücken näher an die Self.
                // 0.5 = halbe Bogenlänge einer Gegnerbox – damit die
                // Halskette „eng" sitzt, ohne dass die BL/BR-Boxen den
                // Self-Box-Rand horizontal berühren.
                //
                // Disconnectet ein Spieler, ändert sich N → automatische,
                // saubere Re-Verteilung über die unten generierten Winkel.
                var opps = Math.max(1, seatCount - 1)
                var selfWeight = 0.5
                var dOpp = 360 / (opps + selfWeight)
                var dSelf = selfWeight * dOpp
                var firstOppAngle = 90 + (dSelf + dOpp) / 2
                var slots = {}
                for (var i = 1; i <= opps; i++) {
                    slots["opp" + i] = point(firstOppAngle + (i - 1) * dOpp)
                }
                return slots
            }

            // Slot-Reihenfolge je nach Gegnerzahl M – symmetrisch links/rechts verteilt,
            // damit unabhängig von der Spielerzahl Kreis-Symmetrie entsteht.
            readonly property var slotSeqPortrait: ({
                1: ["TC"],
                2: ["TL", "TR"],
                3: ["TL", "TC", "TR"],
                4: ["L_upper", "TL", "TR", "R_upper"],
                5: ["L_upper", "TL", "TC", "TR", "R_upper"],
                6: ["L_lower", "L_upper", "TL", "TR", "R_upper", "R_lower"],
                7: ["L_lower", "L_upper", "TL", "TC", "TR", "R_upper", "R_lower"],
                8: ["L_bottom", "L_lower", "L_upper", "TL", "TR", "R_upper", "R_lower", "R_bottom"],
                9: ["L_bottom", "L_lower", "L_upper", "TL", "TC", "TR", "R_upper", "R_lower", "R_bottom"]
            })
            // Slot-Reihenfolge für Wide-Screen: dynamische Namen passen zur
            // dynamischen Slot-Generierung in buildLandscapeSlots() –
            // Sitz N(==i) erhält "opp" + i. Symmetrische Verteilung erfolgt
            // automatisch über die in buildLandscapeSlots() berechneten Winkel.
            readonly property var slotSeqLandscape: {
                var dict = {}
                for (var n = 1; n <= 9; n++) {
                    var seq = []
                    for (var i = 1; i <= n; i++) seq.push("opp" + i)
                    dict[n] = seq
                }
                return dict
            }
            readonly property var slotSeq: wide ? slotSeqLandscape : slotSeqPortrait

            // zoomContent.transformOrigin == TopLeft, x=(1−sc)·w/2 + panX
            // → Bildschirmmitte auf Content-Punkt (cx,cy): panX = w − cx·sc
            function _panToPoint(cx, cy) {
                var sc = zoomFactor
                var maxX = (sc - 1) * width  / 2
                var maxY = (sc - 1) * height / 2
                _zoomPanX = Math.max(-maxX, Math.min(maxX, width  - cx * sc))
                _zoomPanY = Math.max(-maxY, Math.min(maxY, height - cy * sc))
            }
            function _panToSeat(seatIdx) {
                var slot = slotForSeat(seatIdx)
                if (!slot) return
                _panToPoint(width * slot.x, height * slot.y + slot.nudge)
            }

            function slotForSeat(seatIdx) {
                if (!GameTable || seatIdx <= 0) return null
                var players = GameTable.players
                var oppOrder = 0
                for (var i = 1; i <= seatIdx && i < players.length; i++)
                    if (players[i].name !== "") oppOrder++
                if (oppOrder < 1) return null
                var seatCount = 0
                for (var j = 0; j < players.length; j++)
                    if (players[j].name !== "") seatCount++
                var seq = slotSeq[seatCount - 1]
                if (!seq || oppOrder > seq.length) return null
                var name = seq[oppOrder - 1]
                var pos = slotPos[name]
                if (!pos) return null
                var nudge = wide ? 0
                    : (name === "L_lower" || name === "L_bottom"
                       || name === "R_lower" || name === "R_bottom") ? 14
                    : (name === "L_upper" || name === "TL"
                       || name === "R_upper" || name === "TR") ? -4
                    : 0
                return { x: pos[0], y: pos[1], nudge: nudge }
            }

            readonly property real landscapeEllipseCenterY: {
                // GEOMETRISCHE Mitte der Ellipse in Pixeln – exakt dieselbe
                // Berechnung wie in buildLandscapeSlots() (topY/bottomY,
                // centerY = (topY+bottomY)/2). Hier ist der visuelle Tisch-
                // mittelpunkt und damit der natürliche Ort für die Community-
                // Karten der Halsketten-Anordnung.
                var s = boxScale
                var visualH = oppBaseHeight * s
                var selfVisualH = selfBaseHeight * s
                var gapY = Math.max(8, opponentGapBase * s)
                var selfGapY = Config.Responsive.landscapeCompact
                    ? Math.max(8, selfBadgeGapBase * s * 0.5)
                    : selfBadgeGapBase * s
                var topY = 4 + visualH / 2
                var selfTop = height - 4 - selfVisualH
                var bottomY = selfTop - selfGapY - visualH / 2
                return (topY + bottomY) / 2
            }
            readonly property real topOpponentBottomY: {
                var oppCount = seatCount - 1
                var seq = slotSeq[oppCount] || []
                var topCenter = 0.13
                for (var i = 0; i < seq.length; ++i) {
                    var p = slotPos[seq[i]]
                    if (p && p[1] < topCenter) topCenter = p[1]
                }
                return topCenter * height + oppBaseHeight * oppScale / 2
            }
            readonly property real selfVisualTopY:
                selfBox.y + selfBox.height / 2 - selfBox.height * boxScale / 2
            // Community-Karten-Position:
            //   – Wide-Screen: GEOMETRISCHES Zentrum der Halsketten-Ellipse
            //     (landscapeEllipseCenterY). Die Karten sitzen exakt in der
            //     Mitte des ovalen Tisches, umringt von den Gegner-„Perlen".
            //   – Portrait: weiterhin Mittelpunkt zwischen oberster Gegner-
            //     Box-Unterkante und Self-Box-Oberkante – passt zur statischen
            //     3-Säulen-Anordnung.
            // Community-Karten Y-Achse:
            //   – Wide regulär:        geometrische Ellipsen-Mitte.
            //   – landscapeCompact:    zwischen Unterkante der obersten Gegner-
            //                           Boxen und Oberkante der Self-Box; die
            //                           untere Ellipsen-Hälfte wurde näher an
            //                           die Self-Box gezogen, ihr Schwerpunkt
            //                           liegt entsprechend weiter unten — die
            //                           Karten würden sonst mitwandern. Mit
            //                           dieser Formel sitzen sie wieder optisch
            //                           in der Tisch-Mitte.
            //   – Portrait:            ebenfalls (topOpponentBottomY + selfVisualTopY)/2.
            readonly property real communityCenterY:
                wide && !Config.Responsive.landscapeCompact
                    ? landscapeEllipseCenterY
                    : (topOpponentBottomY + selfVisualTopY) / 2

            // ── Zoom-Layer: Gegner + Community – skalierbar + schwenkbar ─────────
            // actionBar und gameBackground liegen AUSSERHALB und bleiben fix.
            // selfBox ist jetzt INNERHALB des zoom-fähigen Layers.
            Item {
                id: zoomLayer
                anchors.fill: parent
                // Nur clippen wenn Zoom aktiv – ohne Zoom sollen Badge-Overlays
                // (z.B. Winner-Badge bei Player 5 oben) über den tableZone-Rand
                // hinausragen können. tableZone ist nach der StatusBar im Dokument,
                // daher rendert der Overflow auf ihr.
                clip: tableZone.zoomActive

                Item {
                    id: zoomContent
                    // Volle tableZone-Höhe (nicht zoomLayer.height), damit alle
                    // Slot-Positionen (tableZone.height * slot[1]) und der
                    // communityArea-verticalCenter-Anker korrekt bleiben.
                    width:  tableZone.width
                    height: tableZone.height
                    transformOrigin: Item.TopLeft
                    scale: tableZone.zoomActive ? tableZone.zoomFactor : 1.0
                    // Zentriert den Zoom-Pivot auf die Mitte der sichtbaren
                    // Gegnerzone (zoomLayer.height/2); der (1−scale)-Term
                    // kompensiert automatisch beim Rauszoomen.
                    x: (1.0 - scale) * (zoomLayer.width  / 2)
                       + (tableZone.zoomActive ? tableZone._zoomPanX : 0)
                    y: (1.0 - scale) * (zoomLayer.height / 2)
                       + (tableZone.zoomActive ? tableZone._zoomPanY : 0)

                    Behavior on scale {
                        NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
                    }

            // ── Gemeinschaftskarten + Pot – im oberen Tischbereich ───────────────
            Item {
                id: communityArea
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                // Portrait: mittig zwischen oberen (0.345·H = L_upper/R_upper)
                // und unteren Seiten-Boxen (0.65·H = L_lower/R_lower); die
                // per seatNudge gespreizten unteren Boxen verschieben den
                // Mittelpunkt um (14−4)/2 = 5px nach unten:
                //   midpoint = (0.345+0.65)/2 = 0.4975 → offset = -0.0025·H + 5
                // Widescreen: im Mittelpunkt der Halsketten-Ellipse, auf der
                // die Gegnerboxen um die Community herum liegen.
                anchors.verticalCenterOffset: tableZone.wide
                    ? tableZone.communityCenterY - tableZone.height / 2
                    : -tableZone.height * 0.0025 + 5
                // Größe = nur die Kartenreihe; das Winning-Hand-Badge liegt als
                // Overlay darunter und zählt NICHT zur Größe → die Karten bleiben
                // zentriert und rutschen nicht nach oben, wenn das Badge erscheint.
                width: cardRow.width
                height: cardRow.height
                z: 0
                // Skaliert dezenter als die Gegner-Boxen (Faktor 0.85), damit
                // bei großen Fenstern Box-Badges nicht in den Karten-Bereich
                // hineinragen.  Skalierung um die Mitte, damit die Position
                // erhalten bleibt.
                transformOrigin: Item.Center
                scale: tableZone.communityScale

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
                    anchors.bottomMargin: tableZone.wide ? 8 : 6
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

            // Gewinner-Hand (z.B. "Full House") – nur während des Showdowns.
            // Als eigenes Top-Level-Element (NICHT in communityArea), damit es
            // unabhängig von deren z/Scale immer ÜBER den Spielerboxen liegt –
            // in Hoch- UND Querformat. Positioniert knapp unter den (skalierten)
            // Community Cards.
            Rectangle {
                id: winHandBadge
                z: 50   // über Boxen (z:1), unter den Overlays (z:150)
                visible: (typeof GameTable !== "undefined" && GameTable)
                         ? GameTable.winningHandText !== "" : false
                anchors.horizontalCenter: parent.horizontalCenter
                // Abstand zur Kartenreihe identisch zum Pot-Badge oben
                // (Portrait 6, Querformat 8 – jeweils · oppScale). Setzt direkt am
                // (skalierten) Mittelpunkt der communityArea an, folgt damit deren
                // Zentrierung in Hoch- UND Querformat.
                y: communityArea.y + communityArea.height / 2
                   + (communityArea.height * communityArea.scale) / 2
                   + (tableZone.wide ? 8 : 6) * communityArea.scale
                width: winHandLabel.implicitWidth + 22
                height: Math.max(20, Math.round(26 * tableZone.communityScale))
                radius: height / 2
                color: Qt.rgba(0.05, 0.24, 0.05, 0.92)
                border.color: "#FFD700"
                border.width: 1
                transformOrigin: Item.Center

                // Gleicher weicher Schein wie das Pot-Badge – hier in Gold passend
                // zum Rahmen, damit die Gewinner-Hand ebenso hervorgehoben wird.
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: "#FFD700"
                    shadowOpacity: 0.45
                    shadowBlur: 0.9
                    shadowVerticalOffset: 0
                }

                Text {
                    id: winHandLabel
                    anchors.centerIn: parent
                    text: (typeof GameTable !== "undefined" && GameTable)
                          ? GameTable.winningHandText : ""
                    color: "#FFD700"
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: Math.max(10, Math.round(14 * tableZone.communityScale))
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

            // ── Gegner-Boxen: auf symmetrische Slots verteilt ────────────────────
            // Sitz 0 (Mensch) sitzt unten in der Mitte; die übrigen besetzten Sitze
            // werden gemäß slotSeq links/rechts ausgewogen verteilt.
            Repeater {
                model: 10
                delegate: Item {
                    id: seatSlot
                    required property int index
                    z: 1

                    readonly property var pdata: (typeof GameTable !== "undefined" && GameTable && GameTable.players.length > index)
                        ? GameTable.players[index] : null
                    readonly property bool occupied: pdata !== null && pdata.name !== ""

                    // Position dieses Sitzes unter den Gegnern (1-basiert; Sitz 0 = Mensch)
                    readonly property int oppOrder: {
                        if (typeof GameTable === "undefined" || !GameTable) return 0
                        var c = 0
                        for (var i = 1; i <= index && i < GameTable.players.length; i++)
                            if (GameTable.players[i].name !== "") c++
                        return c
                    }

                    readonly property int oppCount: tableZone.seatCount - 1
                    readonly property var seq: tableZone.slotSeq[oppCount] || []
                    readonly property string slotName:
                        (occupied && oppOrder >= 1 && oppOrder <= seq.length) ? seq[oppOrder - 1] : ""
                    // Immer ein gültiges [x,y]-Paar liefern. Während eines
                    // Orientierungswechsels (oder vor dem ersten Layout, wenn
                    // width/height noch 0 sind) können slotSeq und slotPos kurz
                    // aus verschiedenen Sätzen stammen → Fallback auf die Mitte,
                    // damit slot[0]/slot[1] nie auf undefined zugreifen.
                    readonly property var slot: {
                        if (slotName === "") return [0.5, 0.5]
                        var p = tableZone.slotPos[slotName]
                        return (p === undefined || p === null) ? [0.5, 0.5] : p
                    }

                    visible: occupied && index !== 0 && slotName !== ""

                    // Inhalt füllt die Box ohne überschüssige Ränder; Karten im
                    // Original-Seitenverhältnis (2×31+3=65)
                    // (4 + Avatar 44 + 4 + Karten 65 + 4 + 4 = 125)
                    width: tableZone.oppBaseWidth
                    height: tableZone.oppBaseHeight
                    // Boxen skalieren mit der Auflösung (max = Höhe der Self-Box);
                    // um die Slot-Mitte herum, damit die Position erhalten bleibt.
                    transformOrigin: Item.Center
                    scale: tableZone.oppScale
                    // Hochformat: die Seiten-Boxen als Gruppe vertikal spreizen,
                    // um der Tischmitte mehr Luft zu geben. Untere (Player 1/2/8/9 →
                    // L_lower/L_bottom/R_lower/R_bottom) 14px nach unten, obere
                    // (L_upper/TL/R_upper/TR) 4px nach oben. TC (oben Mitte) bleibt.
                    readonly property real seatNudge: {
                        if (tableZone.wide) return 0
                        if (slotName === "L_lower" || slotName === "L_bottom"
                            || slotName === "R_lower" || slotName === "R_bottom") return 14
                        if (slotName === "L_upper" || slotName === "TL"
                            || slotName === "R_upper" || slotName === "TR") return -4
                        return 0
                    }
                    x: tableZone.width * slot[0] - width / 2
                    y: tableZone.height * slot[1] - height / 2 + seatNudge

                    GamePlayerBox {
                        anchors.fill: parent
                        seatIndex: seatSlot.index
                        // Nur die oberste Box (Player 5, TC-Slot) zeigt das
                        // Winner-Badge im Hochformat unterhalb – sonst überall oben.
                        winnerBelow: !tableZone.wide && seatSlot.slotName === "TC"
                        // Einsatz/Button zur Tischmitte zeigen lassen:
                        // linke Sitze rechts, rechte Sitze links, oben/unten-Mitte unten.
                        // Im breiten (Querformat-)Layout sitzen die oberen Boxen
                        // (Player 4–6) eng im Bogen → Einsatz/Icon unterhalb der Box
                        // anzeigen, damit der seitliche Bereich nicht mit den
                        // Nachbarboxen überlappt.
                        betSide: tableZone.wide
                               ? (seatSlot.slot[0] < 0.45 ? "left"
                                  : seatSlot.slot[0] > 0.55 ? "right"
                                  : "bottom")
                               : seatSlot.slot[0] < 0.45 ? "right"
                               : seatSlot.slot[0] > 0.55 ? "left"
                               : "bottom"
                        // landscapeCompact: bei der obersten Mitte-Box (Player 5)
                        // würde das Badge unterhalb mit dem Pot-Badge kollidieren →
                        // Button links, Einsatz rechts neben der Box anzeigen.
                        betSplit: tableZone.wide && Config.Responsive.landscapeCompact
                                  && seatSlot.slot[0] >= 0.45 && seatSlot.slot[0] <= 0.55
                    }
                }
            }

            // ── Eigene Box: skaliert jetzt mit dem Zoom-Layer ────────────────────
            GamePlayerSelfBox {
                id: selfBox
                z: 1
                anchors.bottom: parent.bottom
                anchors.bottomMargin: tableZone.wide
                    ? 12 + tableZone.selfBaseHeight * (tableZone.boxScale - 1) / 2
                    :  4 + tableZone.selfBaseHeight * (tableZone.boxScale - 1) / 2
                anchors.horizontalCenter: parent.horizontalCenter
                width: tableZone.selfBaseWidth
                height: tableZone.selfBaseHeight
                transformOrigin: Item.Center
                scale: tableZone.boxScale
                maxAvatarSize: tableZone.wide ? 60 : 54
            }

            // Emoji-Reaktions-Animationen – im Zoom-Layer, damit sie bei
            // aktivem Zoom mit den Spielerboxen mitskalieren.
            GameReactionFx {
                id: reactionFx
                anchors.fill: parent
                z: 60
            }

                } // zoomContent

                // Drag-to-Pan: Finger-Delta direkt auf _zoomPanX/Y übertragen.
                // Wird nur im compact-Modus und bei aktivem Zoom zugelassen, damit
                // normale Tisch-Interaktionen unverändert funktionieren.
                DragHandler {
                    id: zoomPanner
                    target: null
                    enabled: Qt.platform.os === "android" && tableZone.zoomActive && Config.Responsive.compact

                    property point _startPt
                    property real  _startX
                    property real  _startY

                    onActiveChanged: {
                        if (active) {
                            _startPt = centroid.position
                            _startX  = tableZone._zoomPanX
                            _startY  = tableZone._zoomPanY
                        }
                    }
                    onCentroidChanged: {
                        if (!active) return
                        var dx   = centroid.position.x - _startPt.x
                        var dy   = centroid.position.y - _startPt.y
                        // Schwenk-Grenzen: Content-Rand darf gerade bis zum
                        // Bildschirmrand reichen → max = (zoom−1)·halfSize
                        var maxX = (tableZone.zoomFactor - 1) * zoomLayer.width  / 2
                        var maxY = (tableZone.zoomFactor - 1) * zoomLayer.height / 2
                        tableZone._zoomPanX = Math.max(-maxX, Math.min(maxX, _startX + dx))
                        tableZone._zoomPanY = Math.max(-maxY, Math.min(maxY, _startY + dy))
                    }
                }

                // Auto-Zentrierung: Wenn der Spieler am Zug ist und der Zoom aktiv
                // ist, wird automatisch auf die Self-Box-Zone geschwenkt, damit
                // Handkarten und Action-Bereich sofort sichtbar sind.
                Connections {
                    target: (typeof GameTable !== "undefined") ? GameTable : null
                    function onMyTurnChanged() {
                        if (!tableZone.zoomActive || !GameTable || !GameTable.myTurn)
                            return
                        tableZone._zoomPanY = -(tableZone.zoomFactor - 1) * zoomLayer.height / 2
                        tableZone._zoomPanX = 0
                    }
                    function onTimeoutChanged() {
                        if (!tableZone.zoomActive || !GameTable || zoomPanner.active) return
                        var seatId = GameTable.timeoutSeatId
                        if (seatId <= 0) return
                        tableZone._panToSeat(seatId)
                    }
                    function onPlayersChanged() {
                        if (!tableZone.zoomActive || !GameTable || zoomPanner.active) return
                        var players = GameTable.players
                        for (var i = 1; i < players.length; i++) {
                            if (players[i].name !== "" && players[i].myTurn) {
                                tableZone._panToSeat(i)
                                return
                            }
                        }
                    }
                    function onBoardCardsChanged() {
                        if (!tableZone.zoomActive || !GameTable || zoomPanner.active) return
                        if (GameTable.boardCardCount <= 0) return
                        tableZone._panToPoint(tableZone.width / 2, tableZone.communityCenterY)
                    }
                    function onWinningHandTextChanged() {
                        if (!tableZone.zoomActive || !GameTable || zoomPanner.active) return
                        if (!GameTable.winningHandText) return
                        // Mittelpunkt zwischen Community-Karten-Mitte und Self-Box-Mitte:
                        // Beide Bereiche gleichzeitig mit ~20px Rand sichtbar.
                        var selfCY = tableZone.selfVisualTopY
                                     + tableZone.selfBaseHeight * tableZone.boxScale / 2
                        var cy = (tableZone.communityCenterY + selfCY) / 2
                        tableZone._panToPoint(tableZone.width / 2, cy)
                    }
                }
            } // zoomLayer

            // ── Spielverlauf (Log) + Chat – Umschalt-Icons + Overlays ──────────
            property bool showLog: false
            property bool showChat: false
            // Emoji-Reaktions-Picker (Panel unter dem Toggle neben dem Chat-Icon)
            property bool showReactions: false

            // ── Permanenter Game-Chat unten links (nur Desktop, nie Android) ───
            // Der Chat wird dauerhaft links neben der Action-Box gedockt (gleiche
            // Höhe, gleiche vertikale Position) – sofern dort genug freie Breite
            // ist. Reicht der Platz nicht, bleibt es beim Overlay-Chat (Chat-Icon).
            readonly property real dockedChatW: {
                if (Config.Responsive.isMobile) return 0
                if (typeof GameTable === "undefined" || !GameTable || !GameTable.hasHumanOpponents) return 0
                return Math.min(280, (width - actionBar.panelWidth) / 2 - 24)
            }
            readonly property bool dockedChatFits: dockedChatW >= 170
            // Wird der Chat gedockt, ist das Overlay überflüssig.
            onDockedChatFitsChanged: if (dockedChatFits) showChat = false

            // Minimale Höhe des gedockten Chats (= Action-Bar-Höhe minus Außenabstand).
            readonly property real dockedChatMinH: actionBar.height - 8
            // Maximale Höhe: so weit nach oben aufziehbar, bis die Unterkante
            // der untersten Gegnerbox, die horizontal mit dem Chat überlappt,
            // (+ 8 px Abstand) erreicht ist – keine Überlappung garantiert.
            readonly property real dockedChatMaxH: {
                if (!wide || !dockedChatFits) return dockedChatMinH
                var s = oppScale
                var visualW = oppBaseWidth  * s
                var visualH = oppBaseHeight * s
                // Horizontaler Bereich des Chats in tableZone-Koordinaten
                // (Chat ist links mit 8 px Abstand verankert, Breite = dockedChatW).
                var chatLeft  = 8
                var chatRight = 8 + dockedChatW
                // Alle Landscape-Slots durchsuchen: welche Boxen überlappen horizontal?
                var slots = slotPosLandscape
                var maxH = height + actionBar.height - 8   // kein Limit → voll
                for (var name in slots) {
                    var pos     = slots[name]
                    var boxCX   = width  * pos[0]
                    var boxCY   = height * pos[1]
                    var boxL    = boxCX - visualW / 2
                    var boxR    = boxCX + visualW / 2
                    // Überlapp nur prüfen, wenn Box im Chat-Bereich liegt.
                    if (boxR <= chatLeft || boxL >= chatRight) continue
                    // Unterkante der Box + 8 px Sicherheitsabstand:
                    var boxBottom = boxCY + visualH / 2 + 8
                    // Chat darf höchstens bis zur Unterkante dieser Box reichen.
                    var limit = height - boxBottom + actionBar.height - 8
                    if (limit < maxH) maxH = limit
                }
                return Math.max(dockedChatMinH, maxH)
            }
            // Vom Benutzer per Drag-Handle eingestellte Höhe; -1 = Standard.
            property real dockedChatUserH: -1

            // Ungelesene Chat-Nachrichten: alles oberhalb von chatReadCount gilt als
            // ungelesen. Als gelesen markiert wird, sobald der Chat 2 s offen war
            // (chatReadTimer); danach gelten weitere Nachrichten bei offenem Chat
            // sofort als gelesen.
            property int chatReadCount: 0
            readonly property int chatUnread: {
                var n = (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog.length : 0
                return Math.max(0, n - chatReadCount)
            }
            onShowChatChanged: {
                if (showChat) chatReadTimer.restart()
                else chatReadTimer.stop()
            }
            Timer {
                id: chatReadTimer
                interval: 2000
                onTriggered: tableZone.chatReadCount =
                    (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog.length : 0
            }
            Connections {
                target: (typeof GameTable !== "undefined") ? GameTable : null
                // Bei offenem, bereits gelesenem Chat (2s-Timer abgelaufen) gelten
                // neue Nachrichten sofort als gelesen.
                function onChatLogChanged() {
                    // Chat wurde geleert (neues Spiel) → Zähler nachführen.
                    if (GameTable.chatLog.length < tableZone.chatReadCount)
                        tableZone.chatReadCount = GameTable.chatLog.length
                    if (tableZone.showChat && !chatReadTimer.running)
                        tableZone.chatReadCount = GameTable.chatLog.length
                }
            }

            Item {
                id: logOverlay
                z: 150
                // Querformat/Vollbild: Sidebar (~1/3 Breite) von rechts.
                // Hochformat: volles Overlay über den Tisch.
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                width: tableZone.wide ? Math.max(parent.width / 3, 300) : parent.width
                visible: tableZone.showLog

                // Schwebendes Sheet: eingerückt, abgerundet, mit Elevation.
                Rectangle {
                    id: logPanel
                    anchors.fill: parent
                    anchors.topMargin: 50   // Abstand zum Umschalt-Icon oben rechts
                    anchors.bottomMargin: 10
                    anchors.leftMargin: tableZone.wide ? 10 : 8
                    anchors.rightMargin: tableZone.wide ? 10 : 8
                    radius: 16
                    color: Config.Theme.withAlpha(Config.StaticData.palette.secondary.col700, 0.95)
                    border.color: Config.StaticData.palette.secondary.col500
                    border.width: 1

                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: "#000000"
                        shadowOpacity: 0.55
                        shadowBlur: 0.9
                        shadowVerticalOffset: 3
                        shadowHorizontalOffset: 0
                    }
                }

                // Klicks innerhalb des Sheets abfangen (Tisch daneben bleibt nutzbar)
                MouseArea { anchors.fill: logPanel }

                ColumnLayout {
                    anchors.fill: logPanel
                    anchors.margins: 12
                    spacing: 8

                    // Header: Titel + Schließen
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Spielverlauf")
                            color: Config.Theme.colorAccent
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 15
                            font.bold: true
                            font.letterSpacing: 0.4
                        }
                        Rectangle {
                            Layout.preferredWidth: 26
                            Layout.preferredHeight: 26
                            radius: 13
                            color: logCloseArea.containsMouse
                                   ? Config.Theme.withAlpha(Config.StaticData.palette.secondary.col500, 0.7)
                                   : "transparent"
                            VectorImage {
                                anchors.centerIn: parent
                                width: 14; height: 14
                                source: "../resources/close.svg"
                                layer.enabled: true
                                layer.effect: MultiEffect {
                                    colorization: 1.0
                                    colorizationColor: Config.StaticData.palette.secondary.col200
                                }
                            }
                            MouseArea {
                                id: logCloseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: gamePage.toggleLogOverlay()
                            }
                        }
                    }

                    // Trennlinie
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Config.Theme.withAlpha(Config.StaticData.palette.secondary.col500, 0.5)
                    }

                    ListView {
                        id: logList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: (typeof GameTable !== "undefined" && GameTable) ? GameTable.gameLog : []
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {
                            policy: logList.contentHeight > logList.height + 4
                                    ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                        }
                        // Auto-Scroll folgt neuen Einträgen, solange der Nutzer unten
                        // ist. Scrollt er hoch, pausiert das Folgen und die Position
                        // bleibt erhalten – auch wenn neue Zeilen ankommen (das Model
                        // ist eine QVariantList, die bei jeder Änderung komplett ersetzt
                        // wird → die View würde sonst auf contentY=0 zurückspringen).
                        // Nach 3 s ohne Scroll-Bewegung (Timeout ggf. tunen) springt es
                        // wieder ans Ende.
                        property bool autoScroll: true
                        property real savedContentY: 0
                        Timer {
                            id: logAutoScrollTimer
                            interval: 15000
                            onTriggered: { logList.autoScroll = true; logList.positionViewAtEnd() }
                        }
                        function restoreScroll() {
                            contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
                        }
                        // Nur benutzergetriebene Bewegungen (moving = Drag/Flick/Wheel)
                        // auswerten; programmatische Resets/Sprünge ignorieren.
                        onContentYChanged: {
                            if (!moving) return
                            savedContentY = contentY
                            if (atYEnd) { autoScroll = true; logAutoScrollTimer.stop() }
                            else        { autoScroll = false; logAutoScrollTimer.restart() }
                        }
                        onCountChanged: {
                            if (autoScroll) positionViewAtEnd()
                            else Qt.callLater(restoreScroll)
                        }
                        delegate: Text {
                            required property var modelData
                            width: ListView.view.width
                            text: modelData
                            // Farben kommen aus dem HTML (Widgets-Log-Style).
                            textFormat: Text.RichText
                            wrapMode: Text.WordWrap
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 12
                            lineHeight: 1.15
                            bottomPadding: 4
                        }
                    }
                }
            }

            Rectangle {
                id: logToggle
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                width: 34; height: 34; radius: 17
                color: tableZone.showLog ? Config.Theme.colorAccent : Qt.rgba(0, 0, 0, 0.45)

                VectorImage {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    source: "../resources/gameLog.svg"
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: tableZone.showLog ? "#101010" : "#FFFFFF"
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: gamePage.toggleLogOverlay()
                }
            }

            // ── Chat-Overlay (nur bei menschlichen Mitspielern) ────────────────
            Item {
                id: chatOverlay
                z: 150
                // Querformat/Vollbild: Sidebar (~1/3 Breite) von links.
                // Hochformat: volles Overlay über den Tisch.
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: tableZone.wide ? Math.max(parent.width / 3, 300) : parent.width
                visible: tableZone.showChat
                // Chat geschlossen → Emoji-Picker mitschließen.
                onVisibleChanged: if (!visible) overlayChat.closeEmojiPicker()

                // Schwebendes Sheet (von links): eingerückt, abgerundet, mit Elevation.
                Rectangle {
                    id: chatPanel
                    anchors.fill: parent
                    anchors.topMargin: 50   // Abstand zum Chat-Icon oben links
                    anchors.bottomMargin: 10
                    anchors.leftMargin: tableZone.wide ? 10 : 8
                    anchors.rightMargin: tableZone.wide ? 10 : 8
                    radius: 16
                    color: Config.Theme.withAlpha(Config.StaticData.palette.secondary.col700, 0.95)
                    border.color: Config.StaticData.palette.secondary.col500
                    border.width: 1

                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowColor: "#000000"
                        shadowOpacity: 0.55
                        shadowBlur: 0.9
                        shadowVerticalOffset: 3
                        shadowHorizontalOffset: 0
                    }
                }

                MouseArea { anchors.fill: chatPanel }   // Klicks abfangen

                ColumnLayout {
                    anchors.fill: chatPanel
                    anchors.margins: 12
                    spacing: 8

                    // Header: Titel + Schließen
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Chat")
                            color: Config.Theme.colorAccent
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 15
                            font.bold: true
                            font.letterSpacing: 0.4
                        }
                        Rectangle {
                            Layout.preferredWidth: 26
                            Layout.preferredHeight: 26
                            radius: 13
                            color: chatCloseArea.containsMouse
                                   ? Config.Theme.withAlpha(Config.StaticData.palette.secondary.col500, 0.7)
                                   : "transparent"
                            VectorImage {
                                anchors.centerIn: parent
                                width: 14; height: 14
                                source: "../resources/close.svg"
                                layer.enabled: true
                                layer.effect: MultiEffect {
                                    colorization: 1.0
                                    colorizationColor: Config.StaticData.palette.secondary.col200
                                }
                            }
                            MouseArea {
                                id: chatCloseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: gamePage.toggleChatOverlay()
                            }
                        }
                    }

                    // Trennlinie
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Config.Theme.withAlpha(Config.StaticData.palette.secondary.col500, 0.5)
                    }

                    ChatBox {
                        id: overlayChat
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        chatModel: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog : []
                        nickList: gamePage.gameNickList()
                        onSendRequested: (text) => {
                            if (typeof GameTable !== "undefined" && GameTable)
                                GameTable.sendChat(text)
                        }
                    }
                }
            }

            Rectangle {
                id: chatToggle
                z: 200
                // Ausgeblendet, wenn der Chat permanent unten links gedockt ist.
                visible: ((typeof GameTable !== "undefined" && GameTable) ? GameTable.hasHumanOpponents : false)
                         && !tableZone.dockedChatFits
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 8
                width: 34; height: 34; radius: 17
                color: tableZone.showChat ? Config.Theme.colorAccent : Qt.rgba(0, 0, 0, 0.45)

                VectorImage {
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    source: "../resources/gameChat.svg"
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: tableZone.showChat ? "#101010" : "#FFFFFF"
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: gamePage.toggleChatOverlay()
                }

                // Badge mit Anzahl ungelesener Chat-Nachrichten.
                Rectangle {
                    visible: tableZone.chatUnread > 0
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: -3
                    anchors.rightMargin: -3
                    width: Math.max(17, unreadLabel.implicitWidth + 8)
                    height: 17
                    radius: 8.5
                    color: Config.Theme.colorDanger
                    border.color: "#1d222b"
                    border.width: 1.5

                    Text {
                        id: unreadLabel
                        anchors.centerIn: parent
                        text: tableZone.chatUnread > 99 ? "99+" : tableZone.chatUnread
                        color: "#FFFFFF"
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
            }

            // ── Emoji-Reaktions-Picker: Toggle rechts neben dem Chat-Icon ──────
            Rectangle {
                id: reactionToggle
                z: 200
                visible: gamePage.emojiReactionsEnabled
                anchors.top: parent.top
                anchors.left: chatToggle.visible ? chatToggle.right : parent.left
                anchors.leftMargin: chatToggle.visible ? 6 : 8
                anchors.topMargin: 8
                width: 34; height: 34; radius: 17
                color: tableZone.showReactions ? Config.Theme.colorAccent : Qt.rgba(0, 0, 0, 0.45)

                Text {
                    anchors.centerIn: parent
                    text: "🎉"
                    font.family: Config.StaticData.emojiFamily
                    font.pixelSize: 17
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: tableZone.showReactions = !tableZone.showReactions
                }
            }

            // Panel mit den Reaktions-Emojis (Grid, 6 Spalten – wie der
            // Reaction-Picker des Web-Clients, dort 30 Emojis).
            Rectangle {
                id: reactionPanel
                visible: tableZone.showReactions && gamePage.emojiReactionsEnabled
                z: 210
                anchors.top: reactionToggle.bottom
                anchors.topMargin: 6
                anchors.left: parent.left
                anchors.leftMargin: 8
                width: reactionGrid.width + 16
                height: reactionGrid.height + 16
                radius: 8
                color: Qt.rgba(0, 0, 0, 0.88)
                border.color: Qt.rgba(1, 1, 1, 0.12)
                border.width: 1

                Grid {
                    id: reactionGrid
                    anchors.centerIn: parent
                    columns: 6
                    spacing: 3

                    Repeater {
                        model: ["🎉", "🥳", "👏", "🙌", "💪", "🤣",
                                "😂", "😬", "🤦", "😴", "👍", "😎",
                                "🤩", "👀", "🤔", "😱", "😡", "😤",
                                "🔥", "😮", "💰", "💎", "🎰", "🍀",
                                "🃏", "💀", "🤑", "🫵", "🫡", "🤫"]
                        delegate: Rectangle {
                            required property string modelData
                            width: 36; height: 36; radius: 6
                            color: reactArea.containsPress ? Qt.rgba(1, 1, 1, 0.25)
                                 : reactArea.containsMouse ? Qt.rgba(1, 1, 1, 0.12)
                                 : "transparent"
                            scale: reactArea.containsMouse && !reactArea.containsPress ? 1.15 : 1.0
                            Behavior on scale { NumberAnimation { duration: 100 } }

                            Text {
                                anchors.centerIn: parent
                                text: parent.modelData
                                font.family: Config.StaticData.emojiFamily
                                font.pixelSize: 19
                            }
                            MouseArea {
                                id: reactArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: gamePage.sendReaction(parent.modelData)
                            }
                        }
                    }
                }
            }

        }

        // 3. Action-Leiste: Raise-Controls + Fold / Call / Raise
        Item {
            id: actionBar
            Layout.fillWidth: true
            // Höhe wächst dynamisch mit dem Inhalt (Desktop-Querformat: +8 px,
            // damit das Panel mit 8 px Abstand über dem unteren Bildschirmrand
            // schwebt). Auf dem Phone (compactActions) sitzt das Panel bündig
            // am unteren Bildschirmrand.
            Layout.preferredHeight: actionBarCol.implicitHeight
                                    + (tableZone.wide && !actionBar.compactActions ? 8 : 0)

            // Querformat: Inhalt auf die (skalierte) Breite des Community-Cards-
            // Bereichs begrenzen und zentrieren – sonst wird u. a. der Slider viel
            // zu breit. Eine Untergrenze stellt sicher, dass die Steuerelemente
            // (Pot-Buttons + All-In + Spielmodus) nicht zu eng werden. Hochformat:
            // volle Breite.
            readonly property real panelWidth: tableZone.wide
                ? Math.min(width, Math.max(communityArea.width * communityArea.scale, 380))
                : width

            // Aktuell vorbereiteter Raise-Betrag; kann auch vor dem eigenen Zug gesetzt werden
            property int raiseAmount: 0

            readonly property bool raiseAvailable: GameTable !== null
                                                   && GameTable.maxRaiseAmount > 0
                                                   && GameTable.minRaiseAmount > 0
            readonly property int raiseMinAmount: raiseAvailable ? GameTable.minRaiseAmount : 0
            readonly property int raiseMaxAmount: raiseAvailable ? GameTable.maxRaiseAmount : 0

            // Dynamische Button-Beschriftungen – analog zum Qt-Widgets-Client:
            //  • nichts zu callen  → "Check"      sonst → "Call $X"
            //  • Preflop oder schon gesetzt → "Raise $X"; postflop ohne Einsatz → "Bet $X"
            readonly property bool canCheck: GameTable !== null && GameTable.callAmount === 0
            readonly property bool isPreflop: GameTable !== null && GameTable.phaseText === "Preflop"
            readonly property string _amountSep: "\n"
            readonly property string checkCallText: GameTable === null ? qsTr("Call")
                : (canCheck ? qsTr("Check") : qsTr("Call") + _amountSep + "$" + GameTable.callAmount)
            readonly property string betRaiseText: {
                if (GameTable === null) return qsTr("Raise")
                var word = (!isPreflop && canCheck) ? qsTr("Bet") : qsTr("Raise")
                return raiseAvailable ? (word + _amountSep + "$" + raiseAmount) : word
            }

            // ── Vorwahl (pre-selection): vor dem eigenen Zug eine Aktion vormerken ──
            property string preAction: ""        // "", "fold", "call", "raise", "allin"
            // Vorauswahl-Freigabe: false nach eigenem Zug,
            // true bei Rundenwechsel (onRoundValuesReady) oder Gegner-Aktion.
            property bool preSelectEnabled: true
            // Reset bei Handwechsel oder Showdown
            property int lastHandNumber: -1
            Connections {
                target: GameTable
                function onHandNumberChanged() {
                    if (GameTable && GameTable.handNumber !== actionBar.lastHandNumber) {
                        actionBar.preAction = ""
                        actionBar.preSelectEnabled = true   // neue Hand → Vorauswahl freischalten
                        actionBar.lastHandNumber = GameTable.handNumber
                        console.log("[ACTDBG] preAction Reset: Neue Hand " + actionBar.lastHandNumber)
                    }
                }
                function onPhaseTextChanged() {
                    if (!GameTable) return
                    if (GameTable.phaseText === "Showdown") {
                        actionBar.preAction = ""
                        console.log("[ACTDBG] preAction Reset: Showdown")
                    } else if (GameTable.phaseText !== "Preflop") {
                        // Flop/Turn/River: Vorauswahl zurücksetzen; Freischalten erfolgt
                        // in onRoundValuesReady (nach computeCallAndRaiseAmounts) –
                        // analog zum Widget-Client (updateMyButtonsState nach dealXCards2).
                        actionBar.preAction = ""
                        console.log("[ACTDBG] preAction Reset: Rundenwechsel →", GameTable.phaseText)
                    } else {
                        actionBar.preSelectEnabled = true   // Preflop = neue Hand
                    }
                }
            }
            property int preCallAmount: -1        // callAmount zum Zeitpunkt der Vorwahl
            // Spielmodus: 0 = manuell, 1 = Auto Check/Call, 2 = Auto Check/Fold.
            property int playingMode: 0

            readonly property bool canAct: GameTable !== null && GameTable.canAct

            // Kompakte Action-Bar nur auf echten Mobilgeräten mit knappem
            // vertikalem Platz (Phone-Landscape). Auf dem Desktop bleiben die
            // Buttons groß – auch bei breitem Aspect-Ratio (Ultrawide/HiDPI),
            // wo landscapeCompact geometrisch ebenfalls greift.
            readonly property bool compactActions:
                Config.Responsive.landscapeCompact && Config.Responsive.isMobile
            // Höhen der drei Action-Bar-Reihen.
            readonly property int actionRowHeight: compactActions ? 40 : (Config.Theme.compact ? 56 : 54)
            readonly property int raiseRowHeight:  compactActions ? 22 : 26

            // Während der Vorwahl zeigt der Fold-Button bei freiem Check "Check / Fold"
            // Vorwahl bei gratis Check: zweizeilig, damit auch längere Übersetzungen
            // (z. B. "Check / Se coucher") auf den Button passen.
            readonly property string foldText: (GameTable !== null && !GameTable.myTurn && canCheck)
                ? (qsTr("Check") + " /\n" + qsTr("Fold")) : qsTr("Fold")

            function fireAction(which) {
                if (GameTable === null) return
                if (which === "fold")       GameTable.fold()
                else if (which === "call")  GameTable.call()
                else if (which === "raise") GameTable.raise(raiseAmount)
                else if (which === "allin") GameTable.allIn()
            }

            // Vorgemerkte Aktion beim eigenen Zug ausführen.
            // Vorgemerktes "Fold" wird zu "Check", falls ein Check gratis möglich ist.
            function runPreAction(which) {
                if (which === "fold" && canCheck) GameTable.call()
                else fireAction(which)
            }

            function clickAction(which) {
                if (GameTable === null) return
                // Eigener Klick auf einen Action-Button hat Vorrang vor dem
                // Auto-Modus → zurück auf "manuell", dann die Aktion ausführen
                // bzw. vormerken (wie im Qt-Widgets-Client).
                if (playingMode !== 0)
                    playingMode = 0
                // Es ist mein Zug, sobald der Server meinen Aktions-Timer zählt
                // (timeoutSeatId === 0) – auch wenn das myTurn-Flag noch nicht
                // gesetzt sein sollte. Dann SOFORT ausführen, sonst nur vormerken.
                var myTurnNow = GameTable.myTurn || GameTable.timeoutSeatId === 0
                var p0btnDbg = GameTable.players.length > 0 ? GameTable.players[0]["button"] : -1
                console.log("[ACTDBG] click", which,
                            "myTurn=", GameTable.myTurn,
                            "tSeat=", GameTable.timeoutSeatId,
                            "canAct=", GameTable.canAct,
                            "callAmt=", GameTable.callAmount,
                            "preSel=", preSelectEnabled,
                            "p0btn=", p0btnDbg,
                            "(1=D,2=SB,3=BB)",
                            "phase=", GameTable.phaseText,
                            "pre=", preAction,
                            "→ myTurnNow=", myTurnNow)
                if (myTurnNow) {
                    preAction = ""
                    fireAction(which)
                } else if (canAct) {
                    if (preAction === which) {
                        preAction = ""
                    } else {
                        preAction = which
                        preCallAmount = (which === "call") ? GameTable.callAmount : -1
                    }
                }
            }

            function raiseStepFor(maximum) {
                if (maximum <= 1000)
                    return 10
                if (maximum <= 10000)
                    return 50
                if (maximum <= 100000)
                    return 500
                return 5000
            }

            function roundedRaiseAmount(amount) {
                if (!raiseAvailable)
                    return 0
                if (amount >= raiseMaxAmount)
                    return raiseMaxAmount
                var step = raiseStepFor(raiseMaxAmount)
                return Math.floor(amount / step) * step
            }

            function clampRaiseAmount(amount) {
                if (!raiseAvailable)
                    return 0
                return Math.max(raiseMinAmount, Math.min(raiseMaxAmount, amount))
            }

            function syncRaiseAmount() {
                if (!raiseAvailable) {
                    raiseAmount = 0
                    return
                }
                if (raiseAmount <= 0)
                    raiseAmount = raiseMinAmount
                else
                    raiseAmount = clampRaiseAmount(roundedRaiseAmount(raiseAmount))
            }

            // Raise-Wert vorbereiten, Vorwahl ausführen bzw. bei Änderungen verwerfen
            Connections {
                target: GameTable
                function onMyTurnChanged() {
                    // Eigener Zug beginnt → Vorauswahl immer freischalten.
                    // Ausführung der vorgemerkten/automatischen Aktion in onMeInActionTriggered.
                    if (GameTable.myTurn)
                        actionBar.preSelectEnabled = true
                    actionBar.syncRaiseAmount()
                }
                function onMeInActionTriggered() {
                    // Wie meInAction() im Widgets-Client: GENAU HIER die gemerkte
                    // bzw. automatische Aktion ausführen. Dieser Callback kommt bei
                    // jedem eigenen Zug verlässlich (auch wenn m_myTurn schon true
                    // war) → keine verschluckten Aktionen mehr.
                    var p0btnDbg2 = GameTable.players.length > 0 ? GameTable.players[0]["button"] : -1
                    console.log("[ACTDBG] meInActionTriggered",
                                "pre=", actionBar.preAction,
                                "preCallAmt=", actionBar.preCallAmount,
                                "mode=", actionBar.playingMode,
                                "myTurn=", GameTable.myTurn,
                                "tSeat=", GameTable.timeoutSeatId,
                                "callAmt=", GameTable.callAmount,
                                "p0btn=", p0btnDbg2,
                                "(1=D,2=SB,3=BB)",
                                "phase=", GameTable.phaseText,
                                "canAct=", GameTable.canAct,
                                "preSel=", actionBar.preSelectEnabled)
                    actionBar.syncRaiseAmount()

                    if (actionBar.playingMode === 2 || actionBar.playingMode === 1) {
                        gamePage.runAutoAction()
                    } else if (actionBar.preAction !== "") {       // Manuell: Vorwahl ausführen
                        var a = actionBar.preAction
                        actionBar.preAction = ""
                        actionBar.runPreAction(a)
                    }
                    // Nach eigenem Zug: Vorauswahl sperren bis Gegner-Aktion oder Rundenwechsel
                    actionBar.preSelectEnabled = false
                }
                function onRoundValuesReady() {
                    // Werte nach Rundenwechsel sind jetzt korrekt (nach computeCallAndRaiseAmounts()).
                    // Analog zum Widget-Client (updateMyButtonsState nach dealFlopCards2 etc.):
                    // Vorauswahl sofort freischalten – nicht erst auf die erste Spieler-Aktion warten.
                    actionBar.preSelectEnabled = true
                }
                function onRefreshActionTriggered() {
                    if (GameTable.callAmount > 0 && !GameTable.myTurn) {
                        // Gegner hat gesetzt/erhöht → Vorauswahl freischalten.
                        // callAmountChanged allein taugt nicht: feuert auch nach
                        // eigener Aktion (onRefreshSet/Pot/Cash) mit veralteten Werten.
                        actionBar.preSelectEnabled = true
                    }
                    // Sicherheit: vorgemerkter Call verfällt nur bei einer ECHTEN
                    // Gegner-Aktion (FOLD/CHECK/CALL/BET/RAISE/ALLIN), die den Call-
                    // Betrag verändert hat. refreshActionTriggered feuert
                    // ausschließlich für solche Aktionen — Blind-Posts (preflop
                    // SB→BB) lösen dieses Signal NICHT aus, sodass eine
                    // Vorauswahl während des Blindings nicht mehr stillschweigend
                    // gelöscht wird (war Auslöser für „UTG-preflop ohne Reaktion,
                    // Timeout mit Default-Action").
                    if (actionBar.preAction === "call"
                        && GameTable.callAmount !== actionBar.preCallAmount)
                        actionBar.preAction = ""
                }
                function onCallAmountChanged() {
                    // KEIN preSelectEnabled=true hier: callAmountChanged feuert bei
                    // jedem computeCallAndRaiseAmounts()-Aufruf (onRefreshSet/Pot/Cash)
                    // auch mit veralteten Werten → Freischalten nur in onRefreshActionTriggered.
                    // Den Pre-Action-Sicherheits-Check führen wir bewusst NICHT
                    // mehr hier aus, sondern in onRefreshActionTriggered (s.o.) —
                    // sonst löschten Blind-Posts (callAmount 0→SB→BB) jede
                    // UTG-Pre-Action.
                    actionBar.syncRaiseAmount()
                }
                function onMinRaiseAmountChanged() {
                    if (actionBar.preAction === "raise" && !actionBar.raiseAvailable)
                        actionBar.preAction = ""
                    actionBar.syncRaiseAmount()
                }
                function onMaxRaiseAmountChanged() {
                    if (actionBar.preAction === "raise" && !actionBar.raiseAvailable)
                        actionBar.preAction = ""
                    actionBar.syncRaiseAmount()
                }
                function onCanActChanged() {
                    // Kann nicht mehr agieren (gefoldet/all-in) → Vorwahl löschen
                    if (!GameTable.canAct)
                        actionBar.preAction = ""
                }
            }

            Rectangle {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                // Desktop-Querformat: kleiner Abstand zum unteren Bildschirmrand
                // (Tisch zeigt sich darunter durch). Phone (compactActions):
                // Panel bündig am unteren Bildschirmrand.
                anchors.bottomMargin: tableZone.wide && !actionBar.compactActions ? 8 : 0
                anchors.horizontalCenter: parent.horizontalCenter
                width: actionBar.panelWidth
                color: Qt.rgba(0, 0, 0, 0.82)
                // Geschrumpft (Querformat) als leicht abgerundetes Panel.
                radius: tableZone.wide ? 10 : 0
            }

            Column {
                id: actionBarCol
                width: actionBar.panelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 0

                // ── Raise-Bereich: dauerhaft vorbereitbar, Aktion erst beim eigenen Zug ──
                Column {
                    id: raiseSection
                    width: parent.width
                    spacing: 3
                    topPadding: 4
                    bottomPadding: 2
                    leftPadding: 8
                    rightPadding: 8
                    visible: GameTable !== null
                    height: visible ? implicitHeight : 0
                    clip: true

                    // Zeile 1: Betrag-Eingabe (links) + Slider
                    RowLayout {
                        width: parent.width - 16
                        spacing: 6

                        // Betrag-Eingabe – links neben dem Slider
                        Rectangle {
                            Layout.preferredWidth: 78
                            Layout.preferredHeight: actionBar.raiseRowHeight
                            Layout.alignment: Qt.AlignVCenter
                            radius: 5
                            color: actionBar.raiseAvailable ? "#1a2a1a" : "#171717"
                            border.color: actionBar.raiseAvailable ? "#4CAF50" : "#3a3a3a"
                            border.width: 1
                            TextInput {
                                id: raiseAmountInput
                                anchors { fill: parent; leftMargin: 6; rightMargin: 6 }
                                enabled: actionBar.raiseAvailable
                                text: actionBar.raiseAmount.toString()
                                color: enabled ? "#FFFFFF" : "#8a8a8a"
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 13
                                font.bold: true
                                horizontalAlignment: Qt.AlignHCenter
                                verticalAlignment: Qt.AlignVCenter
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 0; top: 9999999 }
                                // Live-Aktualisierung des Bet/Raise-Buttons während der Eingabe –
                                // analog zu spinBoxBetValueChanged() im Qt-Widgets-Client.
                                onTextChanged: {
                                    var v = parseInt(text)
                                    if (!isNaN(v) && actionBar.raiseAvailable)
                                        actionBar.raiseAmount = actionBar.clampRaiseAmount(v)
                                }
                                onAccepted: {
                                    var v = parseInt(text)
                                    if (!isNaN(v) && GameTable) {
                                        actionBar.raiseAmount = actionBar.clampRaiseAmount(v)
                                    }
                                    // Enter im Raise-Feld löst Bet/Raise aus (wie der
                                    // Qt-Widgets-Client: Enter bei fokussiertem Betrag).
                                    actionBar.clickAction("raise")
                                }
                                // Text bleibt synchron mit raiseAmount (von Slider/%-Buttons)
                                onActiveFocusChanged: {
                                    if (!activeFocus) {
                                        text = actionBar.raiseAmount.toString()
                                    }
                                }
                                Connections {
                                    target: actionBar
                                    function onRaiseAmountChanged() {
                                        if (!raiseAmountInput.activeFocus)
                                            raiseAmountInput.text = actionBar.raiseAmount.toString()
                                    }
                                }
                            }
                        }

                        Slider {
                            id: raiseSlider
                            Layout.fillWidth: true
                            Layout.preferredHeight: actionBar.raiseRowHeight
                            Layout.alignment: Qt.AlignVCenter
                            enabled: actionBar.raiseAvailable
                            opacity: enabled ? 1.0 : 0.45
                            from: actionBar.raiseMinAmount
                            to: actionBar.raiseAvailable ? Math.max(actionBar.raiseMinAmount, actionBar.raiseMaxAmount) : 1
                            stepSize: actionBar.raiseStepFor(actionBar.raiseMaxAmount)
                            value: actionBar.raiseAmount
                            onMoved: actionBar.raiseAmount = actionBar.clampRaiseAmount(actionBar.roundedRaiseAmount(value))

                            background: Rectangle {
                                x: raiseSlider.leftPadding
                                y: raiseSlider.topPadding + raiseSlider.availableHeight / 2 - height / 2
                                width: raiseSlider.availableWidth
                                height: 4
                                radius: 2
                                color: "#333333"
                                Rectangle {
                                    width: raiseSlider.visualPosition * parent.width
                                    height: parent.height
                                    radius: 2
                                    color: "#4CAF50"
                                }
                            }
                            handle: Rectangle {
                                x: raiseSlider.leftPadding + raiseSlider.visualPosition * (raiseSlider.availableWidth - width)
                                y: raiseSlider.topPadding + raiseSlider.availableHeight / 2 - height / 2
                                width: 18; height: 18; radius: 9
                                color: raiseSlider.pressed ? "#80FF80" : "#4CAF50"
                                border.color: "#2a7a2a"
                                border.width: 1
                            }
                        }
                    }

                    // Zeile 2: Pot-%-Buttons + All-In (bündig) + Spielmodus-Dropdown (rechts)
                    RowLayout {
                        width: parent.width - 16
                        spacing: 4

                        // Pot-Prozent-Buttons: 1/3 · 1/2 · Pot
                        Repeater {
                            model: [
                                { label: "1/3", frac: 1.0 / 3.0 },
                                { label: "1/2", frac: 0.5 },
                                { label: "Pot", frac: 1.0 }
                            ]
                            delegate: Rectangle {
                                required property var modelData
                                visible: SettingsManager
                                         ? SettingsManager.readConfigInt("ShowPotPercentButtons") !== 0
                                         : true
                                Layout.preferredWidth: visible ? 38 : 0
                                Layout.preferredHeight: actionBar.raiseRowHeight
                                radius: 5
                                enabled: actionBar.raiseAvailable
                                color: !enabled ? "#202020" : potBtnArea.containsPress ? "#2e7d32" : potBtnArea.containsMouse ? "#388e3c" : "#1b5e20"
                                border.color: enabled ? "#4CAF50" : "#3a3a3a"
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: parent.enabled ? "#FFFFFF" : "#8a8a8a"
                                    font.family: Config.StaticData.loadedFont.font.family
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                MouseArea {
                                    id: potBtnArea
                                    anchors.fill: parent
                                    cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                    hoverEnabled: parent.enabled
                                    enabled: parent.enabled
                                    onClicked: {
                                        if (!GameTable || !actionBar.raiseAvailable) return
                                        var tp = GameTable.totalPot
                                        var tgt = Math.round(tp * modelData.frac)
                                        actionBar.raiseAmount = actionBar.clampRaiseAmount(tgt)
                                    }
                                }
                            }
                        }

                        // All-In / Show – bündig an die Pot-Buttons
                        // Im Post-River: zeigt "Show"-Button wenn der Spieler seine Karten
                        // freiwillig zeigen kann (temporär als Ersatz für All-In).
                        Rectangle {
                            id: allInBtn
                            readonly property bool preChecked: actionBar.preAction === "allin"
                            readonly property bool isShowMode: typeof GameTable !== "undefined" && GameTable && GameTable.canShowCards
                            Layout.preferredWidth: 52
                            Layout.preferredHeight: actionBar.raiseRowHeight
                            radius: 5
                            opacity: (isShowMode || (actionBar.canAct && (GameTable.myTurn || actionBar.preSelectEnabled))) ? 1.0 : 0.4
                            color: allInArea.containsPress
                                 ? Qt.lighter(isShowMode ? "#2d6e2d" : Config.Theme.colorAllInBottom, 1.35)
                                 : allInArea.containsMouse
                                 ? (isShowMode ? "#3a8f3a" : Config.Theme.colorAllInTop)
                                 : (isShowMode ? "#2d6e2d" : Config.Theme.colorAllInBottom)
                            border.color: isShowMode ? "#80FF90"
                                        : allInBtn.preChecked ? "#FFD700"
                                        : Config.Theme.colorAllInEdge
                            border.width: (isShowMode || allInBtn.preChecked) ? 2 : 1
                            scale: (allInArea.pressed && ((actionBar.canAct && (GameTable.myTurn || actionBar.preSelectEnabled)) || isShowMode)) ? 0.95 : 1.0
                            Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
                            Text {
                                anchors.centerIn: parent
                                text: allInBtn.isShowMode ? qsTr("Show") : qsTr("All-In")
                                color: "#FFFFFF"
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 12
                                font.bold: true
                            }
                            MouseArea {
                                id: allInArea
                                anchors.fill: parent
                                enabled: (actionBar.canAct && (GameTable.myTurn || actionBar.preSelectEnabled)) || allInBtn.isShowMode
                                cursorShape: ((actionBar.canAct && (GameTable.myTurn || actionBar.preSelectEnabled)) || allInBtn.isShowMode) ? Qt.PointingHandCursor : Qt.ArrowCursor
                                hoverEnabled: true
                                onPressed: function(mouse) {
                                    console.log("[ACTDBG] AllIn MouseArea press",
                                                "enabled=", allInArea.enabled,
                                                "myTurn=", GameTable ? GameTable.myTurn : "n/a")
                                }
                                onClicked: {
                                    console.log("[ACTDBG] AllIn MouseArea click isShow=", allInBtn.isShowMode)
                                    if (allInBtn.isShowMode)
                                        GameTable.showMyCards()
                                    else
                                        actionBar.clickAction("allin")
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Spielmodus-Dropdown (rechts): Manuell / Auto Check/Call / Auto Check/Fold
                        ComboBox {
                            id: playingModeCombo
                            Layout.preferredWidth: 132
                            Layout.preferredHeight: actionBar.raiseRowHeight
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 11
                            model: [ qsTr("Manuell"), qsTr("Auto Check/Call"), qsTr("Auto Check/Fold") ]
                            currentIndex: actionBar.playingMode
                            onActivated: (index) => gamePage.applyPlayingMode(index)
                            // Popup nach oben öffnen – verhindert, dass er hinter
                            // der Android-Navigationsleiste verschwindet.
                            popup.y: -popup.implicitHeight

                            contentItem: Text {
                                leftPadding: 8
                                rightPadding: playingModeCombo.indicator.width + 4
                                text: playingModeCombo.displayText
                                font: playingModeCombo.font
                                color: "#FFFFFF"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                radius: 5
                                color: actionBar.playingMode === 0 ? "#222222" : "#3a2e10"
                                border.color: actionBar.playingMode === 0 ? "#3a3a3a" : Config.Theme.colorAccent
                                border.width: 1
                            }
                        }
                    }
                }

                // ── Aktions-Buttons: Fold / Check-Call / Bet-Raise ────────────────
                // Dynamische Beschriftung + Aktivierung wie im Qt-Widgets-Client.
                Item {
                    width: parent.width
                    height: actionBar.actionRowHeight

                    // Wiederverwendbarer Aktions-Button mit Verlauf, dynamischem Text und
                    // Vorwahl-Zustand (goldener Rahmen = vorgemerkt).
                    component ActionButton: Rectangle {
                        id: ab
                        property string actionKey: ""
                        property string label: ""
                        property color topColor: "#4080d8"
                        property color bottomColor: "#1a3d8b"
                        property color edgeColor: "#6aa0e8"
                        property bool armed: false   // klickbar: eigener Zug ODER Vorwahl möglich
                        property bool highlight: false   // primäre Aktion hervorheben (Raise)
                        readonly property bool myTurnNow: GameTable !== null && GameTable.myTurn
                        readonly property bool preChecked: ab.actionKey !== "" && actionBar.preAction === ab.actionKey

                        onArmedChanged: console.log("[ACTDBG] armed", ab.actionKey, "→", ab.armed,
                                                    "(myTurn=", GameTable ? GameTable.myTurn : "n/a",
                                                    "canAct=", actionBar.canAct,
                                                    "preSel=", actionBar.preSelectEnabled, ")")

                        radius: 9
                        border.width: (ab.preChecked || (ab.highlight && ab.armed)) ? 2 : 1
                        border.color: ab.preChecked ? "#FFD700" : (ab.armed ? edgeColor : "#3a3a3a")
                        opacity: !ab.armed ? 0.4 : ((ab.myTurnNow || ab.preChecked) ? 1.0 : 0.72)
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: ab.armed ? ab.topColor : "#2b2b2b" }
                            GradientStop { position: 1.0; color: ab.armed ? ab.bottomColor : "#1c1c1c" }
                        }

                        // Press-Feedback: kurzes Einsinken beim Tippen.
                        scale: (abMouse.pressed && ab.armed) ? 0.96 : 1.0
                        Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

                        // Raise als primäre Aktion mit weichem Glow hervorheben.
                        layer.enabled: ab.highlight && ab.armed
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowColor: ab.edgeColor
                            shadowOpacity: 0.55
                            shadowBlur: 0.8
                            shadowVerticalOffset: 0
                            shadowHorizontalOffset: 0
                        }

                        Text {
                            anchors.centerIn: parent
                            horizontalAlignment: Text.AlignHCenter
                            text: ab.label
                            color: "#F0F0F0"
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: actionBar.compactActions ? 12 : 15
                            font.bold: true
                            font.letterSpacing: 0.5
                            lineHeight: 0.95
                        }

                        // kleiner "vorgemerkt"-Punkt oben rechts
                        Rectangle {
                            visible: ab.preChecked
                            anchors { top: parent.top; right: parent.right; margins: 4 }
                            width: 8; height: 8; radius: 4
                            color: "#FFD700"
                        }

                        MouseArea {
                            id: abMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            enabled: ab.armed
                            cursorShape: ab.armed ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onPressed: function(mouse) {
                                console.log("[ACTDBG] MouseArea press", ab.actionKey,
                                            "armed=", ab.armed,
                                            "myTurn=", GameTable ? GameTable.myTurn : "n/a",
                                            "canAct=", GameTable ? GameTable.canAct : "n/a",
                                            "preSel=", actionBar.preSelectEnabled,
                                            "btn=", mouse.button)
                            }
                            onClicked: {
                                console.log("[ACTDBG] MouseArea click", ab.actionKey)
                                actionBar.clickAction(ab.actionKey)
                            }
                        }
                    }

                    RowLayout {
                        anchors {
                            fill: parent; leftMargin: 8; rightMargin: 8
                            topMargin: 5
                            bottomMargin: Config.Theme.compact ? 6 : 5
                        }
                        spacing: 8

                        ActionButton {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            actionKey: "fold"
                            label: actionBar.foldText
                            topColor: Config.Theme.colorFoldTop
                            bottomColor: Config.Theme.colorFoldBottom
                            edgeColor: Config.Theme.colorFoldEdge
                            // myTurnNow gatet nie den echten Zug; preSelectEnabled sperrt
                            // die Vorauswahl nach eigenem Zug/Rundenwechsel.
                            armed: myTurnNow || (actionBar.canAct && actionBar.preSelectEnabled)
                        }

                        ActionButton {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            actionKey: "call"
                            label: actionBar.checkCallText
                            topColor: Config.Theme.colorCallTop
                            bottomColor: Config.Theme.colorCallBottom
                            edgeColor: Config.Theme.colorCallEdge
                            armed: myTurnNow || (actionBar.canAct && actionBar.preSelectEnabled)
                        }

                        ActionButton {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            actionKey: "raise"
                            label: actionBar.betRaiseText
                            topColor: Config.Theme.colorRaiseTop
                            bottomColor: Config.Theme.colorRaiseBottom
                            edgeColor: Config.Theme.colorRaiseEdge
                            highlight: true     // primäre Aktion betonen
                            armed: (myTurnNow || (actionBar.canAct && actionBar.preSelectEnabled)) && actionBar.raiseAvailable
                        }
                    }
                }
            }
        }
    }

    // ── Permanenter Game-Chat: links neben der Action-Box ───────────────────
    // Direktes Kind von gamePage (über dem ColumnLayout), damit er nach oben
    // über die Action-Bar hinaus aufgezogen werden kann.
    Rectangle {
        id: dockedChat
        visible: tableZone.dockedChatFits
        z: 20
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        width: tableZone.dockedChatW
        height: {
            var h = tableZone.dockedChatUserH >= 0
                    ? tableZone.dockedChatUserH : tableZone.dockedChatMinH
            return Math.max(tableZone.dockedChatMinH,
                            Math.min(tableZone.dockedChatMaxH, h))
        }
        radius: 10
        // Bewusst transparenter als das Chat-Overlay – der Tisch bleibt
        // hinter dem permanenten Chat sichtbar.
        color: Config.Theme.withAlpha(Config.StaticData.palette.secondary.col700, 0.7)
        border.color: Config.StaticData.palette.secondary.col500
        border.width: 1

        onVisibleChanged: {
            if (visible && typeof GameTable !== "undefined" && GameTable)
                tableZone.chatReadCount = GameTable.chatLog.length
            if (!visible)
                dockedChatBox.closeEmojiPicker()
        }

        // Permanent sichtbar → neue Nachrichten gelten sofort als gelesen.
        Connections {
            target: GameTable
            function onChatLogChanged() {
                if (dockedChat.visible)
                    tableZone.chatReadCount = GameTable.chatLog.length
            }
        }

        // ── Größenänderungs-Handle (Ziehen nach oben) ────────────────────────
        Item {
            id: chatResizeHandle
            anchors.top: parent.top
            width: parent.width
            height: 10
            z: 10

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 3
                radius: 2
                color: resizeDragArea.containsMouse || resizeDragArea.pressed
                       ? Config.Theme.colorAccent
                       : Qt.rgba(1, 1, 1, 0.22)
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: resizeDragArea
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                hoverEnabled: true
                property real pressGlobalY: 0
                property real pressH: 0
                onPressed: (mouse) => {
                    pressGlobalY = mapToItem(gamePage, mouse.x, mouse.y).y
                    pressH = dockedChat.height
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var curY = mapToItem(gamePage, mouse.x, mouse.y).y
                    var delta = pressGlobalY - curY   // nach oben = positiv
                    var newH = Math.max(tableZone.dockedChatMinH,
                                       Math.min(tableZone.dockedChatMaxH,
                                                pressH + delta))
                    tableZone.dockedChatUserH = newH
                }
            }
        }

        ChatBox {
            id: dockedChatBox
            anchors.fill: parent
            anchors.margins: 6
            anchors.topMargin: 12   // Platz für den Resize-Handle
            chatModel: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog : []
            nickList: gamePage.gameNickList()
            messageFontSize: 11
            inputHeight: 28
            // Wenig Platz → Picker als Popup über der Box.
            emojiPickerAsPopup: true
            onSendRequested: (text) => {
                if (typeof GameTable !== "undefined" && GameTable)
                    GameTable.sendChat(text)
            }
        }
    }

    // ── Lupe-Button ──────────────────────────────────────────────────────────
    // Direktes Kind von gamePage (nicht tableZone), damit der Button im
    // Landscape-Modus am unteren Bildschirmrand erscheint – also neben der
    // Action-Box, nicht über ihr. z:200 legt ihn über alle ColumnLayout-Elemente.
    // Kein layer.enabled/Shadow auf dem Rectangle – vermeidet Interferenz
    // zwischen verschachtelten MultiEffects, die die Icon-Kolorierung bricht.
    Rectangle {
        id: zoomToggle
        visible: Qt.platform.os === "android" && Config.Responsive.compact && Config.Parameters.tableZoomEnabled
        z: 200
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        // Portrait: 8 px über Unterkante Action-Bar (= 8 px über tableZone.bottom)
        // Landscape: 8 px vom echten Bildschirmrand (= neben der schmalen Action-Box)
        anchors.bottomMargin: tableZone.wide ? 8 : (8 + actionBar.height)
        width: 36; height: 36; radius: 18
        color: tableZone.zoomActive ? Config.Theme.colorAccent : Qt.rgba(0, 0, 0, 0.50)

        onVisibleChanged: {
            if (!visible) {
                tableZone.zoomActive = false
                tableZone._zoomPanX = 0
                tableZone._zoomPanY = 0
            }
        }

        VectorImage {
            anchors.centerIn: parent
            width: 22; height: 22
            source: tableZone.zoomActive ? "../resources/zoomOut.svg" : "../resources/zoomIn.svg"
            layer.enabled: true
            layer.effect: MultiEffect {
                colorization: 1.0
                colorizationColor: tableZone.zoomActive ? "#101010" : "#FFFFFF"
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                tableZone.zoomActive = !tableZone.zoomActive
                if (!tableZone.zoomActive) {
                    tableZone._zoomPanX = 0
                    tableZone._zoomPanY = 0
                }
            }
        }
    }
}
