import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../components"
import "../config" as Config

Rectangle {
    id: gamePage
    objectName: "gamePage"
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    color: "transparent"

    // ── Tisch-Theme-Farben für Chat-/Log-Box ──────────────────────────────────
    // Fest (dunkel), unabhängig vom Hell/Dunkel-Modus der übrigen App. Quelle ist
    // das Tisch-Theme (StyleProvider.chatLog*, per XML überschreibbar); der
    // Fallback greift nur, falls die Context-Property mal nicht gesetzt ist.
    readonly property color tblChatBackground:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBackground : "#1d222b"
    readonly property color tblChatSurface:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogSurface : "#394150"
    readonly property color tblChatBorder:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBorder : "#576378"
    readonly property color tblChatText:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogText : "#eff1f5"
    readonly property color tblChatTextSecondary:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextSecondary : "#cdd3e0"
    readonly property color tblChatTextMuted:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextMuted : "#7787a3"

    // Spielmodus umschalten – die eigentliche Logik (inkl. verzögerter
    // Auto-Aktion) lebt in der GameActionBar; hier nur als Weiterleitung für
    // die Tastatur-Shortcuts.
    function applyPlayingMode(index) {
        if (actionBar)
            actionBar.applyPlayingMode(index)
    }

    // Zuschauer: weder Tischchat noch Verlauf/Chancen. Auch die Tastenkürzel
    // müssen aus – sonst öffnete Alt+C/Alt+L/Alt+I ein Overlay, dessen
    // Schließen-Button (der Toggle) gar nicht mehr da ist.
    readonly property bool spectating: tableZone ? tableZone.spectating : false

    // ── Info-Panel (Verlauf/Chancen/Blatt) ────────────────────────────────────
    // Desktop: permanent unten RECHTS angedockt (gespiegelt zum Chat unten links),
    // ÜBER dem Tisch schwebend (reserviert keinen Platz). Reicht die Breite nicht
    // (bzw. Mobil), bleibt es beim schwebenden Overlay mit Toggle. Der Toggle
    // klappt das angedockte Panel ein/aus (dockedInfoCollapsed).
    readonly property bool infoDocked: tableZone ? tableZone.dockedInfoFits : false
    // Sichtbarkeit/Aktiv-Zustand des Toggles je nach Modus.
    readonly property bool infoPanelOpen:
        infoDocked ? !tableZone.dockedInfoCollapsed
                   : (tableZone ? tableZone.showInfo : false)

    // Panel öffnen bzw. Tab wählen (Shortcuts).
    function showInfoTab(idx) {
        if (gamePage.infoDocked) {
            tableZone.dockedInfoCollapsed = false
            infoPanelDock.currentIndex = idx
        } else {
            tableZone.showInfo = true
            infoPanelOverlay.currentIndex = idx
            if (!tableZone.wide)
                tableZone.showChat = false
        }
    }

    // Combined-Toggle: Panel ein-/ausblenden (angedockt: einklappen).
    function toggleInfoOverlay() {
        if (gamePage.infoDocked) {
            tableZone.dockedInfoCollapsed = !tableZone.dockedInfoCollapsed
            return
        }
        tableZone.showInfo = !tableZone.showInfo
        if (tableZone.showInfo && !tableZone.wide)
            tableZone.showChat = false
    }

    // Alt+L (Verlauf-Tab) – Name aus Kompatibilität zur bisherigen Belegung.
    // Toggle: liegt der Verlauf-Tab bereits offen, schließt der Shortcut das
    // Panel wieder – angedockt (Desktop) genauso wie als Overlay.
    function toggleLogOverlay() {
        if (gamePage.infoDocked) {
            if (!tableZone.dockedInfoCollapsed && infoPanelDock.currentIndex === 0)
                tableZone.dockedInfoCollapsed = true
            else
                showInfoTab(0)
            return
        }
        if (tableZone.showInfo && infoPanelOverlay.currentIndex === 0)
            tableZone.showInfo = false
        else
            showInfoTab(0)
    }

    // Chat analog zum Info-Panel: Desktop → permanent unten links angedockt
    // (chatDocked), per Toggle einklappbar; sonst schwebendes Overlay.
    readonly property bool chatDocked: tableZone ? tableZone.dockedChatFits : false
    readonly property bool chatPanelOpen:
        chatDocked ? !tableZone.dockedChatCollapsed
                   : (tableZone ? tableZone.showChat : false)

    function toggleChatOverlay() {
        if (!tableZone)
            return
        if (gamePage.chatDocked) {
            tableZone.dockedChatCollapsed = !tableZone.dockedChatCollapsed
            return
        }
        tableZone.showChat = !tableZone.showChat
        if (tableZone.showChat && !tableZone.wide)
            tableZone.showInfo = false
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
    // Zusätzlich sind Reaktionen nur in Netzwerkspielen sinnvoll, in denen
    // menschliche Gegner mitspielen – in einem Localgame (nur Computer-Gegner)
    // wird der Toggle daher ausgeblendet.
    property bool emojiReactionsEnabled: true
    function refreshEmojiReactionsEnabled() {
        var enabledInSettings = SettingsManager
            ? SettingsManager.readConfigInt("DisableEmojiReactions") === 0 : true
        var isLocalGame = (typeof GameTable !== "undefined" && GameTable
            && GameTable.isLocalGameRunning())
        emojiReactionsEnabled = enabledInSettings && !isLocalGame
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
        // Sitz 0 sitzt in der Self-Box – außer als Zuschauer, dann ist er ein
        // gewöhnlicher Ring-Sitz und wird über slotForSeat() gefunden.
        if (seatIdx <= 0 && !tableZone.spectating) {
            px = selfBox.x + selfBox.width / 2
            py = selfBox.y + selfBox.height / 2
                 - (selfBox.height * tableZone.boxScale) / 2 - 6
        } else {
            var slot = tableZone.slotForSeat(seatIdx)
            if (!slot) return
            px = tableZone.width * slot.x + (slot.nudgeX || 0)
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
            // console.log("[REACT] received from", playerName, "->", emoji)
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
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
        ? SettingsManager.readConfigInt("AlternateFKeysUserActionMode") !== 0 : false

    function fKeyAction(which) {
        if (actionBar)
            actionBar.clickAction(which)
    }

    // Diese GamePage ist die oberste im Stack – auch dann, wenn eine ANDERE Seite
    // darüber liegt (Tisch-/Spieler-Statistik, Einstellungen, Community-Seiten).
    // Genau dafür gibt es die Property: `visible` ist in dem Fall false, der
    // Zug-Timer des Servers läuft aber weiter. Die Aktions-Tasten müssen dann
    // erreichbar bleiben, sonst sind sie – aus Sicht des Spielers grundlos –
    // stumm, bis er zufällig zum Tisch zurücknavigiert.
    // Re-Eval bei jeder Navigation über depth/currentItem (Muster wie
    // mainWindow.inLobbySession); find() sucht von oben, liefert also die
    // zuletzt gepushte GamePage. Damit ist auch bei einer (durch die Push-Guards
    // eigentlich verhinderten) zweiten GamePage im Stack immer nur EIN
    // Shortcut-Satz aktiv – zwei aktive Shortcuts mit derselben Sequenz feuern
    // in Qt gar nicht mehr, sondern nur noch activatedAmbiguously.
    readonly property bool topGamePage: {
        var view = gamePage.StackView.view
        if (!view)
            return false
        var _d = view.depth
        var _c = view.currentItem
        return view.find(function(it) { return it && it.objectName === "gamePage" }) === gamePage
    }

    // Diagnose für genau diesen Fall: feuert der Shortcut mehrdeutig, wird die
    // Aktion still verworfen. Ohne Log wäre das vom Spieler nicht von einer
    // "kaputten" Taste zu unterscheiden.
    function ambiguousShortcut(seq) {
        console.warn("[SHORTCUT] '" + seq + "' ist mehrdeutig (zwei aktive Shortcuts "
                     + "mit derselben Sequenz) – Aktion NICHT ausgeführt.")
    }

    Shortcut {
        sequence: "Alt+L"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.toggleLogOverlay()
    }
    Shortcut {
        sequence: "Alt+C"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.toggleChatOverlay()
    }
    Shortcut {
        sequence: "Alt+I"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.toggleInfoOverlay()
    }
    Shortcut {
        sequence: "Alt+F"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(2)
    }
    Shortcut {
        sequence: "Alt+M"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(0)
    }
    Shortcut {
        sequence: "Alt+K"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(1)
    }
    // F11 (Vollbild) liegt bewusst NICHT hier, sondern am ApplicationWindow
    // (pokerth.qml) – es soll auf jeder Seite wirken, nicht nur am Tisch.

    // ── Gametable-Actions: F-Tasten (siehe fKeysAlternate oben) ──
    // Für Zuschauer aus: die Action-Bar ist dann ausgeblendet (actionBar.visible),
    // ihre Bedienelemente wären also per Tastatur erreichbar, per Maus nicht.
    // F11 (Vollbild) bleibt bewusst auch für Zuschauer aktiv.
    //
    // Bedingung ist topGamePage (NICHT visible): F1–F5 greifen ins laufende Spiel
    // ein und müssen auch dann funktionieren, wenn gerade eine andere Seite über
    // dem Tisch liegt – der Zug-Timer läuft dort weiter. Die Panel-Toggles und die
    // Spielmodus-Tasten unten bleiben dagegen an visible, sie bedienen nur die
    // Oberfläche des Tisches.
    Shortcut {
        sequence: "F1"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "allin" : "fold")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F1")
    }
    Shortcut {
        sequence: "F2"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "raise" : "call")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F2")
    }
    Shortcut {
        sequence: "F3"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "call" : "raise")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F3")
    }
    Shortcut {
        sequence: "F4"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: gamePage.fKeyAction(gamePage.fKeysAlternate ? "fold" : "allin")
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F4")
    }
    Shortcut {
        sequence: "F5"
        context: Qt.ApplicationShortcut
        enabled: gamePage.topGamePage && !gamePage.spectating
        onActivated: if (GameTable && GameTable.canShowCards) GameTable.showMyCards()
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F5")
    }
    Shortcut {
        sequence: "F6"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(0)   // Manuell
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F6")
    }
    Shortcut {
        sequence: "F7"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(2)   // Auto Check/Fold
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F7")
    }
    Shortcut {
        sequence: "F8"
        context: Qt.ApplicationShortcut
        enabled: gamePage.visible && !gamePage.spectating
        onActivated: gamePage.applyPlayingMode(1)   // Auto Check/Call
        onActivatedAmbiguously: gamePage.ambiguousShortcut("F8")
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
        // z über der tableZone (nächstes ColumnLayout-Element, das sonst darüber
        // gezeichnet wird): die obere-Mitte-Spielerbox (TC-Slot) ragt in den
        // Streifen der Leiste hinein und würde sonst mit ihrer MouseArea den
        // Klick auf den (anklickbaren) Tischnamen abfangen.
        GameStatusBar {
            z: 1
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
            // Clip-Container für das Tisch-Hintergrundbild.
            //   • OBEN bei der tableZone-Kante clippen → das (im center-Modus) zum
            //     Cover hochskalierte Bild kann NICHT über die obere Kante hinaus
            //     die Status-/Navigationsleiste überdecken.
            //   • UNTEN bis zum Fensterrand ziehen: die Action-Box ist das dritte
            //     ColumnLayout-Element und nimmt unten Platz weg, sodass tableZone
            //     an ihrer Oberkante endet. Der Container reicht daher um die
            //     Action-Box-Höhe weiter nach unten (tableZone clippt selbst nicht),
            //     damit die Tischgrafik HINTER der Action-Box bis zum unteren
            //     Fensterrand sichtbar bleibt (kein dunkler Streifen).
            // Spieler-Boxen/Badges bleiben direkte tableZone-Kinder und dürfen
            // weiterhin über den Rand hinausragen.
            Item {
                id: tableBgClip
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: tableZone.height + (tableZone.wide ? actionBar.height : 0)
                clip: true

                Image {
                    id: tableBackgroundImage
                    // Wir verwenden die Bild-Quelle als implizite Größe (implicitWidth/Height)
                    source: (typeof StyleProvider !== "undefined" && StyleProvider && StyleProvider.tableBackground !== "")
                            ? StyleProvider.tableBackground : "../resources/tableGreen.png"
                    fillMode: Image.PreserveAspectCrop
                    smooth: true

                    // ── center-Modus (z. B. danuxi) ───────────────────────────────
                    // Das Bild füllt den GESAMTEN Gametable randlos (bis zum unteren
                    // Fensterrand hinter der Action-Box) und der Tisch sitzt im
                    // Zentrum der Player-Box-Ellipse (communityCenterY). Damit das
                    // Bild auch bei außermittiger Ellipse die komplette Zone deckt,
                    // wird so weit hochskaliert, dass es – zentriert auf (Zonenmitte,
                    // Ellipsenmittelpunkt) – ALLE vier Kanten überdeckt (stärkeres,
                    // symmetrisches Beschneiden des äußeren Randes = "mehr crop").
                    // Ohne center-Stil: klassisches Cover.
                    // Bezugsgröße ist bewusst tableZone (NICHT parent/tableBgClip –
                    // dessen Höhe ist um die Action-Box-Höhe größer).
                    readonly property bool centerMode:
                        typeof StyleProvider !== "undefined" && StyleProvider
                        && StyleProvider.tableBackgroundAlignment === "center"
                    readonly property real srcW: Math.max(1, implicitWidth  || tableZone.width)
                    readonly property real srcH: Math.max(1, implicitHeight || tableZone.height)
                    // Zusatzhöhe für den Bereich HINTER der Action-Leiste. Ist sie
                    // ausgeblendet (Zuschauer), gibt es nichts zu überdecken – sonst
                    // schöbe der Zuschlag die Tischgrafik nach unten aus der Mitte.
                    readonly property real coverExtra:
                        (tableZone.wide && actionBar.visible) ? actionBar.height : 0
                    // Vertikaler Mittelpunkt der Player-Box-Ellipse (Zonen-Koords).
                    readonly property real ellipseCenterY: tableZone.communityCenterY
                    // Crop-/Zoom-Faktor aus dem Stil (>= 1.0, default 1.0): größer =
                    // Tisch größer / mehr Beschnitt des äußeren Randes. Per Daten
                    // (XML <TableBackgroundZoom>) justierbar – kein Neubau nötig.
                    readonly property real centerZoom:
                        centerMode && StyleProvider.tableBackgroundZoom > 0
                        ? Math.max(1.0, StyleProvider.tableBackgroundZoom) : 1.0
                    // Skalierung, die – zentriert auf (Zonenmitte, ellipseCenterY) –
                    // die ganze Zone (inkl. Bereich hinter der Action-Box) deckt,
                    // multipliziert mit dem Zoom-/Crop-Faktor.
                    readonly property real fillScale: {
                        var reqH = 2 * Math.max(ellipseCenterY,
                                                tableZone.height + coverExtra - ellipseCenterY)
                        return Math.max(tableZone.width / srcW, reqH / srcH) * centerZoom
                    }

                    width:  centerMode ? Math.round(srcW * fillScale) : tableZone.width
                    height: centerMode ? Math.round(srcH * fillScale)
                                       : tableZone.height + coverExtra
                    // Horizontal auf Zonenmitte, vertikal auf den Ellipsenmittelpunkt
                    // zentrieren → der Tisch liegt in der Bildmitte = Ellipsenmitte.
                    x: centerMode ? Math.round(tableZone.width / 2 - width / 2) : 0
                    y: centerMode ? Math.round(ellipseCenterY - height / 2) : 0
                }
            }

            // Platzhalter-Modus: Sitze von Spielern, die den Tisch verlassen haben
            // (Disconnect/Kick/Verlassen/Ausgeschieden – vom GameHandler mit
            // `reserved` markiert), bleiben Teil des Rings. Sie werden nur nicht
            // gezeichnet, belegen ihren Slot aber weiter → die Ellipse wird NICHT
            // neu verteilt und die verbleibenden Boxen bleiben, wo sie sind.
            readonly property bool keepEmptySeats: Config.Parameters.keepEmptySeats

            // Einziges Kriterium dafür, ob ein Sitz einen Ring-Slot beansprucht.
            // Muss überall gelten, wo Slots gezählt/zugeordnet werden (seatCount,
            // slotForSeat, oppOrder im Delegate) – sonst driften Reihenfolge und
            // Slot-Zahl auseinander.
            function seatOnRing(p) {
                if (!p) return false
                return p.name !== "" || (keepEmptySeats && p.reserved === true)
            }

            // Anzahl der Sitze, die einen Platz im Ring beanspruchen
            readonly property int seatCount: {
                if (typeof GameTable === "undefined" || !GameTable) return 1
                var c = 0
                for (var i = 0; i < GameTable.players.length; i++)
                    if (seatOnRing(GameTable.players[i])) c++
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

            // Zuschauer-Modus: kein eigener Sitz. Der Platz unten in der Mitte,
            // an dem sonst die (größere) Self-Box klebt, wird zu einem ganz
            // normalen Ring-Sitz für Sitz 0 – alle Boxen sind gleich groß und
            // gleichmäßig über die Ellipse verteilt.
            readonly property bool spectating:
                (typeof GameTable !== "undefined" && GameTable) ? GameTable.spectating : false

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
            //
            // Sitz-Stil "inset" (Config.SeatStyle): der Einsatz steht im Sockel
            // INNERHALB der Box – die Box wird dafür um betStripH höher. Diese
            // Zusatzhöhe steckt bewusst in oppBaseHeight/selfBaseHeight, damit
            // die komplette Platz-Bisektion unten (boxScale, Ellipsen-Radien,
            // Community-Skala, Chat-/Info-Dock) automatisch mit der größeren Box
            // rechnet und keine neuen Überlappungen entstehen. Die daraus
            // ABGELEITETEN Maße (Boxbreite, Karten-/Avatarhöhe) rechnen sie
            // wieder heraus – die Box wächst nur in der Höhe.
            readonly property int betStripH: Config.SeatStyle.betStripExtra
            readonly property int oppBaseHeight: (wide ? 84 : 71) + betStripH
            // Breite dynamisch: 2×hMargin + AvatarCardRow.implicitWidth.
            // AvatarCardRow: avatarH + gap(4) + 2·cardW + cardSpacing(4)
            //   Landscape: topRow=40, cardW=29 → 2×4 + 40 + 4 + 2×29 + 4 = 114
            //   Portrait : topRow=43, cardW=31 → 2×4 + 43 + 4 + 2×31 + 4 = 121
            readonly property int oppBaseWidth: {
                var rowH = oppBaseHeight - (wide ? 44 : 28) - betStripH
                var cw   = Math.round(rowH * 120 / 168)
                return 2 * 4 + rowH + 4 + 2 * cw + 4
            }
            // Self-Box bewusst GRÖSSER als die Gegnerboxen (eigene Box prominent) —
            // in JEDEM Modus: Desktop-Wide 96, Kompakt-Landscape 94, Portrait 82
            // (Gegnerboxen: 84 bzw. 71). Die Bisektion leitet selfVisualH/bottomY/
            // selfClearX/selfVisualTopY aus selfBaseHeight ab → reserviert automatisch
            // mehr Platz (Gegner-Bottom-Sitze rücken hoch), daher überlappungsfrei.
            // Als Zuschauer sitzt unten ein gewöhnlicher Sitz: gleiche Maße wie die
            // Gegnerboxen, damit der Ring gleichmäßig wirkt.
            readonly property int selfBaseHeight:
                spectating ? oppBaseHeight
                           : ((!wide ? 82 : (Config.Responsive.landscapeCompact ? 94 : 96))
                              + betStripH)
            // Self-Box-Breite dynamisch: identische Abstände wie Gegnerboxen.
            //   Compact  : cardsH=46, cardW=33, avW=46 → 2×4 + 46 + 4 + 2×33 + 4 = 128
            //   Landscape: cardsH=40, cardW=29, avW=40 → 2×4 + 40 + 4 + 2×29 + 4 = 114
            //   Portrait : cardsH=41, cardW=29, avW=41 → 2×4 + 41 + 4 + 2×29 + 4 = 115
            readonly property int selfBaseWidth: {
                if (spectating) return oppBaseWidth
                var cH  = selfBaseHeight - betStripH - 12 - (Config.Responsive.landscape ? 32 : 18)
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
                // Sitze auf dem Ring (als Zuschauer inkl. Sitz 0), s. ringCount.
                var ringCnt = spectating ? _peakSeatCount : oppCnt
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
                    // oberhalb der gewohnten Fenstergröße (√(w·h) ≈ 760 bei
                    // 1024×521) und nur bei wenigen Spielern (1-t); gedeckelt
                    // bei 2.2, damit Boxen/Schrift nicht grotesk groß werden.
                    // √(width·height) statt nur height, weil das Problem im
                    // Fullscreen-Querformat vor allem durch die BREITE entsteht:
                    // bei 1920×960 ist √ ≈ 1358 statt 960 → grow dreimal stärker
                    // → boxScale ~1.75 statt ~1.28 → Boxen füllen proportional
                    // zum Bildschirm. Dichte Tische und die Startauflösung bleiben
                    // unverändert.
                    var grow = (1 - t) * Math.max(0, (Math.sqrt(width * height) - 760) / 700)
                    // Voll besetzte Tische (t≈1) bei breiten Fenstern: die
                    // Bottom-Seiten-Sitze (Player 1/N) bekommen durch den größer
                    // werdenden radiusX mehr Abstand zu ihren oberen Nachbarn –
                    // feasibleAt() erlaubt dadurch deutlich höhere boxScale als
                    // bei Normalfenster. Bei hohen Einsatzbeträgen könnte das
                    // Badge von Player 1/N in die Nachbarbox ragen. Linear ab
                    // 1024 px, maximal –0.15 (Deckel ≥ 1.25 für 9+ Spieler).
                    var denseShrink = t * Math.min(0.15, Math.max(0, (width - 1024) / 4000))
                    return Math.min(2.2, countCap * (1 + grow) - denseShrink)
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
                    // Reiner Sicherheits-Abstand zwischen benachbarten Box-Paaren
                    // in der Bisektions-Probe. Kleiner = Boxen dürfen enger packen
                    // = höheres feasibles boxScale (größere Boxen/Karten/Schrift).
                    // Im landscapeCompact (kleine Phone-Landscape-Auflösungen wie
                    // iPhone mini, Höhe < 600) deutlich großzügiger: dort standen die
                    // oberen/seitlichen Gegner-Paare (Player 2↔3 / 7↔8) so eng, dass
                    // sie sich visuell berührten. Mehr Sicherheits-Abstand → die
                    // Bisektion cappt boxScale niedriger → kleinere, überlappungs-
                    // freie Boxen.
                    var gap = Config.Responsive.landscapeCompact ? 12 : 4
                    // Muss mit buildLandscapeSlots().selfWeight übereinstimmen.
                    // Zuschauer: Sitz 0 ist ein normaler Ring-Sitz (Gewicht 1.0)
                    // → gleichmäßige Verteilung über die volle Ellipse.
                    var selfWeightCap = spectating ? 1.0
                                      : (Config.Responsive.landscapeCompact ? 0.5 : 0.3)
                    var stepDeg = oppCnt >= 1 ? 360 / (oppCnt + selfWeightCap) : 360
                    var firstAngle = 90 + (selfWeightCap * stepDeg + stepDeg) / 2

                    function feasibleAt(sTest) {
                        if (oppCnt < 2) return true
                        var sideMargin = Math.max(18, width * 0.025) + sideBadgeGapBase * sTest
                        var visualW = oppBaseWidth * sTest
                        var visualH = oppBaseHeight * sTest
                        // Zuschauer: unterhalb der Ellipse liegt keine Self-Box,
                        // der unterste Sitz IST der Bodenpunkt der Ellipse.
                        var selfVisualH = spectating ? 0 : selfBaseHeight * sTest
                        var gapY = Math.max(8, opponentGapBase * sTest)
                        // Im landscapeCompact ziehen wir die untere Ellipsen-
                        // Hälfte näher an die Self-Box: das verschafft den
                        // Seiten-Paaren mehr vertikalen Spielraum (sonst kleben
                        // Player 7↔8 / 2↔3 visuell zusammen). selfBadgeGapBase
                        // bleibt als Minimum, damit Bet-/Action-Badges
                        // unterhalb der Bottom-Reihe nicht ins Self-Avatar
                        // hineinragen.
                        var selfGapY = spectating ? 0
                            : (Config.Responsive.landscapeCompact
                               ? Math.max(8, selfBadgeGapBase * sTest * 0.5)
                               : selfBadgeGapBase * sTest)
                        // radiusX nach oben auf 0.36 deckeln: an großen/breiten
                        // Fenstern sollen die Seiten-Sitze nicht ganz an den Rand
                        // rücken, sondern näher am Zentrum bleiben. Muss mit
                        // buildLandscapeSlots().radiusX identisch sein.
                        var radiusXpix = Math.min(0.36 * width,
                                                  Math.max(0.22 * width,
                                                           0.5 * width - sideMargin - visualW / 2))
                        // Im Compact-Landscape ragt die Bet-Badge von Player 5
                        // (betSide="bottom") 39 Basis-Pixel unterhalb seiner Box heraus.
                        // Diesen Bereich aus dem verfügbaren Ellipsen-Radius herausrechnen,
                        // damit Community Cards nie durch eine Spielerbox verdeckt werden.
                        var topBadgeExt = Config.Responsive.landscapeCompact ? 39 : 0
                        // Desktop-Top-Pad 8 (statt 4): die oberste Gegnerbox bekommt
                        // mehr Abstand zur Status-/Info-Leiste. Muss mit `topY` in
                        // buildLandscapeSlots() identisch sein.
                        var topYpix = (Config.Responsive.landscapeCompact ? 0 : 12)
                                      + visualH / 2 + topBadgeExt * sTest
                        var bottomYpix = height - 4 - selfVisualH - selfGapY - visualH / 2
                        // Oberkante des untersten Tisch-Inhalts, gegen die die
                        // Community-Reihe geprüft wird: die Self-Box bzw. – als
                        // Zuschauer – der unterste Ring-Sitz selbst.
                        var bottomContentTop = spectating
                            ? bottomYpix - visualH / 2
                            : height - 12 - selfVisualH
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
                        var maxBottomYpix = (height - 4 - selfVisualH) + selfVisualH * 0.55 - visualH / 2
                        var vMaxLowerP    = radiusYpix > 0 ? (maxBottomYpix - centerYpix) / radiusYpix : 1.0
                        // Ohne Self-Box (Zuschauer) gibt es keine Ecken neben ihr,
                        // in die untere Seiten-Sitze absinken dürften → Absenkung aus.
                        var lowerToSelfP  = Config.Responsive.landscapeCompact && !spectating
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
                            if (lowerToSelfP && sinV > 0
                                && Math.abs(radiusXpix * cosV) > selfClearXpix
                                && vMaxLowerP > vFactor)
                                vFactor = vFactor + (vMaxLowerP - vFactor) * sinOrig
                            // Desktop: oberen Bogen abflachen – obere Sitze (vFactor<0)
                            // 10 % Richtung Mitte ziehen. Flacherer Top-Bogen + mehr
                            // Abstand der Top-Box zur Info-Leiste. Muss mit point() in
                            // buildLandscapeSlots() identisch sein.
                            if (!Config.Responsive.landscapeCompact && vFactor < 0)
                                vFactor *= 0.82
                            // Fast quadratischer Tisch: obere SEITEN-Sitze (Player 3/7
                            // auf Community-Höhe) nach oben liften, sodass sie ÜBER der
                            // Community liegen (sonst Overlap mit der breiten Karten-
                            // reihe). |cos|-gewichtet (reine Seiten am stärksten), nur
                            // bei Aspect → 1 (breite Fenster bleiben unverändert).
                            // Muss mit point() in buildLandscapeSlots() identisch sein.
                            var sqLiftP = Math.max(0, Math.min(1,
                                (1.6 - width / Math.max(height, 1)) / 0.6))
                            if (!Config.Responsive.landscapeCompact && sinV < 0 && sqLiftP > 0) {
                                vFactor -= 0.3 * sqLiftP * Math.abs(cosV)
                                if (vFactor < -1.0) vFactor = -1.0
                            }
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
                                && bottomContentTop - topOppBottom
                                   < 0.72 * sTest * 124 + 28)
                                return false
                        }

                        // Bet-Badges auf beiden Seiten einrechnen (chip+text+Abstand).
                        // Ohne diesen Aufschlag erlaubt die Bisection zu große scales
                        // und die Einsatz-Anzeige reicht in die Nachbarbox hinein.
                        var xNeeded = sTest * (oppBaseWidth + sideBadgeGapBase) + gap
                        var yNeeded = sTest * oppBaseHeight + gap
                        // Als Zuschauer beginnt der Ring beim Bodensitz (90°), damit
                        // auch das Paar opp0↔opp1 geprüft wird. Das Wrap-Paar
                        // oppN↔opp0 ist dazu spiegelsymmetrisch und damit abgedeckt.
                        var ringFirst = spectating ? 90 : firstAngle
                        var ringSeats = spectating ? oppCnt + 1 : oppCnt
                        for (var iPair = 1; iPair < ringSeats; iPair++) {
                            var d1 = ringFirst + (iPair - 1) * stepDeg
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
                        // Oberkante der untersten Box: Self-Box bzw. – als
                        // Zuschauer – der gleich große unterste Ring-Sitz.
                        var bottomContentTop = spectating
                            ? height - 4 - visualH
                            : height - 12 - selfBaseHeight * sTest
                        var topYband = (Config.Responsive.landscapeCompact ? 0 : 4) + visualH / 2
                        var topOppBottom = topYband + visualH / 2 + sTest * 25
                        return bottomContentTop - topOppBottom
                               >= 0.72 * sTest * 124 + 28
                    }

                    // Gemeinsames Limit für Gegnerboxen, Self-Box und Community-Badges:
                    // 1.4 verhindert zu große Schrift und Bet-Überlappungen bei
                    // Vollbild/maximiert; compact bleibt bei 1.7 (breiter, flacher).
                    // fillCap() dämpft das Maximum bei wenigen Spielern zusätzlich.
                    var lo = 0.55, hi = fillCap(Config.Responsive.landscapeCompact ? 2.3 : 1.9)
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
                    var seqP = (spectating ? slotSeqSpectate : slotSeqPortrait)[ringCnt] || []
                    var posP = slotPosPortrait

                    function feasibleAtP(sTest) {
                        if (sTest <= 0) return false
                        var visualW = oppBaseWidth * sTest
                        var visualH = oppBaseHeight * sTest
                        var selfVisualH = selfBaseHeight * sTest

                        // Wand-Checks
                        if (visualW > 2 * (0.15 * width - 4)) return false
                        if (visualH > 2 * (0.075 * height - 4)) return false
                        if (spectating) {
                            // Wand unten: der BC-Sitz (y=0.90) darf den unteren
                            // Rand nicht berühren. Es gibt keine Self-Box.
                            if (0.90 * height + visualH / 2 > height - 4) return false
                        } else if (oppCnt >= 8) {
                            // Self-Box vs. Bottom-Reihe (L_bottom/R_bottom bei oppCnt>=8).
                            // seatNudge=+14 für diese Slots wird berücksichtigt:
                            //   self_top    = height - 4 - selfVisualH  (scale-kompensierbares bottomMargin)
                            //   bottom_kant = 0.785*height + 14 + visualH/2
                            //   Abstand     = 0.215*height - 18 - selfVisualH - visualH/2
                            //   Constraint  = Abstand >= gapP  →  0.215*H - 26 - ... >= 0
                            if (0.215 * height - 26 - selfVisualH - visualH / 2 < gapP)
                                return false
                        }

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
            //   – Querformat (Desktop + Android-Kompakt): füllen die freie
            //     Tischmitte (lücken-basiert, s. u.), Boden = Bisektions-Reserve.
            //   – Portrait: füllt das reservierte Mittelband, begrenzt durch
            //     Seitenspalten (horizontal) und Top-/Bottom-Reihe (vertikal).
            readonly property real communityScale: {
                if (wide) {
                    // Querformat (Desktop UND Android-Kompakt): Community-Cards
                    // FÜLLEN die freie Tischmitte statt nur mit boxScale mitzu-
                    // wachsen. Wir messen die vertikale Lücke um den Ablagepunkt
                    // communityCenterY und skalieren daran. Teiler 84 (> reine
                    // Halbhöhe 64) lässt Luft.
                    //   – topB: Unterkante der obersten Box + deren nach unten
                    //     zeigende Bet-Badge (Kompakt 39, sonst 26 · oppScale).
                    //   – selfTop: Oberkante der (skalierten) Self-Box.
                    // Hängt NUR von boxScale ab → keine Zirkularität. Boden =
                    // Bisektions-Reserve (Kompakt 1.1, sonst 0.72 · boxScale) →
                    // garantierte Mindestlücke, kein Regressionsrisiko.
                    var isCmp     = Config.Responsive.landscapeCompact
                    var topB      = topOpponentBottomY + (isCmp ? 39 : 26) * oppScale
                    var halfAbove = communityCenterY - topB - 6
                    var halfBelow = selfVisualTopY - communityCenterY - 6
                    var avail     = Math.min(halfAbove, halfBelow)
                    // Kompakt gefüllt (82), aber bewusst nicht so aggressiv wie
                    // früher (66): auf kleinen Phone-Landscape-Auflösungen (iPhone
                    // mini) wirkten die Community-Karten sonst überproportional groß
                    // gegenüber den (kleinen) Gegnerboxen. Desktop bleibt bei 84.
                    var gapFill   = avail > 0 ? avail / (isCmp ? 82 : 84) : 0
                    // Horizontale Sicherheit: Kartenreihe (~264 Basisbreite) bleibt
                    // innerhalb 70 % der Fensterbreite (Mitte zwischen den Seitenboxen).
                    var capW      = (0.70 * width) / 264
                    var cap       = Math.min(isCmp ? 2.1 : 1.8, boxScale * 2.0, capW)
                    // Boden (Mindestgröße): Kompakt auf 0.85·boxScale gesenkt (war
                    // 1.1) – so bleiben die Karten proportional zu den Boxen, statt
                    // sie zu überragen. Desktop unverändert (0.72).
                    var floor     = boxScale * (isCmp ? 0.85 : 0.72)
                    return Math.max(0.55, Math.min(cap, Math.max(floor, gapFill)))
                }
                // Portrait: die Community sitzt vertikal ZWISCHEN oberer (~0.345)
                // und unterer (~0.65) Reihe – auf ihrer Höhe (Tischmitte) liegen
                // KEINE Seitenboxen. Sie darf daher horizontal breit werden; die
                // einzige Overlap-Grenze ist der vertikale Abstand zur nächsten
                // Reihe (~0.15·Höhe minus Box-Halbhöhe). Horizontal nur durch die
                // Bildschirmbreite begrenzt. → deutlich größer als der frühere
                // Seitenspalten-Cap (der fälschlich Boxen auf Community-Höhe annahm).
                var vHalf = 0.15 * height - oppBaseHeight * boxScale / 2 - 6
                var maxScaleV = vHalf > 0 ? vHalf / 62 : 0.55
                var maxScaleScreen = Math.max(0, width - 16) / 264
                return Math.max(0.55, Math.min(1.8, maxScaleV, maxScaleScreen))
            }

            // ── Lupe: Zoom + Pan der Gegnerzone (compact-only) ──────────────────
            property bool  zoomActive: false
            readonly property real zoomFactor: 2.0
            // Diskreter Zoom-Faktor (Ziel, NICHT der animierte scale-Wert) für die
            // Karten-Rasterung: so wird beim Zoom-Toggle einmalig in der größeren
            // Auflösung gerastert statt pro Animationsframe (s. CardImage.renderScale).
            readonly property real zoomRenderMul: zoomActive ? zoomFactor : 1.0
            property real  _zoomPanX: 0
            property real  _zoomPanY: 0
            // Merkt sich, ob der Zoom vor dem Showdown aktiv war. Im Showdown wird
            // zur Tisch-Übersicht herausgezoomt; zur nächsten Hand wird ein zuvor
            // aktiver Zoom automatisch wieder eingeschaltet.
            property bool  _zoomSuspendedByShowdown: false
            // Verzögertes Nachführen auf den aktiven Spieler (compact-Zoom):
            // Wird ein Spieler am Zug, springt der Ausschnitt NICHT sofort zu ihm,
            // sondern erst wenn er handelt (refreshActionTriggered) ODER 1/4 seiner
            // Bedenkzeit verstrichen ist. So bleibt nach einer neuen Community-Karte
            // der Tisch-/Community-Bereich länger sichtbar.
            property int   _pendingFollowSeat: -1   // geplanter Schwenk-Sitz (Timer läuft)
            property int   _followedSeat: -1        // bereits angeschwenkter aktiver Sitz
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
            // "BC" (bottom center) wird nur im Zuschauer-Modus benutzt: dort steht
            // dort statt der Self-Box ein gewöhnlicher Ring-Sitz (Sitz 0).
            readonly property var slotPosPortrait: ({
                "L_bottom": [0.15, 0.785],
                "L_lower":  [0.15, 0.65],
                "L_upper":  [0.15, 0.345],
                "TL":       [0.15, 0.21],
                "TC":       [0.50, 0.075],
                "TR":       [0.85, 0.21],
                "R_upper":  [0.85, 0.345],
                "R_lower":  [0.85, 0.65],
                "R_bottom": [0.85, 0.785],
                "BC":       [0.50, 0.90]
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
                // Zuschauer: keine Self-Box unter der Ellipse – ihr Bodenpunkt
                // ist selbst der Mittelpunkt des untersten Ring-Sitzes.
                // Muss mit feasibleAt() identisch sein.
                var selfVisualH = spectating ? 0 : selfBaseHeight * s
                var sideMargin = Math.max(18, width * 0.025) + sideBadgeGapBase * s
                var wantedGapY = opponentGapBase * s
                var gapY = Math.max(8, wantedGapY)
                // Im landscapeCompact: halbierte selfGapY (s. Bisection-Comment).
                var selfGapY = spectating ? 0
                    : (Config.Responsive.landscapeCompact
                       ? Math.max(8, selfBadgeGapBase * s * 0.5)
                       : selfBadgeGapBase * s)
                var sideX = (sideMargin + visualW / 2) / Math.max(width, 1)
                // radiusX so groß wie möglich (Seiten-Sitze landen am Rand).
                // Top-Trio passt durch den boxScale-Cap (siehe boxScale oben)
                // automatisch in dieses Bogenstück, ohne dass wir radiusX hier
                // weiter aufblasen müssen (sonst rutschen Seiten-Sitze raus).
                // radiusX nach oben auf 0.36 deckeln → Seiten-Sitze bleiben an
                // breiten Fenstern näher am Zentrum (statt ganz an den Rand zu
                // rücken). Muss mit radiusXpix in feasibleAt() identisch sein.
                var radiusX = Math.min(0.36, Math.max(0.22, 0.5 - sideX))
                // Top- und Bottom-Rand bewusst klein – die offene Ellipse
                // soll möglichst viel vertikalen Platz beanspruchen, damit
                // bei mittlerer Skalierung Player 2↔3 (L↔TLo) genug Luft
                // bekommen.
                // Compact: oberste Box bündig an die Tisch-Oberkante (0 statt 4) –
                // schafft Luft zwischen ihrem Bet-Badge und dem Pot-Badge.
                // Desktop-Top-Pad 8 (statt 4): mehr Abstand der obersten Box zur
                // Status-/Info-Leiste. Muss mit `topYpix` in feasibleAt() identisch sein.
                var topY = ((Config.Responsive.landscapeCompact ? 0 : 12) + visualH / 2) / Math.max(height, 1)
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
                // Box-Unterkante 55 % in die Self-Box-Höhe hineinragt (tiefer =
                // Player 1/9 rücken weiter runter, mehr Luft in der Mitte für
                // Community + größere Boxen). Muss mit maxBottomYpix in feasibleAt().
                var maxBottomY = (selfTop + selfVisualH * 0.55 - visualH / 2) / Math.max(height, 1)
                var vMaxLower  = radiusY > 0 ? (maxBottomY - centerY) / radiusY : 1.0
                // Ohne Self-Box (Zuschauer) gibt es keine freien Ecken neben ihr →
                // Absenkung aus. Muss mit lowerToSelfP in feasibleAt() identisch sein.
                var lowerToSelf = Config.Responsive.landscapeCompact && !spectating
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
                    // Desktop-Landscape: die seitlichen Außen-Paare (|cos|→1)
                    // vertikal dezent entzerren – die obere Box nach oben, die
                    // untere nach unten (Richtung über sinOrig). Damit ragt das
                    // WINNER-Badge der unteren Box nicht mehr in die obere
                    // Nachbarbox. |cos|-gewichtet, sodass die Top-Mitte (cos≈0)
                    // unberührt bleibt. BEWUSST nur hier, NICHT in feasibleAt():
                    // sonst würde die Bisection den größeren Paarabstand sofort
                    // wieder mit größeren Boxen auffüllen statt echten Abstand zu
                    // schaffen. Die Caps unten halten die Boxen in der Bahn.
                    if (!Config.Responsive.landscapeCompact && cosV !== 0) {
                        var pairSpread = 0.02 * Math.abs(cosV)
                        vFactor += (sinOrig < 0 ? -pairSpread : pairSpread)
                    }
                    if (vFactor > 1.0) vFactor = 1.0   // nie unter bottomY (Self-Box)
                    if (vFactor < -1.0) vFactor = -1.0 // nie über die obere Bahn-Kante
                    // Desktop: oberen Bogen abflachen – obere Sitze (vFactor<0) 10 %
                    // Richtung Mitte ziehen → flacherer Top-Bogen + mehr Abstand der
                    // Top-Box zur Info-Leiste. Muss mit slotVec() in feasibleAt()
                    // identisch sein.
                    if (!Config.Responsive.landscapeCompact && vFactor < 0)
                        vFactor *= 0.82
                    // Graduell Richtung vMaxLower absenken, gewichtet mit dem
                    // ORIGINAL-sin: die untersten Sitze (BL/BR, sin≈0.88) sinken
                    // fast voll ab, die darüber (sin≈0.40) nur teilweise – ein
                    // einheitliches vMaxLower setzte alle auf dieselbe Höhe
                    // (flache Linie statt Ellipsenbogen).
                    if (lowerToSelf && sinV > 0
                        && Math.abs(radiusX * cosV) > selfClearX
                        && vMaxLower > vFactor)
                        vFactor = vFactor + (vMaxLower - vFactor) * sinOrig
                    // Fast quadratischer Tisch: obere SEITEN-Sitze (Player 3/7 auf
                    // Community-Höhe) nach oben liften, sodass sie ÜBER der Community
                    // liegen (sonst Overlap mit der breiten Kartenreihe). |cos|-
                    // gewichtet (reine Seiten am stärksten), nur bei Aspect → 1
                    // (breite Fenster bleiben unverändert). Muss mit slotVec() in
                    // feasibleAt() identisch sein.
                    var sqLift = Math.max(0, Math.min(1,
                        (1.6 - width / Math.max(height, 1)) / 0.6))
                    if (!Config.Responsive.landscapeCompact && sinV < 0 && sqLift > 0) {
                        vFactor -= 0.3 * sqLift * Math.abs(cosV)
                        if (vFactor < -1.0) vFactor = -1.0
                    }
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
                //
                // Zuschauer: Sitz 0 ist eine ganz normale Perle (selfWeight 1.0)
                // → dOpp = 360/seatCount, firstOppAngle = 90 + dOpp. Sitz 0 liegt
                // damit exakt auf dem Bodenpunkt (90°), die übrigen Sitze folgen
                // gleichmäßig im Kreis.
                var opps = Math.max(1, seatCount - 1)
                var selfWeight = spectating ? 1.0
                               : (Config.Responsive.landscapeCompact ? 0.5 : 0.3)
                var dOpp = 360 / (opps + selfWeight)
                var dSelf = selfWeight * dOpp
                var firstOppAngle = 90 + (dSelf + dOpp) / 2
                var slots = {}
                // Sitz 0 des Zuschauers: exakt auf dem Bodenpunkt der Ellipse.
                if (spectating)
                    slots["opp0"] = point(90)
                for (var i = 1; i <= opps; i++) {
                    slots["opp" + i] = point(firstOppAngle + (i - 1) * dOpp)
                }

                // Der Ring ist von Haus aus kopflastig-nach-unten: sideGravity und
                // lowerGravity drücken die Sitze abwärts, und am oberen Scheitel
                // (270°) sitzt bei den meisten Spielerzahlen niemand. Mit Self-Box
                // ist das gewollt – der Ring soll sich um sie schließen. Als
                // Zuschauer bleibt oben dadurch eine große Lücke, während der
                // Bodensitz am Fensterrand klebt.
                //
                // Statt die (fein austarierten) Gravity-Terme anzufassen, wird der
                // FERTIGE Ring als Ganzes vertikal in der Zone zentriert. Eine reine
                // Verschiebung: alle Paar-Abstände – und damit die Bisektion in
                // boxScale – bleiben unberührt, ebenso der Abstand zwischen oberen
                // Sitzen und Community (beide verschieben sich gleich weit).
                if (spectating) {
                    var halfH = (visualH / 2) / Math.max(height, 1)
                    var minY = Infinity, maxY = -Infinity
                    for (var name in slots) {
                        var sy = slots[name][1]
                        if (sy < minY) minY = sy
                        if (sy > maxY) maxY = sy
                    }
                    // Gleiche Ränder oben/unten: top + shift == 1 - (bottom + shift)
                    var shiftY = (1 - (maxY + halfH) - (minY - halfH)) / 2
                    for (var key in slots)
                        slots[key] = [slots[key][0], slots[key][1] + shiftY]
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
            // Zuschauer-Ring: Sitz 0 sitzt unten in der Mitte, danach folgen die
            // übrigen Sitze in genau der Reihenfolge, in der sonst die Gegner
            // verteilt werden. Indiziert über die GESAMTE Sitzzahl (1..10), nicht
            // über die Gegnerzahl.
            readonly property var slotSeqSpectate: {
                var base  = wide ? slotSeqLandscape : slotSeqPortrait
                var first = wide ? "opp0" : "BC"
                var dict  = {}
                dict[1] = [first]
                for (var n = 2; n <= 10; n++) {
                    var rest = base[n - 1]
                    if (rest) dict[n] = [first].concat(rest)
                }
                return dict
            }
            readonly property var slotSeq:
                spectating ? slotSeqSpectate : (wide ? slotSeqLandscape : slotSeqPortrait)
            // Zahl der Sitze, die auf dem Ring verteilt werden: als Zuschauer alle,
            // sonst alle außer dem eigenen (der in der Self-Box unten sitzt).
            readonly property int ringCount: spectating ? seatCount : seatCount - 1

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
                _panToPoint(width * slot.x + (slot.nudgeX || 0), height * slot.y + slot.nudge)
            }

            // Plant einen verzögerten Schwenk auf den gerade aktiven Sitz. Der
            // Schwenk erfolgt erst, wenn der Spieler handelt oder 1/4 seiner
            // Bedenkzeit (timeoutSec) abgelaufen ist – ausgelöst über followTimer
            // bzw. refreshActionTriggered.
            function _scheduleFollow(seatId, sec) {
                if (!zoomActive || !GameTable || zoomPanner.active) return
                // -1 = keiner. Sitz 0 bin ich (myTurn-Pfad) – außer als Zuschauer,
                // dort ist Sitz 0 ein ganz normaler Ring-Sitz.
                if (seatId < 0 || (seatId === 0 && !spectating)) return
                if (seatId === _followedSeat) return          // schon dort
                if (seatId === _pendingFollowSeat && followTimer.running) return  // schon geplant
                _pendingFollowSeat = seatId
                followTimer.interval = Math.max(800, (sec > 0 ? sec : 8) * 250)
                followTimer.restart()
            }

            // Führt den geplanten Schwenk sofort aus (Timer-Ablauf oder Aktion).
            function _doFollow() {
                followTimer.stop()
                if (!zoomActive || !GameTable || zoomPanner.active) return
                if (_pendingFollowSeat < 0 || (_pendingFollowSeat === 0 && !spectating)) return
                _panToSeat(_pendingFollowSeat)
                _followedSeat = _pendingFollowSeat
                _pendingFollowSeat = -1
            }

            Timer {
                id: followTimer
                repeat: false
                onTriggered: tableZone._doFollow()
            }

            function slotForSeat(seatIdx) {
                // Sitz 0 hat nur als Zuschauer einen Ring-Slot; sonst sitzt dort
                // die Self-Box, die nicht angeschwenkt wird.
                if (!GameTable || seatIdx < 0 || (seatIdx === 0 && !spectating)) return null
                var players = GameTable.players
                var oppOrder = 0
                for (var i = spectating ? 0 : 1; i <= seatIdx && i < players.length; i++)
                    if (seatOnRing(players[i])) oppOrder++
                if (oppOrder < 1) return null
                var seatCount = 0
                for (var j = 0; j < players.length; j++)
                    if (seatOnRing(players[j])) seatCount++
                var seq = slotSeq[spectating ? seatCount : seatCount - 1]
                if (!seq || oppOrder > seq.length) return null
                var name = seq[oppOrder - 1]
                var pos = slotPos[name]
                if (!pos) return null
                var nudge
                var nudgeX = 0
                // Spiegelt seatNudge/seatNudgeX im Repeater-Delegate.
                var flankWide = wide && !Config.Responsive.landscapeCompact && !spectating
                                && (name === "opp1" || name === "opp" + (seatCount - 1))
                                && pos[1] > 0.5
                if (flankWide) {
                    nudge = oppBaseHeight * boxScale * 0.6
                    var dir = pos[0] < 0.5 ? -1 : 1
                    var wantCenter = width / 2 + dir *
                        (selfBaseWidth * boxScale / 2 + 40 * boxScale
                         + oppBaseWidth * oppScale / 2 + 18)
                    var d = wantCenter - width * pos[0]
                    nudgeX = dir < 0 ? Math.min(0, d) : Math.max(0, d)
                } else if (wide) {
                    nudge = 0
                } else {
                    nudge = (name === "L_lower" || name === "L_bottom"
                             || name === "R_lower" || name === "R_bottom") ? 14
                          : (name === "L_upper" || name === "TL"
                             || name === "R_upper" || name === "TR") ? -4
                          : 0
                }
                return { x: pos[0], y: pos[1], nudge: nudge, nudgeX: nudgeX }
            }

            readonly property real topOpponentBottomY: {
                var seq = slotSeq[ringCount] || []
                // 0.13 deckelt, wie tief die "Oberkante der oberen Reihe" gelten
                // darf – nötig, solange die Community zwischen oberen Sitzen und
                // Self-Box eingepasst wird. Als Zuschauer wird sie stattdessen
                // mittig in den freien Ring gelegt; dort ist der ECHTE oberste
                // Sitz maßgeblich (bei wenigen Spielern liegt er weit unter 0.13).
                var topCenter = spectating ? Infinity : 0.13
                for (var i = 0; i < seq.length; ++i) {
                    var p = slotPos[seq[i]]
                    if (p && p[1] < topCenter) topCenter = p[1]
                }
                if (!isFinite(topCenter)) topCenter = 0.13
                return topCenter * height + oppBaseHeight * oppScale / 2
            }
            // Oberkante der untersten Box. Ohne Zuschauer-Modus ist das die
            // Self-Box; als Zuschauer der unterste Ring-Sitz (Slot opp0 / BC).
            readonly property real selfVisualTopY: {
                if (spectating) {
                    var p = slotPos[wide ? "opp0" : "BC"]
                    var cy = (p ? p[1] : 0.9) * height
                    return cy - oppBaseHeight * oppScale / 2
                }
                return selfBox.y + selfBox.height / 2 - selfBox.height * boxScale / 2
            }
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
                // Der Schwerpunkt-Trick unten existiert, um die Karten in den Pulk
                // aus Gegnern UND Self-Box zu ziehen. Ohne Self-Box (Zuschauer)
                // liegt der Ring symmetrisch um die Zonenmitte – dann gehören die
                // Karten schlicht in die Mitte des freien Innenraums.
                if (!wide || Config.Responsive.landscapeCompact || spectating)
                    return (topOpponentBottomY + selfVisualTopY) / 2
                var sumY = height - 12 - selfBaseHeight * boxScale / 2   // Self-Box-Mitte
                var n = 1
                var seq = slotSeq[ringCount] || []
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
                // Karten in der echten Bildschirmgröße rastern (scale × Zoom).
                cardRenderScale: tableZone.communityScale * tableZone.zoomRenderMul
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
                    // Gewinner-Sitz nach vorne holen, damit sein WINNER-Badge
                    // (und der goldene Rahmen) bei Überlappung IMMER über den
                    // benachbarten Gegnerboxen liegt – ein höheres z im Badge
                    // selbst greift nur innerhalb der eigenen Box, nicht
                    // zwischen den gleichrangigen (z:1) Geschwister-Slots.
                    z: (typeof GameTable !== "undefined" && GameTable
                        && GameTable.winnerSeatIds.indexOf(index) !== -1) ? 5 : 1

                    readonly property var pdata: (typeof GameTable !== "undefined" && GameTable && GameTable.players.length > index)
                        ? GameTable.players[index] : null
                    // Echter Spieler sitzt hier → Box wird gezeichnet.
                    readonly property bool occupied: pdata !== null && pdata.name !== ""
                    // Sitz beansprucht einen Ring-Slot – auch als reservierter,
                    // unsichtbarer Platzhalter eines abgegangenen Spielers.
                    readonly property bool onRing: tableZone.seatOnRing(pdata)

                    // Position dieses Sitzes auf dem Ring (1-basiert). Ohne
                    // Zuschauer-Modus sitzt Sitz 0 in der Self-Box und zählt
                    // nicht mit; als Zuschauer ist er der erste Ring-Sitz.
                    readonly property int oppOrder: {
                        if (typeof GameTable === "undefined" || !GameTable) return 0
                        var c = 0
                        for (var i = tableZone.spectating ? 0 : 1;
                             i <= index && i < GameTable.players.length; i++)
                            if (tableZone.seatOnRing(GameTable.players[i])) c++
                        return c
                    }

                    readonly property int oppCount: tableZone.ringCount
                    readonly property var seq: tableZone.slotSeq[oppCount] || []
                    readonly property string slotName:
                        (onRing && oppOrder >= 1 && oppOrder <= seq.length) ? seq[oppOrder - 1] : ""
                    // Unterster Ring-Sitz (nur im Zuschauer-Modus besetzt).
                    readonly property bool bottomCenter:
                        slotName === "opp0" || slotName === "BC"
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

                    visible: occupied && (tableZone.spectating || index !== 0) && slotName !== ""

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
                    // Self-flankierende Bottom-Sitze (opp1 / oppN = Player 1 & 9) im
                    // Desktop-Wide: tiefer + horizontal nach AUSSEN, damit sie die
                    // (große) Self-Box nicht berühren / einengen. Ultrawide
                    // (landscapeCompact) hat dafür sein eigenes Corner-Sink-Layout.
                    // Sitze links/rechts der Self-Box nach außen schieben. Als
                    // Zuschauer gibt es keine Self-Box, um die herum Platz nötig
                    // wäre – der Ring ist bereits gleichmäßig.
                    readonly property bool flankWide:
                        tableZone.wide && !Config.Responsive.landscapeCompact
                        && !tableZone.spectating
                        && (slotName === "opp1" || slotName === "opp" + oppCount)
                        && slot[1] > 0.5
                    // Horizontaler Auswärts-Versatz: Box-Mitte mindestens
                    // (Self-Halbbreite + Opp-Halbbreite + 18) von der Tischmitte weg.
                    // Nur nach außen schieben (nie nach innen ziehen).
                    readonly property real seatNudgeX: {
                        if (!flankWide) return 0
                        var dir = slot[0] < 0.5 ? -1 : 1
                        var wantCenter = tableZone.width / 2 + dir *
                            (tableZone.selfBaseWidth * tableZone.boxScale / 2
                             + 40 * tableZone.boxScale   // Dealer/Blind-Puck rechts neben der Self-Box (6 + 32 + Luft)
                             + tableZone.oppBaseWidth * tableZone.oppScale / 2 + 18)
                        var d = wantCenter - tableZone.width * slot[0]
                        return dir < 0 ? Math.min(0, d) : Math.max(0, d)
                    }
                    readonly property real seatNudge: {
                        if (tableZone.wide)
                            return flankWide
                                ? tableZone.oppBaseHeight * tableZone.boxScale * 0.6 : 0
                        if (slotName === "L_lower" || slotName === "L_bottom"
                            || slotName === "R_lower" || slotName === "R_bottom") return 14
                        if (slotName === "L_upper" || slotName === "TL"
                            || slotName === "R_upper" || slotName === "TR") return -4
                        return 0
                    }
                    x: tableZone.width * slot[0] - width / 2 + seatNudgeX
                    y: tableZone.height * slot[1] - height / 2 + seatNudge

                    GamePlayerBox {
                        anchors.fill: parent
                        seatIndex: seatSlot.index
                        // Karten in der echten Bildschirmgröße rastern (oppScale × Zoom).
                        cardRenderScale: tableZone.oppScale * tableZone.zoomRenderMul
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
                        // Oberste Mitte-Box (Player 5): das Badge unterhalb würde mit
                        // dem Pot-Badge kollidieren → Button LINKS, Einsatz RECHTS
                        // neben der Box. Gilt im gesamten Querformat (Desktop-Wide
                        // wie Ultrawide), nicht nur im landscapeCompact.
                        //
                        // Der unterste Ring-Sitz des Zuschauers (Slot opp0/BC) braucht
                        // dieselbe Aufteilung, und zwar in JEDER Orientierung: unter
                        // ihm ist kein Platz, und nebeneinander INNERHALB der Boxbreite
                        // überlappen Einsatz (zentriert) und Blind-Button (rechts).
                        betSplit: seatSlot.bottomCenter
                                  || (tableZone.wide
                                      && seatSlot.slot[0] >= 0.45 && seatSlot.slot[0] <= 0.55)
                    }
                }
            }

            // ── Eigene Box: skaliert jetzt mit dem Zoom-Layer ────────────────────
            GamePlayerSelfBox {
                id: selfBox
                z: 1
                // Als Zuschauer habe ich keinen Sitz – Sitz 0 wird stattdessen
                // als gewöhnlicher Ring-Sitz (Slot opp0/BC) gezeichnet.
                visible: !tableZone.spectating
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
                // Karten in der echten Bildschirmgröße rastern (boxScale × Zoom).
                cardRenderScale: tableZone.boxScale * tableZone.zoomRenderMul
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
                        // Eigener Zug: geplanten Gegner-Schwenk abbrechen und sofort
                        // auf die Self-Box-Zone schwenken.
                        followTimer.stop()
                        tableZone._pendingFollowSeat = -1
                        tableZone._followedSeat = 0
                        tableZone._zoomPanY = -(tableZone.zoomFactor - 1) * zoomLayer.height / 2
                        tableZone._zoomPanX = 0
                    }
                    function onTimeoutChanged() {
                        // Neuer aktiver Sitz → verzögertes Nachführen einplanen
                        // (springt nicht sofort dorthin). seatId <= 0 (−1 = kurz
                        // zwischen zwei Spielern, 0 = ich) wird ignoriert, damit
                        // ein noch ausstehender Schwenk auf den handelnden Spieler
                        // bestehen bleibt.
                        tableZone._scheduleFollow(GameTable ? GameTable.timeoutSeatId : -1,
                                                  GameTable ? GameTable.timeoutSec : 0)
                    }
                    function onRefreshActionTriggered() {
                        // Der eingeplante Spieler hat gehandelt → sofort dorthin
                        // schwenken (zeigt die Aktion), ohne das 1/4-Intervall
                        // abzuwarten.
                        tableZone._doFollow()
                    }
                    function onPlayersChanged() {
                        // Sicherheitsnetz: aktiven Gegner ebenfalls verzögert
                        // nachführen, falls timeoutChanged ausbleibt. _scheduleFollow
                        // ist idempotent (kein Timer-Thrash bei Dauerfeuer).
                        if (!tableZone.zoomActive || !GameTable || zoomPanner.active) return
                        var players = GameTable.players
                        for (var i = 1; i < players.length; i++) {
                            if (players[i].name !== "" && players[i].myTurn) {
                                tableZone._scheduleFollow(i, GameTable.timeoutSec)
                                return
                            }
                        }
                    }
                    function onBoardCardsChanged() {
                        if (!tableZone.zoomActive || !GameTable) return
                        // Neue Setzrunde / neue Hand: „bereits angeschwenkt"-Merkmal
                        // zurücksetzen, damit der erste Spieler der Runde wieder
                        // verzögert verfolgt wird.
                        followTimer.stop()
                        tableZone._pendingFollowSeat = -1
                        tableZone._followedSeat = -1
                        if (zoomPanner.active) return
                        if (GameTable.boardCardCount <= 0) return
                        // Neue Karte(n): auf den Community-Bereich schwenken.
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
                    function onShowdownActiveChanged() {
                        if (!GameTable) return
                        if (GameTable.showdownActive) {
                            // Showdown beginnt: automatisch herauszoomen, damit der
                            // gesamte Tisch mit allen Spielern (und ihren aufgedeckten
                            // Karten) wieder sichtbar ist. Vorherigen Zoom-Zustand
                            // merken und geplanten Spieler-Schwenk abbrechen.
                            tableZone._zoomSuspendedByShowdown = tableZone.zoomActive
                            followTimer.stop()
                            tableZone._pendingFollowSeat = -1
                            tableZone._followedSeat = -1
                            tableZone.zoomActive = false
                            tableZone._zoomPanX = 0
                            tableZone._zoomPanY = 0
                        } else if (tableZone._zoomSuspendedByShowdown) {
                            // Nächste Hand: einen vor dem Showdown aktiven Zoom wieder
                            // einschalten.
                            tableZone._zoomSuspendedByShowdown = false
                            tableZone.zoomActive = true
                        }
                    }
                }
            } // zoomLayer

            // ── Spielverlauf (Log) + Chat – Umschalt-Icons + Overlays ──────────
            property bool showChat: false
            // Info-Panel-Overlay (Verlauf/Chancen/Blatt) – nur im NICHT angedockten
            // Modus (Hochformat/Mobil). Im Desktop-Querformat ist es angedockt.
            property bool showInfo: false
            // Emoji-Reaktions-Picker (Panel unter dem Toggle neben dem Chat-Icon)
            property bool showReactions: false

            // ── Permanenter Game-Chat unten links (nur Desktop, nie Android) ───
            // Der Chat wird dauerhaft links neben der Action-Box gedockt (gleiche
            // Höhe, gleiche vertikale Position) – sofern dort genug freie Breite
            // ist. Reicht der Platz nicht, bleibt es beim Overlay-Chat (Chat-Icon).
            readonly property real dockedChatW: {
                if (Config.Responsive.isMobile) return 0
                if (spectating) return 0
                if (typeof GameTable === "undefined" || !GameTable || !GameTable.hasHumanOpponents) return 0
                return Math.min(280, (width - actionBar.panelWidth) / 2 - 24)
            }
            readonly property bool dockedChatFits: dockedChatW >= 170
            // Wird der Chat gedockt, ist das Overlay überflüssig.
            onDockedChatFitsChanged: if (dockedChatFits) showChat = false

            // Slot-Namen, hinter denen wirklich eine SICHTBARE Spielerbox steckt.
            // Reservierte Platzhalter (keepEmptySeats) halten zwar ihren Slot im
            // Ring, zeichnen aber nichts – Chat und Info-Panel dürfen deshalb über
            // sie hinweg bis zur nächsten sichtbaren Box aufgezogen werden. Ohne
            // diese Unterscheidung würde ein unsichtbarer Platzhalter die beiden
            // Boxen genauso begrenzen wie ein echter Gegner.
            readonly property var visibleSlotNames: {
                var set = {}
                if (typeof GameTable === "undefined" || !GameTable) return set
                var seq = slotSeq[ringCount] || []
                var players = GameTable.players
                var order = 0
                for (var i = spectating ? 0 : 1; i < players.length; i++) {
                    if (!seatOnRing(players[i])) continue
                    order++
                    if (order <= seq.length && players[i].name !== "")
                        set[seq[order - 1]] = true
                }
                return set
            }

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
                // Bet-/Puck-Badge-Ausmaße (Basis-Pixel, skalieren mit s) – siehe
                // GamePlayerBox.betGroup. Die Rand-Sitze tragen Einsatz + Dealer-/
                // Blind-Button SEITLICH zur Bildschirmkante hin (= in den Chat-
                // Bereich hinein), die mittigen Sitze unterhalb der Box. Damit der
                // Einsatz eines Gegners nie vom Chat verdeckt wird, zählt die volle
                // Box-Breite PLUS Einsatz-Breite zur Horizontal-Überlappung.
                var betSideExt   = (8 + 52) * s
                var betBottomExt = 39 * s
                // Alle Landscape-Slots durchsuchen: welche Boxen überlappen horizontal?
                var slots = slotPosLandscape
                var maxH = height + actionBar.height - 8   // kein Limit → voll
                for (var name in slots) {
                    // Unsichtbarer Platzhalter eines verlassenen Sitzes → begrenzt
                    // den Chat nicht; er darf bis zur nächsten sichtbaren Box hoch.
                    if (!visibleSlotNames[name]) continue
                    var pos     = slots[name]
                    // seatNudge/seatNudgeX aus dem Repeater-Delegate spiegeln: die
                    // beiden die Self-Box flankierenden Bottom-Sitze (opp1/oppN)
                    // werden tiefer UND weiter nach außen (zum Bildschirmrand = in
                    // den Chat-Bereich) gerückt als ihre rohe Slot-Position. Ohne
                    // diese Korrektur unterschätzt die Prüfung, wie tief/weit außen
                    // sie reichen → maxH zu groß → Überlappung mit dem Chat.
                    var nudgeY  = 0, nudgeX = 0
                    if (!Config.Responsive.landscapeCompact && !spectating
                            && (name === "opp1" || name === "opp" + (seatCount - 1))
                            && pos[1] > 0.5) {
                        nudgeY = oppBaseHeight * boxScale * 0.6
                        var dir = pos[0] < 0.5 ? -1 : 1
                        var wantCX = width / 2 + dir *
                            (selfBaseWidth * boxScale / 2 + 40 * boxScale
                             + oppBaseWidth * oppScale / 2 + 18)
                        var dCX = wantCX - width * pos[0]
                        nudgeX = dir < 0 ? Math.min(0, dCX) : Math.max(0, dCX)
                    }
                    var boxCX   = width  * pos[0] + nudgeX
                    var boxCY   = height * pos[1] + nudgeY
                    var boxL    = boxCX - visualW / 2
                    var boxR    = boxCX + visualW / 2
                    var boxBot  = boxCY + visualH / 2
                    // Einsatz-/Puck-Bereich je nach betSide einrechnen (vgl. Repeater-
                    // Delegate): linke Sitze nach links, rechte nach rechts, mittige
                    // nach unten; im landscapeCompact zeigt die mittige Box gesplittet
                    // nach beiden Seiten.
                    if (Config.Responsive.landscapeCompact && pos[0] >= 0.45 && pos[0] <= 0.55) {
                        boxL -= betSideExt
                        boxR += betSideExt
                    } else if (pos[0] < 0.45) {
                        boxL -= betSideExt
                    } else if (pos[0] > 0.55) {
                        boxR += betSideExt
                    } else {
                        boxBot += betBottomExt
                    }
                    // Überlapp nur prüfen, wenn Box (inkl. Einsatz) im Chat-Bereich liegt.
                    if (boxR <= chatLeft || boxL >= chatRight) continue
                    // Unterkante der Box/des Einsatzes + 8 px Sicherheitsabstand:
                    var boxBottom = boxBot + 8
                    // Chat darf höchstens bis zur Unterkante dieser Box reichen.
                    var limit = height - boxBottom + actionBar.height - 8
                    if (limit < maxH) maxH = limit
                }
                return Math.max(dockedChatMinH, maxH)
            }
            // Vom Benutzer per Drag-Handle eingestellte Höhe; -1 = Standard.
            property real dockedChatUserH: -1
            // Eingeklappt (per Toggle ausgeblendet)? Default: eingeklappt (Toggle aus).
            property bool dockedChatCollapsed: true

            // ── Permanentes Info-Panel unten RECHTS – Spiegelbild des Docked-Chat ──
            // Gleiches Layout wie der Chat (unten links): schwebt ÜBER dem Tisch,
            // nach oben aufziehbar, reserviert keinen Platz. Nur Desktop mit genug
            // freier Breite; sonst Overlay + Toggle.
            readonly property real dockedInfoW: {
                if (Config.Responsive.isMobile) return 0
                if (spectating) return 0
                return Math.min(280, (width - actionBar.panelWidth) / 2 - 24)
            }
            readonly property bool dockedInfoFits: dockedInfoW >= 170
            // Wird das Panel gedockt, ist das Overlay überflüssig.
            onDockedInfoFitsChanged: if (dockedInfoFits) showInfo = false
            // Eingeklappt (per Toggle ausgeblendet)? Default: eingeklappt (Toggle aus).
            property bool dockedInfoCollapsed: true

            readonly property real dockedInfoMinH: actionBar.height - 8
            // Maximale Höhe: bis zur Unterkante der untersten Gegnerbox, die
            // horizontal mit dem Panel (rechter Rand) überlappt – wie beim Chat.
            readonly property real dockedInfoMaxH: {
                if (!wide || !dockedInfoFits) return dockedInfoMinH
                var s = oppScale
                var visualW = oppBaseWidth  * s
                var visualH = oppBaseHeight * s
                var infoRight = width - 8
                var infoLeft  = width - 8 - dockedInfoW
                // Bet-/Puck-Badge-Ausmaße wie beim Chat (s. dockedChatMaxH): die
                // rechten Rand-Sitze tragen den Einsatz seitlich nach rechts (in den
                // Info-Bereich hinein) – volle Box- PLUS Einsatz-Breite zählen.
                var betSideExt   = (8 + 52) * s
                var betBottomExt = 39 * s
                var slots = slotPosLandscape
                var maxH = height + actionBar.height - 8
                for (var name in slots) {
                    // Platzhalter-Slot (kein sichtbarer Spieler) → keine Begrenzung.
                    if (!visibleSlotNames[name]) continue
                    var pos   = slots[name]
                    // seatNudge/seatNudgeX aus dem Repeater-Delegate spiegeln (vgl.
                    // dockedChatMaxH): die Self-Box flankierenden Bottom-Sitze
                    // (opp1/oppN) sitzen tiefer und weiter außen (hier: rechter
                    // Sitz in den Info-Bereich) als ihre rohe Slot-Position.
                    var nudgeY = 0, nudgeX = 0
                    if (!Config.Responsive.landscapeCompact && !spectating
                            && (name === "opp1" || name === "opp" + (seatCount - 1))
                            && pos[1] > 0.5) {
                        nudgeY = oppBaseHeight * boxScale * 0.6
                        var dir = pos[0] < 0.5 ? -1 : 1
                        var wantCX = width / 2 + dir *
                            (selfBaseWidth * boxScale / 2 + 40 * boxScale
                             + oppBaseWidth * oppScale / 2 + 18)
                        var dCX = wantCX - width * pos[0]
                        nudgeX = dir < 0 ? Math.min(0, dCX) : Math.max(0, dCX)
                    }
                    var boxCX = width  * pos[0] + nudgeX
                    var boxCY = height * pos[1] + nudgeY
                    var boxL  = boxCX - visualW / 2
                    var boxR  = boxCX + visualW / 2
                    var boxBot = boxCY + visualH / 2
                    if (Config.Responsive.landscapeCompact && pos[0] >= 0.45 && pos[0] <= 0.55) {
                        boxL -= betSideExt
                        boxR += betSideExt
                    } else if (pos[0] < 0.45) {
                        boxL -= betSideExt
                    } else if (pos[0] > 0.55) {
                        boxR += betSideExt
                    } else {
                        boxBot += betBottomExt
                    }
                    if (boxR <= infoLeft || boxL >= infoRight) continue
                    var boxBottom = boxBot + 8
                    var limit = height - boxBottom + actionBar.height - 8
                    if (limit < maxH) maxH = limit
                }
                return Math.max(dockedInfoMinH, maxH)
            }
            // Vom Benutzer per Drag-Handle eingestellte Höhe; -1 = Standard.
            property real dockedInfoUserH: -1

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

            // ── Info-Panel-Overlay (Verlauf/Chancen/Blatt) ────────────────────
            // Nur im NICHT angedockten Modus (Hochformat/Mobil). Im Desktop-
            // Querformat liegt das Panel permanent angedockt rechts (infoDock,
            // direktes Kind von gamePage).
            GameSidePanel {
                id: infoOverlay
                z: 150
                edge: Qt.RightEdge
                wide: tableZone.wide
                // Keine Überschrift – die Tab-Leiste (Verlauf/Chancen) reicht;
                // geschlossen wird über den Umschalt-Button oben rechts.
                showHeader: false
                visible: tableZone.showInfo && !gamePage.infoDocked
                onCloseRequested: gamePage.toggleInfoOverlay()

                GameInfoPanel {
                    id: infoPanelOverlay
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            // Combined-Toggle (Verlauf/Chancen/Blatt). Als Kind der tableZone
            // sitzt er automatisch an deren rechter Kante: angedockt links neben
            // dem Panel, sonst am rechten Bildschirmrand. Im Desktop-Querformat
            // klappt er das angedockte Panel ein/aus, sonst das Overlay.
            GameRoundIconButton {
                id: infoToggle
                z: 200
                // Zuschauer: kein Verlauf/Chancen-Panel (Chancen setzen ein
                // eigenes Blatt voraus, das es hier nicht gibt).
                visible: !tableZone.spectating
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 8
                iconSource: "../resources/gameLog.svg"
                active: gamePage.infoPanelOpen
                tooltipText: qsTr("Verlauf & Chancen")
                onClicked: gamePage.toggleInfoOverlay()
            }

            // ── Chat-Overlay (nur bei menschlichen Mitspielern) ────────────────
            GameSidePanel {
                id: chatOverlay
                z: 150
                edge: Qt.LeftEdge
                wide: tableZone.wide
                // Keine Überschrift – geschlossen wird über den Umschalt-Button oben.
                showHeader: false
                visible: tableZone.showChat
                onCloseRequested: gamePage.toggleChatOverlay()
                // Chat geschlossen → Emoji-Picker mitschließen.
                onVisibleChanged: if (!visible) overlayChat.closeEmojiPicker()

                ChatBox {
                    id: overlayChat
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // Feste Tisch-Theme-Farben (dunkel, modus-unabhängig).
                    colText: gamePage.tblChatText
                    colTextSecondary: gamePage.tblChatTextSecondary
                    colTextMuted: gamePage.tblChatTextMuted
                    colBorder: gamePage.tblChatBorder
                    colSurface: gamePage.tblChatSurface
                    colBackground: gamePage.tblChatBackground
                    chatModel: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog : []
                    chatTranslator: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatTranslator : null
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
                // Immer sichtbar (sofern menschliche Mitspieler) – auch wenn der
                // Chat permanent angedockt ist, damit er sich ausblenden lässt.
                // Zuschauer nehmen am Tischchat nicht teil.
                visible: !tableZone.spectating
                         && ((typeof GameTable !== "undefined" && GameTable) ? GameTable.hasHumanOpponents : false)
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 8
                iconSource: "../resources/gameChat.svg"
                active: gamePage.chatPanelOpen
                unread: tableZone.chatUnread
                tooltipText: qsTr("Chat")
                onClicked: gamePage.toggleChatOverlay()
            }

            // ── Emoji-Reaktions-Picker: Toggle rechts neben dem Chat-Icon ──────
            GameRoundIconButton {
                id: reactionToggle
                z: 200
                // Zuschauer haben keinen Sitz, über dem eine eigene Reaktion
                // aufsteigen könnte. Reaktionen ANDERER bleiben sichtbar.
                visible: gamePage.emojiReactionsEnabled && !tableZone.spectating
                anchors.top: parent.top
                anchors.left: chatToggle.visible ? chatToggle.right : parent.left
                anchors.leftMargin: chatToggle.visible ? 6 : 8
                anchors.topMargin: 8
                iconSource: "../resources/addReaction.svg"
                active: tableZone.showReactions
                tooltipText: qsTr("Emoji-Reaktionen")
                onClicked: tableZone.showReactions = !tableZone.showReactions
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
        // Zuschauer greifen nicht ins Spiel ein → keine Action-Leiste. Der
        // freiwerdende Platz kommt dem Tisch zugute (Layout.fillHeight oben).
        GameActionBar {
            id: actionBar
            visible: !tableZone.spectating
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            wide: tableZone.wide
            communityVisualWidth: communityArea.width * communityArea.scale
            // Während neue Gemeinschaftskarten aufgedeckt werden, keine Aktion.
            boardDealing: communityArea.dealing
        }
    }

    // ── Permanenter Game-Chat: links neben der Action-Box ───────────────────
    // Direktes Kind von gamePage (über dem ColumnLayout), damit er nach oben
    // über die Action-Bar hinaus aufgezogen werden kann.
    Rectangle {
        id: dockedChat
        visible: tableZone.dockedChatFits && !tableZone.dockedChatCollapsed
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
        color: Config.Theme.withAlpha(gamePage.tblChatBackground, 0.7)
        border.color: gamePage.tblChatBorder
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
            // Feste Tisch-Theme-Farben (dunkel, modus-unabhängig).
            colText: gamePage.tblChatText
            colTextSecondary: gamePage.tblChatTextSecondary
            colTextMuted: gamePage.tblChatTextMuted
            colBorder: gamePage.tblChatBorder
            colSurface: gamePage.tblChatSurface
            colBackground: gamePage.tblChatBackground
            chatModel: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatLog : []
            chatTranslator: (typeof GameTable !== "undefined" && GameTable) ? GameTable.chatTranslator : null
            nickList: gamePage.gameNickList()
            messageFontSize: 13
            inputHeight: 28
            // Wenig Platz → Picker als Popup über der Box.
            emojiPickerAsPopup: true
            onSendRequested: (text) => {
                if (typeof GameTable !== "undefined" && GameTable)
                    GameTable.sendChat(text)
            }
        }
    }

    // ── Permanentes Info-Panel: unten rechts (Spiegelbild des Docked-Chat) ──────
    // Direktes Kind von gamePage (über dem ColumnLayout), damit es nach oben über
    // die Action-Bar hinaus aufgezogen werden kann. Schwebt ÜBER dem Tisch.
    Rectangle {
        id: infoDock
        visible: tableZone.dockedInfoFits && !tableZone.dockedInfoCollapsed
        z: 20
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        width: tableZone.dockedInfoW
        height: {
            // Initial = Höhe der Action-Button-Box (wie der Docked-Chat),
            // per Handle aufziehbar. Immer auf den verfügbaren Platz geklemmt.
            var h = tableZone.dockedInfoUserH >= 0
                    ? tableZone.dockedInfoUserH : tableZone.dockedInfoMinH
            return Math.max(tableZone.dockedInfoMinH,
                            Math.min(tableZone.dockedInfoMaxH, h))
        }
        radius: 10
        // Bewusst transparenter (wie der Docked-Chat) – der Tisch bleibt sichtbar.
        color: Config.Theme.withAlpha(gamePage.tblChatBackground, 0.7)
        border.color: gamePage.tblChatBorder
        border.width: 1

        GameInfoPanel {
            id: infoPanelDock
            anchors.fill: parent
            anchors.margins: 8
            anchors.topMargin: 12   // Platz für den Resize-Handle
            // Gleiche Schriftgröße wie der gedockte Chat.
            messageFontSize: 11
        }

        // ── Größenänderungs-Handle (Ziehen nach oben) – wie beim Chat ─────────
        Item {
            id: infoResizeHandle
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
                color: infoResizeDrag.containsMouse || infoResizeDrag.pressed
                       ? Config.Theme.colorAccent
                       : Qt.rgba(1, 1, 1, 0.22)
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: infoResizeDrag
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                hoverEnabled: true
                property real pressGlobalY: 0
                property real pressH: 0
                onPressed: (mouse) => {
                    pressGlobalY = mapToItem(gamePage, mouse.x, mouse.y).y
                    pressH = infoDock.height
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var curY = mapToItem(gamePage, mouse.x, mouse.y).y
                    var delta = pressGlobalY - curY   // nach oben = positiv
                    tableZone.dockedInfoUserH = Math.max(
                        tableZone.dockedInfoMinH,
                        Math.min(tableZone.dockedInfoMaxH, pressH + delta))
                }
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

        SvgIcon {
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

    // ── Spielende im lokalen Spiel ───────────────────────────────────────────
    // Der GameHandler meldet das Turnierende (nur noch ein Spieler mit Chips)
    // und startet keine weitere Hand mehr. Hier bekommt der Sieger seine
    // Meldung – mit der Wahl, mit denselben Einstellungen neu zu starten oder
    // zum Menü zurückzukehren (Pendant zum „Start"-Button des Widget-Clients).
    Connections {
        target: GameTable
        function onLocalGameFinished(winnerName, winnerSeatId) {
            gameOverPopup.winnerName = winnerName
            gameOverPopup.humanWon = (winnerSeatId === 0)
            gameOverPopup.open()
        }
    }

    Popup {
        id: gameOverPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: Math.min((parent ? parent.width : 380) * 0.85, 380)
        closePolicy: Popup.CloseOnEscape

        property string winnerName: ""
        property bool humanWon: false

        background: Rectangle {
            color: Config.Theme.colorBox
            border.color: Config.StaticData.palette.secondary.col400
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            spacing: 12
            width: gameOverPopup.availableWidth

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Game Over")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }
            AppLabel {
                Layout.fillWidth: true
                text: gameOverPopup.humanWon
                      ? qsTr("Congratulations, you won the game!")
                      : qsTr("%1 wins the game!").arg(gameOverPopup.winnerName)
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("Back to Menu")
                    onClicked: {
                        gameOverPopup.close()
                        mainWindow.performLeaveGame()
                    }
                }
                CustomButton {
                    Layout.fillWidth: true
                    text: qsTr("New Game")
                    onClicked: {
                        // Neues Spiel mit denselben Einstellungen – die Seite
                        // bleibt stehen, GameHandler::setGame() setzt Tisch,
                        // Verlauf und Chat zurück.
                        gameOverPopup.close()
                        GameTable.startLocalGame()
                    }
                }
            }
        }
    }
}
