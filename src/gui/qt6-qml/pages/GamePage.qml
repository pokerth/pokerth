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

    // Spielmodus umschalten – die eigentliche Logik (inkl. verzögerter
    // Auto-Aktion) lebt in der GameActionBar; hier nur als Weiterleitung für
    // die Tastatur-Shortcuts.
    function applyPlayingMode(index) {
        if (actionBar)
            actionBar.applyPlayingMode(index)
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
        // Im landscapeCompact knapper (28 statt 40) — schafft ~12 px mehr
        // tableZone-Höhe für die Halsketten-Ellipse.
        GameStatusBar {
            Layout.fillWidth: true
            Layout.preferredHeight: Config.Responsive.landscapeCompact ? 28 : 40
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

                // Box-Skala-Obergrenze wächst mit der Spielerzahl: bei wenigen
                // Spielern sollen die Boxen NICHT den ganzen leeren Tisch
                // ausfüllen. Ohne diesen Deckel sprang die Skala bei z. B. nur
                // zwei Spielern auf das Maximum (1.4 bzw. 1.7) – Self- und
                // Gegnerbox wirkten viel zu groß und ihre Bet-Badges berührten
                // die Gemeinschaftskarten. Ab 6 Gegnern (7+ Spielern) wird die
                // volle Obergrenze erreicht, die dicht besetzten Tische bleiben
                // daher unverändert. Der Deckel SENKT nur (nie erhöht) und kann
                // somit keine neuen Überlappungen erzeugen.
                function fillCap(maxScale) {
                    var base = 0.95
                    var t = Math.max(0, Math.min(1, (oppCnt - 1) / 5))
                    var countCap = base + (maxScale - base) * t
                    // Wenige Spieler bei großen (maximierten/Vollbild-) Fenstern
                    // mitwachsen lassen: ohne dies bleibt boxScale gedeckelt,
                    // während die Ellipse (Bruchteil der Breite) aufspreizt → im
                    // Vollbild winzige Boxen am Rand + leere Mitte. Wächst NUR
                    // oberhalb der gewohnten Fenstergröße (~760px Tischzonenhöhe)
                    // und nur bei wenigen Spielern (1-t); gedeckelt bei 2.2, damit
                    // Boxen/Schrift nicht grotesk groß werden. Dichte Tische und
                    // die Startauflösung bleiben unverändert.
                    var grow = (1 - t) * Math.max(0, (height - 760) / 700)
                    return Math.min(2.2, countCap * (1 + grow))
                }

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
                    // Muss mit buildLandscapeSlots().selfWeight übereinstimmen.
                    var selfWeightCap = Config.Responsive.landscapeCompact ? 0.5 : 0.3
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
                        // Community-Karten werden mittig in die Lücke zwischen der
                        // Unterkante der obersten Gegnerbox und der Oberkante der
                        // Self-Box gelegt (siehe communityCenterY). Diese Lücke muss
                        // groß genug sein, damit Kartenreihe + Pott-Badge hinein-
                        // passen – sonst boxScale verkleinern. Nur reguläres Wide;
                        // im landscapeCompact sitzt die Community per eigener Formel.
                        if (!Config.Responsive.landscapeCompact) {
                            // Tiefste Unterkante der oberen Sitze – die zentrale
                            // Top-Box zählt zusätzlich ihre nach unten zeigende
                            // Bet-Badge mit (sonst ragt sie in die Kartenreihe).
                            var topOppBottom = -1e9
                            for (var iC = 0; iC < oppCnt; iC++) {
                                var vC = slotVec(firstAngle + iC * stepDeg)
                                if (vC[1] >= 0) continue          // nur obere Sitze
                                var b = centerYpix + radiusYpix * vC[1] + visualH / 2
                                      + (Math.abs(vC[0]) < 0.25 ? sTest * 25 : 0)
                                if (b > topOppBottom) topOppBottom = b
                            }
                            // Community-Gesamthöhe (Kartenreihe 64 + Pott-Badge 40 +
                            // Winner-Badge 20) · communityScale + Pad.
                            if (topOppBottom > -1e9
                                && (height - 12 - selfVisualH) - topOppBottom
                                   < 0.95 * sTest * 124 + 28)
                                return false
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

                    // Heads-up (1 Gegner): feasibleAt() hat kein Nachbar-Paar zu
                    // prüfen. Stattdessen sicherstellen, dass die mittig in die
                    // Lücke zwischen oben-zentrierter Gegnerbox (inkl. nach unten
                    // zeigender Bet-Badge) und Self-Box gelegte Community-Reihe
                    // hineinpasst – kritisch in flachen Fenstern. Gleiche Logik
                    // wie die Community-Prüfung in feasibleAt().
                    function feasibleHeadsUp(sTest) {
                        if (sTest <= 0) return false
                        var visualH = oppBaseHeight * sTest
                        var selfVisualH = selfBaseHeight * sTest
                        var topYband = (Config.Responsive.landscapeCompact ? 0 : 4) + visualH / 2
                        var topOppBottom = topYband + visualH / 2 + sTest * 25
                        return (height - 12 - selfVisualH) - topOppBottom
                               >= 0.95 * sTest * 124 + 28
                    }

                    // Gemeinsames Limit für Gegnerboxen, Self-Box und Community-Badges:
                    // 1.4 verhindert zu große Schrift und Bet-Überlappungen bei
                    // Vollbild/maximiert; compact bleibt bei 1.7 (breiter, flacher).
                    // fillCap() dämpft das Maximum bei wenigen Spielern zusätzlich.
                    var lo = 0.55, hi = fillCap(Config.Responsive.landscapeCompact ? 1.7 : 1.4)
                    if (oppCnt < 2) {
                        // Bis zum (gedeckelten) hi gehen, solange die Badges die
                        // Community nicht berühren.
                        if (!feasibleHeadsUp(lo)) {
                            s = lo
                        } else if (feasibleHeadsUp(hi)) {
                            s = hi
                        } else {
                            for (var iterH = 0; iterH < 14; iterH++) {
                                var midH = (lo + hi) / 2
                                if (feasibleHeadsUp(midH)) lo = midH
                                else hi = midH
                            }
                            s = lo
                        }
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

                    var loP = 0.55, hiP = fillCap(1.85)
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
                // beansprucht. Kleiner = Gegner rücken näher an die Self / weiter
                // zur Boden-Mitte und damit tiefer (mehr sin) → der Ring schließt
                // sich enger um die Self-Box, sie hebt sich nur noch minimal ab
                // (User-Wunsch). Reguläres Wide: 0.3; im landscapeCompact bleibt
                // 0.5 erhalten (eigenes, separat abgestimmtes Layout). Muss mit
                // `selfWeightCap` in der feasibleAt-Probe identisch sein.
                //
                // Disconnectet ein Spieler, ändert sich N → automatische,
                // saubere Re-Verteilung über die unten generierten Winkel.
                var opps = Math.max(1, seatCount - 1)
                var selfWeight = Config.Responsive.landscapeCompact ? 0.5 : 0.3
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
            // Community-Karten Y-Position:
            //   – Reguläres Wide: vertikaler SCHWERPUNKT aller Boxen (Gegner +
            //     Self). Die früher genutzte Mitte (topOpp+self)/2 ist spieler-
            //     zahl-unabhängig (~Tischmitte) und ließ die Karten bei wenigen
            //     Spielern ÜBER den tief sitzenden Seitenboxen schweben (z. B. bei
            //     4 Spielern: ein einsamer Top-Spieler oben, zwei Seitenspieler
            //     weiter unten). Der Schwerpunkt zählt diese tieferen Boxen mit →
            //     die Karten rücken in den Pulk und wirken mittig.
            //   – landscapeCompact / Portrait: weiterhin (topOpp+self)/2
            //     (eigenes, separat abgestimmtes Layout).
            readonly property real communityCenterY: {
                if (!wide || Config.Responsive.landscapeCompact)
                    return (topOpponentBottomY + selfVisualTopY) / 2
                var sumY = height - 12 - selfBaseHeight * boxScale / 2   // Self-Box-Mitte
                var n = 1
                var seq = slotSeq[seatCount - 1] || []
                for (var i = 0; i < seq.length; ++i) {
                    var p = slotPos[seq[i]]
                    if (p) { sumY += p[1] * height; n++ }
                }
                return sumY / n
            }

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
            // Position: Portrait mittig zwischen den oberen/unteren Seiten-Boxen,
            // Widescreen im Mittelpunkt der Halsketten-Ellipse. Größe = nur die
            // Kartenreihe, damit das Winning-Hand-Badge die Zentrierung nicht stört.
            CommunityCards {
                id: communityArea
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: tableZone.wide
                    ? tableZone.communityCenterY - tableZone.height / 2
                    : -tableZone.height * 0.0025 + 5
                z: 0
                wide: tableZone.wide
                // Skaliert dezenter als die Gegner-Boxen; Skalierung um die Mitte.
                scale: tableZone.communityScale
            }

            // Gewinner-Hand (z.B. "Full House") – nur während des Showdowns.
            // Bewusst eigenständig (NICHT in den Community-Cards), damit es
            // unabhängig von deren z/Scale immer ÜBER den Spielerboxen liegt.
            WinningHandBadge {
                community: communityArea
                wide: tableZone.wide
                communityScale: tableZone.communityScale
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

            GameSidePanel {
                id: logOverlay
                z: 150
                edge: Qt.RightEdge
                wide: tableZone.wide
                title: qsTr("Spielverlauf")
                visible: tableZone.showLog
                onCloseRequested: gamePage.toggleLogOverlay()

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
                    // Nach Ablauf des Timers springt es wieder ans Ende.
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

            GameRoundIconButton {
                id: logToggle
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                iconSource: "../resources/gameLog.svg"
                active: tableZone.showLog
                onClicked: gamePage.toggleLogOverlay()
            }

            // ── Chat-Overlay (nur bei menschlichen Mitspielern) ────────────────
            GameSidePanel {
                id: chatOverlay
                z: 150
                edge: Qt.LeftEdge
                wide: tableZone.wide
                title: qsTr("Chat")
                visible: tableZone.showChat
                onCloseRequested: gamePage.toggleChatOverlay()
                // Chat geschlossen → Emoji-Picker mitschließen.
                onVisibleChanged: if (!visible) overlayChat.closeEmojiPicker()

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

            GameRoundIconButton {
                id: chatToggle
                z: 200
                // Ausgeblendet, wenn der Chat permanent unten links gedockt ist.
                visible: ((typeof GameTable !== "undefined" && GameTable) ? GameTable.hasHumanOpponents : false)
                         && !tableZone.dockedChatFits
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 8
                iconSource: "../resources/gameChat.svg"
                active: tableZone.showChat
                unread: tableZone.chatUnread
                onClicked: gamePage.toggleChatOverlay()
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
            ReactionPicker {
                visible: tableZone.showReactions && gamePage.emojiReactionsEnabled
                z: 210
                anchors.top: reactionToggle.bottom
                anchors.topMargin: 6
                anchors.left: parent.left
                anchors.leftMargin: 8
                onPicked: (emoji) => gamePage.sendReaction(emoji)
            }

        }

        // 3. Action-Leiste: Raise-Controls + Fold / Call / Raise
        GameActionBar {
            id: actionBar
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            wide: tableZone.wide
            communityVisualWidth: communityArea.width * communityArea.scale
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
