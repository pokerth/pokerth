import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Gemeinsame Chat-Box für ALLE Chats (Lobby compact/wide, GameWait,
// Game-Chat-Overlay, gedockter Ingame-Chat):
//   • Nachrichtenliste (RichText-Zeilen) mit Auto-Scroll-Logik
//     (pausiert beim Hochscrollen, Position bleibt bei neuen Zeilen
//     erhalten, nach 15 s Inaktivität wieder ans Ende)
//   • Emoji-Picker – inline über der Eingabezeile oder als Popup über
//     der Box (Platzmangel, z. B. gedockter Ingame-Chat)
//   • Eingabezeile mit Emoji-Toggle und Send-Button
//   • Chat-History (Pfeil hoch/runter, max. 50, wie Qt-Widgets-Client)
//   • Tab-Nick-Vervollständigung mit Iteration (Config.StaticData.nickComplete)
Item {
    id: root

    // ── API ──────────────────────────────────────────────────────────────
    // Liste formatierter (RichText-)Zeilen, z. B. GameTable.chatLog / Lobby.chatLog.
    property var chatModel: []
    // Nicknames für die Tab-Vervollständigung.
    property var nickList: []
    // Chat-Übersetzer des zugehörigen Handlers (Lobby.chatTranslator bzw.
    // GameTable.chatTranslator). Taps auf das Globus-Symbol werden hierhin
    // geroutet; null = keine Übersetzung (Symbole erscheinen dann gar nicht).
    property var chatTranslator: null
    property bool inputEnabled: true
    property string placeholder: qsTr("Nachricht …")
    property int messageFontSize: 14
    // Emoji-Picker als Popup ÜBER der Box statt inline über der Eingabezeile.
    property bool emojiPickerAsPopup: false
    property int pickerInlineHeight: 150
    property int inputHeight: 36
    property bool showEmojiPicker: false

    // ── Farb-Tokens (überschreibbar) ────────────────────────────────────────
    // Default = globale Palette, damit Lobby/GameWait weiter dem Hell/Dunkel-
    // Modus folgen. Am Tisch werden diese mit den (festen, dunklen) Farben des
    // Tisch-Themes (StyleProvider.chatLog*) überschrieben.
    property color colText:          Config.StaticData.palette.secondary.col100
    property color colTextSecondary: Config.StaticData.palette.secondary.col200
    property color colTextMuted:     Config.StaticData.palette.secondary.col400
    property color colBorder:        Config.StaticData.palette.secondary.col500
    property color colSurface:       Config.StaticData.palette.secondary.col600
    property color colBackground:    Config.StaticData.palette.secondary.col700
    property color colAccent:        Config.Theme.colorAccent

    signal sendRequested(string text)

    function closeEmojiPicker() { showEmojiPicker = false }
    function scrollToEnd() { msgFlick.scrollToBottom() }

    // Öffnet einen Link im externen Browser. NICHT direkt Qt.openUrlExternally:
    // Im AppImage/Bundle erbt QDesktopServices das gebundelte LD_LIBRARY_PATH/
    // LD_PRELOAD → xdg-open crasht und nichts öffnet. LobbyHandler.openExternalUrl
    // startet die Host-Tools (xdg-open/gio/kde-open) mit bereinigter Umgebung –
    // exakt wie der Footer (LobbyStatsBar). Qt.openUrlExternally nur als Fallback.
    function _openLink(link) {
        if (!link || link === "")
            return
        var opened = false
        if (typeof Lobby !== "undefined" && Lobby)
            opened = Lobby.openExternalUrl(link)
        if (!opened)
            opened = Qt.openUrlExternally(link)
        if (!opened)
            console.warn("ChatBox: konnte URL nicht öffnen:", link)
    }

    // ── Übersetzen-Symbol nur an der Zeile unter dem Mauszeiger ───────────
    // Der Verlauf ist EIN RichText-Dokument (keine ListView), „Zeile" ist also
    // der Index in chatModel. Die Einträge werden mit <br> verbunden und liegen
    // im Dokument damit durch Zeilentrenner (U+2028) getrennt vor: der
    // Zeilenindex an einer Position = Anzahl der Trenner davor. chatModel-
    // Einträge enthalten selbst nie Zeilenumbrüche (eine Chat-Zeile = ein
    // Eintrag), die Zuordnung ist also 1:1.
    // _hoverFrom/_hoverTo cachen den Bereich der aktuell markierten Zeile, damit
    // pro Mausbewegung nur die (billige) Positionsabfrage nötig ist und nicht
    // das Durchzählen des ganzen Verlaufs.
    property int _hoverFrom: -1
    property int _hoverTo: -1

    function _isLineSep(code) { return code === 0x2028 || code === 0x2029 }

    function _updateHoverLine(x, y) {
        if (!chatTranslator)
            return
        // Solange etwas markiert ist, NICHT ins Dokument schreiben: das Ein-/
        // Ausblenden des Symbols ersetzt msgText.text komplett und würde die
        // Auswahl (bzw. ein laufendes Ziehen) zerstören.
        if (msgText.selectedText.length > 0)
            return
        var pos = msgText.positionAt(x, y)
        if (pos >= _hoverFrom && pos <= _hoverTo)
            return                       // immer noch dieselbe Zeile
        var all = msgText.getText(0, msgText.length)
        var line = 0, from = 0, i
        for (i = 0; i < pos && i < all.length; ++i) {
            if (_isLineSep(all.charCodeAt(i))) { ++line; from = i + 1 }
        }
        var to = all.length
        for (i = pos; i < all.length; ++i) {
            if (_isLineSep(all.charCodeAt(i))) { to = i; break }
        }
        _hoverFrom = from
        _hoverTo = to
        chatTranslator.setHoveredLine(line)
    }

    function _clearHoverLine() {
        _hoverFrom = -1
        _hoverTo = -1
        if (chatTranslator)
            chatTranslator.setHoveredLine(-1)
    }

    // Neue Nachricht (oder ein ein-/ausgeblendetes Symbol) verschiebt alle
    // Positionen dahinter → gecachten Zeilenbereich verwerfen.
    onChatModelChanged: { _hoverFrom = -1; _hoverTo = -1 }

    // Verschwindet die Box (Seitenwechsel Lobby/Warteraum – beide hängen am
    // selben Übersetzer), bliebe sonst ein Symbol an der zuletzt überfahrenen
    // Zeile stehen.
    onVisibleChanged: if (!visible) _clearHoverLine()
    Component.onDestruction: _clearHoverLine()

    implicitWidth: 200
    implicitHeight: 160

    // Rechter Freiraum für die vertikale Scrollbar, damit sie den Inhalt nie
    // überlappt (Scrollbars liegen im Overlay-Stil sonst über dem Text).
    readonly property int scrollGutter: 14

    // Beim Aufklappen des Inline-Pickers schrumpft die Liste – ans Ende
    // scrollen, damit die letzten Nachrichten sichtbar bleiben.
    onShowEmojiPickerChanged: {
        if (showEmojiPicker && !emojiPickerAsPopup)
            Qt.callLater(msgFlick.scrollToBottom)
    }

    // ── History + Tab-Vervollständigung ──────────────────────────────────
    // History-Speicher (gesendete Nachrichten, max. 50). Default: eigenes
    // Array pro Instanz. Mehrere ChatBoxen desselben Chat-Kanals (z. B.
    // Lobby compact/wide + GameWait) können hier DASSELBE Array binden und
    // teilen sich damit die History – der Navigationsindex bleibt lokal.
    property var historyStore: []
    property int _historyIndex: 0
    property var _nickState: ({ counter: 0, base: "", matches: [] })
    // Link unter dem zuletzt rechtsgeklickten Punkt (für das Kontextmenü).
    property string _menuLink: ""

    function _showHistory(idx) {
        if (idx > 0 && idx <= historyStore.length)
            inputField.text = historyStore[historyStore.length - idx]
        else
            inputField.text = ""
        inputField.cursorPosition = inputField.text.length
    }

    // Der Server prüft VALIDATE_STRING_SIZE(chattext, 1, MAX_CHAT_TEXT_SIZE=128)
    // und trennt bei Überlänge die Verbindung. Wie der Widgets-Client
    // (ChatTools::checkInputLength) begrenzen wir die Eingabe daher auf 128
    // UTF-8-Bytes – NICHT auf eine feste Zeichenzahl, da Umlaute (2 Bytes) und
    // Emojis (4 Bytes) mehr als ein Byte belegen. So lässt sich nur so viel
    // eingeben, wie auch wirklich gesendet werden darf.
    readonly property int maxChatBytes: 128

    // UTF-8-Bytelänge eines Strings (JS-Strings sind UTF-16). Surrogatpaare
    // (Emojis) zählen als 4 Bytes und werden über i++ als Einheit übersprungen.
    function _utf8ByteLen(str) {
        var n = 0
        for (var i = 0; i < str.length; ++i) {
            var c = str.charCodeAt(i)
            if (c < 0x80) n += 1
            else if (c < 0x800) n += 2
            else if (c >= 0xD800 && c <= 0xDBFF) { n += 4; ++i }
            else n += 3
        }
        return n
    }

    // Kürzt die Eingabe zeichenweise, bis sie ins Server-Byte-Limit passt.
    // Läuft bei JEDER Textänderung (auch Einfügen/Emoji), damit übergroßer
    // Text gar nicht erst stehen bleibt. Surrogatpaare werden als Ganzes
    // entfernt, damit kein halbes Emoji zurückbleibt.
    function _clampChatInput() {
        var s = inputField.text
        if (_utf8ByteLen(s) <= maxChatBytes)
            return
        while (s.length > 0 && _utf8ByteLen(s) > maxChatBytes) {
            var last = s.charCodeAt(s.length - 1)
            var drop = (last >= 0xDC00 && last <= 0xDFFF) ? 2 : 1
            s = s.slice(0, s.length - drop)
        }
        var pos = s.length
        inputField.text = s
        inputField.cursorPosition = pos
    }

    // ── Shortcode-Autovervollständigung (":smi…" → 😄) ────────────────────
    // Vorschläge kommen aus derselben C++-Map, die beim Senden ersetzt
    // (chat_emote_shortcuts.h via Lobby.chatEmoteShortcodes) – angeboten wird
    // also nur, was auch wirklich funktioniert. Trigger: ":" + mindestens
    // 2 Kleinbuchstaben vor dem Cursor (wie Discord; so kollidieren ASCII-
    // Kürzel wie ":p"/":s" nicht mit dem Popup).
    property var _emoteCodes: []
    property var _emoteMatches: []
    property int _emoteIndex: 0
    property int _emoteTokenStart: -1
    // Esc blendet das Popup bis zur nächsten Eingabe aus.
    property bool _emoteSuppressed: false

    function _emoteList() {
        if (_emoteCodes.length === 0 && typeof Lobby !== "undefined" && Lobby)
            _emoteCodes = Lobby.chatEmoteShortcodes()
        return _emoteCodes
    }

    function _updateEmoteSuggestions() {
        var upto = inputField.text.slice(0, inputField.cursorPosition)
        // Token = ":" (am Anfang oder nach Leerzeichen) + 2+ Code-Zeichen
        // direkt vor dem Cursor. Der schließende ":" beendet den Token –
        // fertige Shortcodes lassen das Popup also von selbst verschwinden.
        var m = upto.match(/(?:^|\s):([a-z0-9_+-]{2,})$/)
        if (!m) {
            if (_emoteMatches.length > 0)
                _emoteMatches = []
            return
        }
        var typed = m[1]
        _emoteTokenStart = upto.length - typed.length - 1
        var list = _emoteList()
        var pre = [], sub = []
        for (var i = 0; i < list.length; ++i) {
            var idx = list[i].code.indexOf(typed)
            if (idx === 0) pre.push(list[i])
            else if (idx > 0) sub.push(list[i])
        }
        _emoteMatches = pre.concat(sub)
        _emoteIndex = 0
    }

    // Ersetzt den getippten Token (":smi") durch das Emoji des gewählten
    // Vorschlags – als Emoji statt ":smile:", genau wie der Emoji-Picker
    // (WYSIWYG und weniger Bytes im 128-Byte-Server-Limit).
    function _acceptEmoteSuggestion() {
        if (_emoteMatches.length === 0)
            return
        var e = _emoteMatches[Math.min(_emoteIndex, _emoteMatches.length - 1)]
        var t = inputField.text
        var newText = t.slice(0, _emoteTokenStart) + e.emoji
                      + t.slice(inputField.cursorPosition)
        var pos = _emoteTokenStart + e.emoji.length
        inputField.text = newText
        inputField.cursorPosition = Math.min(pos, inputField.text.length)
        _emoteMatches = []
    }

    function _send() {
        var t = inputField.text.trim()
        if (t === "")
            return
        historyStore.push(inputField.text)
        if (historyStore.length > 50)
            historyStore.shift()
        _historyIndex = 0
        root.sendRequested(t)
        inputField.text = ""
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        // ── Nachrichtenverlauf: EIN zusammenhängendes RichText-Dokument ──
        // Wie das QTextBrowser des Widgets-Clients (chattools.cpp) – statt einer
        // ListView mit einem TextEdit pro Zeile. Vorteile: durchgehende Maus-
        // Selektion über ALLE Nachrichten (statt isolierter Markierung je Zeile)
        // und natives, zuverlässiges Link-Handling über onLinkActivated.
        Flickable {
            id: msgFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: msgText.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ScrollBar.vertical: ScrollBar {
                id: msgScrollBar
                policy: msgFlick.contentHeight > msgFlick.height + 4
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                // Ziehen am Scrollbar-Griff setzt contentY direkt und erzeugt
                // KEINE movementStarted/Ended-Signale der Flickable – deshalb
                // hier von Hand an dieselbe Auswertung hängen.
                onPressedChanged: pressed ? msgFlick.userScrollStarted()
                                          : msgFlick.userScrollEnded()
            }

            // Auto-Scroll: pausiert beim Hochscrollen, Position bleibt bei neuen
            // Zeilen erhalten (der Text wird komplett ersetzt → die View würde
            // sonst springen), nach 15 s Inaktivität wieder ans Ende.
            property bool autoScroll: true
            property real savedContentY: 0
            Timer {
                id: autoScrollTimer
                interval: 15000
                onTriggered: { msgFlick.autoScroll = true; msgFlick.scrollToBottom() }
            }
            // Beim Scrollen wandern die Zeilen unter dem stehenden Mauszeiger
            // hindurch – das Übersetzen-Symbol muss der neuen Zeile folgen.
            // Verzögert, damit das während eines Flicks nicht pro Frame läuft.
            Timer {
                id: hoverRecheckTimer
                interval: 60
                onTriggered: if (msgHover.hovered)
                                 root._updateHoverLine(msgHover.point.position.x,
                                                       msgHover.point.position.y)
            }
            // Ans Ende kleben. pinBottom() prüft selbst autoScroll, damit ein
            // nachgelagerter (Qt.callLater-)Aufruf nichts tut, wenn der Nutzer
            // inzwischen weggescrollt hat.
            function pinBottom() {
                if (autoScroll) contentY = Math.max(0, contentHeight - height)
            }
            // Externer Sprung ans Ende = „wieder mitlaufen": autoScroll erst an,
            // dann pinnen.
            function scrollToBottom() { autoScroll = true; pinBottom() }
            function restoreScroll() {
                contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
            }
            // Bei Auto-Scroll ZWEIMAL ans Ende ziehen: sofort (contentHeight ist im
            // Change-Handler bereits der neue Wert) UND einmal per Qt.callLater.
            // Grund: QQuickTextEdit aktualisiert seine implicitHeight erst in der
            // Polish-Phase und QQuickFlickable aktualisiert seine INTERNE
            // Scroll-Grenze (max. contentY) ebenfalls erst dort. Je nach Reihen-
            // folge wird das sofortige Setzen von der noch alten Grenze nach unten
            // geklemmt – dann greift der callLater NACH dem Polish auf die finale
            // Grenze. Einer der beiden Durchläufe landet immer korrekt; das
            // doppelte Setzen auf denselben Endwert ist folgenlos.
            // Das war der Bug: mit reinem Qt.callLater blieb die zuletzt getippte
            // Zeile bis zur nächsten Nachricht unter dem Sichtbereich.
            function followBottom() {
                // Während einer laufenden Nutzergeste gar nichts anfassen.
                if (moving || msgScrollBar.pressed)
                    return
                if (autoScroll) { pinBottom(); Qt.callLater(pinBottom) }
                // Pausiert: gemerkte Position halten, während der Text komplett
                // ersetzt wird.
                else restoreScroll()
            }
            // An contentHeight hängen: feuert bei JEDER Höhenänderung – neue Zeile,
            // async umbrechende RichText-Zeilen und komplettes Ersetzen des Texts.
            onContentHeightChanged: followBottom()
            // Resize (z. B. geänderte Spieleranzahl) – gleich behandeln.
            onHeightChanged: followBottom()

            // ── Auto-Scroll-Zustand NUR aus echten Nutzergesten ableiten ──────
            // Früher hing das an onContentYChanged (gefiltert über `moving`).
            // Das war die zweite Fehlerquelle: `moving` gilt auch für die
            // Positionskorrektur, die die Flickable bei JEDER Höhenänderung
            // selbst vornimmt (fixupY nach setContentHeight – z. B. wenn das
            // Übersetzen-Symbol eine Zeile umbrechen lässt oder der Emoji-
            // Picker aufgeht). Ein solcher Zwischenwert liegt zwangsläufig
            // nicht am unteren Rand → autoScroll kippte auf false, obwohl der
            // Nutzer nichts getan hatte. Danach hielt restoreScroll() die View
            // knapp über dem Ende fest: die letzte Zeile blieb angeschnitten,
            // bis der 15-s-Timer oder die nächste Nachricht sie wieder
            // einfing – und ein Vergrößern der Box „reparierte" es nur dann,
            // wenn dabei savedContentY über die neue Untergrenze rutschte.
            // Jetzt: Geste beginnt → pausieren, Geste ist zur RUHE gekommen →
            // einmal sauber entscheiden.
            function userScrollStarted() {
                autoScroll = false
                autoScrollTimer.stop()
            }
            function userScrollEnded() {
                savedContentY = contentY
                // Mit Toleranz prüfen statt exaktem atYEnd: knapp am Ende reicht
                // (Subpixel/async wachsende RichText-Zeilen), damit der
                // Auto-Scroll am unteren Rand zuverlässig wieder anspringt.
                autoScroll = contentY >= contentHeight - height - 4
                if (autoScroll) { autoScrollTimer.stop(); pinBottom() }
                else autoScrollTimer.restart()
            }
            onMovementStarted: userScrollStarted()
            onMovementEnded: userScrollEnded()

            // Beim Scrollen wandern die Zeilen unter dem stehenden Mauszeiger
            // hindurch – unabhängig davon, wer gescrollt hat.
            onContentYChanged: hoverRecheckTimer.restart()

            // Read-only TextEdit hält den gesamten Verlauf als EIN HTML-Dokument.
            // Die einzelnen chatModel-Einträge sind bereits fertiges RichText und
            // werden mit <br> aneinandergereiht.
            TextEdit {
                id: msgText
                // Fester Freiraum für die Scrollbar – NICHT abhängig davon, ob
                // sie gerade gebraucht wird. Sonst hinge die Textbreite an
                // msgFlick.contentHeight, diese an msgText.implicitHeight und
                // die wiederum (QQuickTextEdit rechnet in geometryChange sofort
                // neu) an der Textbreite: ein echter, synchroner Bindungs-
                // Zirkel. QML bricht dabei die re-entrante Auswertung ab, und
                // je nach Einstiegspunkt bleibt contentHeight auf dem ALTEN
                // Wert stehen, während das Dokument schon höher ist – die
                // Scrollbar steht dann am Ende, die letzte Zeile ist trotzdem
                // angeschnitten, und erst die nächste Nachricht (oder ein
                // Resize) rechnet neu. Konstanter Gutter = deterministischer
                // Umbruch, wie im GameInfoPanel (root.scrollGutter).
                width: msgFlick.width - root.scrollGutter
                text: root.chatModel.join("<br>")
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                persistentSelection: true
                // Der Verlauf darf den Tastaturfokus NIE an sich ziehen: jeder
                // Press auf den Text (Selektion, Drag-Scrollen, Link-/Globus-Tap)
                // würde ihn sonst dem Eingabefeld wegnehmen – die bereits
                // getippte Nachricht bliebe stehen, aber Enter ginge ins Leere.
                // Maus-Selektion funktioniert ohne Fokus weiter (persistent-
                // Selection hält sie sichtbar), Ctrl+C fängt inputField ab.
                activeFocusOnPress: false
                color: root.colText
                selectionColor: root.colAccent
                selectedTextColor: "#101010"
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: root.messageFontSize
                // WICHTIG: Links NICHT über onLinkActivated öffnen. Das TextEdit
                // liegt in einer Flickable, deren childMouseEventFilter den Press
                // abfängt (Flick-Erkennung) – dadurch feuert onLinkActivated nie
                // (genauso wenig wie ein MouseArea.onClicked). Ein TapHandler nimmt
                // dagegen am Grab-Wettbewerb teil und erkennt den Klick zuverlässig,
                // während ein Drag (Text-Selektion) ihn an das TextEdit zurückgibt.
                //
                // Hover/Cursor funktioniert weiter nativ (Flickable filtert nur
                // Maustasten, keine Hover-Events) → hoveredLink ist gesetzt und wird
                // beim Tap direkt zum Öffnen genutzt (kein Koordinaten-Mapping).
                HoverHandler {
                    id: msgHover
                    cursorShape: msgText.hoveredLink !== ""
                                 ? Qt.PointingHandCursor : Qt.IBeamCursor
                    // Das Übersetzen-Symbol folgt dem Mauszeiger von Zeile zu
                    // Zeile (siehe root._updateHoverLine).
                    onPointChanged: root._updateHoverLine(point.position.x,
                                                          point.position.y)
                    // Beim Verlassen ausblenden – aber nicht, während etwas
                    // markiert ist: das Neusetzen des Texts würde die Auswahl
                    // verwerfen, kurz bevor sie kopiert wird.
                    onHoveredChanged: if (!hovered && msgText.selectedText.length === 0)
                                          root._clearHoverLine()
                }
                // Linksklick: Link über die TAP-POSITION ermitteln (linkAt), NICHT
                // über hoveredLink – letzteres wird beim Drücken (Press-/Selektions-
                // Grab) geleert und wäre im onTapped bereits "". Der TapHandler
                // selbst feuert zuverlässig (das Rechtsklick-Menü beweist es).
                TapHandler {
                    id: linkTap
                    acceptedButtons: Qt.LeftButton
                    onTapped: {
                        const link = msgText.linkAt(linkTap.point.position.x,
                                                    linkTap.point.position.y)
                        if (link === "") {
                            // Ohne Maus (Touch) gibt es kein Hover: ein Tipp auf
                            // die Zeile holt ihr Übersetzen-Symbol hervor.
                            root._updateHoverLine(linkTap.point.position.x,
                                                  linkTap.point.position.y)
                            return
                        }
                        // Das Globus-Symbol ist ein Pseudo-Link "pokerthtranslate:<id>".
                        // NICHT extern öffnen, sondern die Zeile übersetzen lassen.
                        if (link.indexOf("pokerthtranslate:") === 0) {
                            if (root.chatTranslator)
                                root.chatTranslator.requestTranslation(
                                    parseInt(link.substring(17)))
                            // Der Tap aufs Symbol darf keine (gelbe) Textauswahl
                            // hinterlassen – Selektion bleibt sonst aber möglich.
                            // callLater: nach dem Neu-Rendern der Zeile abräumen.
                            msgText.deselect()
                            Qt.callLater(msgText.deselect)
                        } else {
                            root._openLink(link)
                        }
                    }
                }
                // Rechtsklick: Menü öffnen und Link unter dem Cursor merken
                // (für „Link öffnen" / „Link kopieren").
                TapHandler {
                    id: ctxTap
                    acceptedButtons: Qt.RightButton
                    onTapped: {
                        const l = msgText.linkAt(ctxTap.point.position.x,
                                                 ctxTap.point.position.y)
                        // Das Übersetzen-Pseudo-Link ist kein echter Link → im
                        // Kontextmenü nicht als „Link öffnen/kopieren" anbieten.
                        root._menuLink = (l.indexOf("pokerthtranslate:") === 0) ? "" : l
                        ctxMenu.popup()
                    }
                }
            }
        }

        // ── Emoji-Picker inline (über der Eingabezeile) ──
        EmojiPicker {
            Layout.fillWidth: true
            Layout.preferredHeight: root.pickerInlineHeight
            visible: root.showEmojiPicker && !root.emojiPickerAsPopup
            onPicked: (emoji) => {
                inputField.insert(inputField.cursorPosition, emoji)
                inputField.forceActiveFocus()
                root.showEmojiPicker = false
            }
        }

        // ── Eingabezeile: Emoji-Toggle · Eingabefeld · Senden ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                Layout.preferredWidth: root.inputHeight
                Layout.preferredHeight: root.inputHeight
                // Kein Klick-Fokus: der Fokus muss im Eingabefeld bleiben,
                // sonst sendet Enter nach dem Auf-/Zuklappen nicht mehr.
                focusPolicy: Qt.NoFocus
                onClicked: root.showEmojiPicker = !root.showEmojiPicker
                background: Rectangle {
                    radius: 6
                    color: root.showEmojiPicker
                           ? root.colBorder : "transparent"
                }
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                contentItem: Text {
                    text: "🙂"
                    font.family: Config.StaticData.emojiFamily
                    font.pixelSize: Math.round(root.inputHeight * 0.55)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            TextField {
                id: inputField
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: root.inputHeight
                enabled: root.inputEnabled
                placeholderText: root.placeholder
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: root.messageFontSize + 1
                color: root.colText
                placeholderTextColor: root.colTextMuted
                background: Rectangle {
                    radius: 6
                    color: Config.Theme.withAlpha(root.colSurface, 0.6)
                    border.color: inputField.activeFocus
                        ? root.colTextSecondary
                        : Config.Theme.withAlpha(root.colTextMuted, 0.6)
                    border.width: 1
                }
                onAccepted: root._send()
                // Begrenzung auf das Server-Byte-Limit – feuert auch bei
                // Einfügen und Emoji-Insert (nicht nur bei Tastatureingabe).
                // Danach die Shortcode-Vorschläge aktualisieren.
                onTextChanged: {
                    root._clampChatInput()
                    root._updateEmoteSuggestions()
                }
                // Cursorbewegung (Pfeil links/rechts, Klick) kann den Token
                // unter dem Cursor ändern → Vorschläge neu berechnen.
                onCursorPositionChanged: root._updateEmoteSuggestions()
                // Tippt der Nutzer: History-Navigation + Tab-Iteration zurücksetzen.
                onTextEdited: {
                    root._historyIndex = 0
                    root._nickState.counter = 0
                    root._emoteSuppressed = false
                }
                // Offenes Shortcode-Popup: Hoch/Runter = Auswahl, Tab/Enter =
                // übernehmen, Esc = ausblenden. Sonst: Tab = Nick-Vervoll-
                // ständigung (iteriert bei wiederholtem Tab), Hoch/Runter = History.
                Keys.onPressed: (event) => {
                    // Der Verlauf hat nie den Fokus (activeFocusOnPress: false),
                    // bekäme also kein Ctrl+C ab. Ist im Eingabefeld selbst nichts
                    // markiert, im Verlauf aber schon, kopiert Ctrl+C die
                    // Verlaufs-Auswahl (sonst täte es hier ohnehin nichts).
                    if (event.key === Qt.Key_C
                            && (event.modifiers & Qt.ControlModifier)
                            && inputField.selectedText.length === 0
                            && msgText.selectedText.length > 0) {
                        event.accepted = true
                        msgText.copy()
                        return
                    }
                    if (emoteSuggestBox.visible) {
                        if (event.key === Qt.Key_Up) {
                            event.accepted = true
                            root._emoteIndex = (root._emoteIndex + root._emoteMatches.length - 1)
                                               % root._emoteMatches.length
                            return
                        } else if (event.key === Qt.Key_Down) {
                            event.accepted = true
                            root._emoteIndex = (root._emoteIndex + 1) % root._emoteMatches.length
                            return
                        } else if (event.key === Qt.Key_Tab
                                   || event.key === Qt.Key_Return
                                   || event.key === Qt.Key_Enter) {
                            event.accepted = true
                            root._acceptEmoteSuggestion()
                            return
                        } else if (event.key === Qt.Key_Escape) {
                            event.accepted = true
                            root._emoteSuppressed = true
                            return
                        }
                    }
                    if (event.key === Qt.Key_Tab) {
                        event.accepted = true
                        var t = Config.StaticData.nickComplete(root._nickState,
                                                               inputField.text, root.nickList)
                        if (t !== null) {
                            inputField.text = t
                            inputField.cursorPosition = t.length
                        }
                    } else if (event.key === Qt.Key_Up) {
                        event.accepted = true
                        // Ein zufällig passender Token am Ende des History-
                        // Eintrags soll das Shortcode-Popup nicht öffnen.
                        root._emoteSuppressed = true
                        if (root._historyIndex + 1 <= root.historyStore.length)
                            root._historyIndex++
                        root._showHistory(root._historyIndex)
                    } else if (event.key === Qt.Key_Down) {
                        event.accepted = true
                        root._emoteSuppressed = true
                        if (root._historyIndex - 1 >= 0)
                            root._historyIndex--
                        root._showHistory(root._historyIndex)
                    }
                }
                // Rechtsklick → Bearbeiten-Menü (Ausschneiden/Kopieren/Einfügen/
                // Alles auswählen). Qt-Quick-Controls-TextFields haben kein
                // eigenes Kontextmenü; passiver TapHandler stört die Eingabe nicht.
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: editMenu.popup()
                }
            }

            Button {
                Layout.preferredWidth: root.inputHeight
                Layout.preferredHeight: root.inputHeight
                enabled: root.inputEnabled && inputField.text.trim().length > 0
                // Kein Klick-Fokus: nach dem Senden per Maus bleibt der Cursor
                // im Eingabefeld, die nächste Nachricht geht direkt per Enter raus.
                focusPolicy: Qt.NoFocus
                onClicked: root._send()
                background: Item {}
                HoverHandler { cursorShape: Qt.PointingHandCursor }
                contentItem: Image {
                    anchors.centerIn: parent
                    width: 18; height: 18
                    source: "../resources/send.svg"
                    sourceSize: Qt.size(36, 36)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    antialiasing: true
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        colorization: 1.0
                        colorizationColor: Config.Theme.colorChatSend
                    }
                }
            }
        }
    }

    // ── Emoji-Picker als Popup über der Box (außerhalb des Layouts) ──
    // Bündig mit dem SICHTBAREN Chat-Rahmen, nicht mit der ChatBox: am Tisch
    // sitzt die ChatBox mit Rand in ihrem Dock-Rechteck – ohne diesen Ausgleich
    // wirkt der Picker schmaler als die Chat-Box und sitzt leicht versetzt.
    Rectangle {
        visible: root.showEmojiPicker && root.emojiPickerAsPopup
        y: -height - 10
        x: root.parent ? -root.x : 0
        width: root.parent ? root.parent.width : root.width
        height: 156
        radius: 10
        z: 50
        color: Config.Theme.withAlpha(root.colBackground, 0.7)
        border.color: root.colBorder
        border.width: 1

        EmojiPicker {
            anchors.fill: parent
            anchors.margins: 3
            // Hintergrund/Rahmen kommen vom Popup-Wrapper.
            color: "transparent"
            border.width: 0
            onPicked: (emoji) => {
                inputField.insert(inputField.cursorPosition, emoji)
                inputField.forceActiveFocus()
                root.showEmojiPicker = false
            }
        }
    }

    // ── Shortcode-Vorschlagsliste (über der Eingabezeile, außerhalb des
    // Layouts – wie das Emoji-Picker-Popup). Präfix-Treffer stehen vor
    // Substring-Treffern; Auswahl per Maus oder Hoch/Runter + Tab/Enter.
    Rectangle {
        id: emoteSuggestBox
        visible: inputField.activeFocus && root._emoteMatches.length > 0
                 && !root._emoteSuppressed
        width: root.width
        height: Math.min(root._emoteMatches.length, 6) * 26 + 8
        y: root.height - root.inputHeight - height - 8
        z: 60
        radius: 8
        color: Config.Theme.withAlpha(root.colBackground, 0.95)
        border.color: root.colBorder
        border.width: 1

        ListView {
            id: emoteSuggestList
            anchors.fill: parent
            anchors.margins: 4
            clip: true
            model: root._emoteMatches
            currentIndex: root._emoteIndex
            // Bei Tastatur-Navigation die Auswahl im Sichtbereich halten.
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: emoteSuggestList.width
                height: 26
                radius: 5
                color: index === root._emoteIndex ? root.colSurface : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 8
                    Text {
                        text: modelData.emoji
                        font.family: Config.StaticData.emojiFamily
                        font.pixelSize: 15
                    }
                    Text {
                        Layout.fillWidth: true
                        text: ":" + modelData.code + ":"
                        elide: Text.ElideRight
                        color: index === root._emoteIndex
                               ? root.colText : root.colTextSecondary
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                    }
                }
                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                    onHoveredChanged: if (hovered) root._emoteIndex = index
                }
                // TapHandler statt MouseArea: nimmt dem Eingabefeld nicht den
                // Fokus weg.
                TapHandler {
                    onTapped: {
                        root._emoteIndex = index
                        root._acceptEmoteSuggestion()
                        inputField.forceActiveFocus()
                    }
                }
            }
        }
    }

    // ── Rechtsklick-Kontextmenü für den Nachrichtenverlauf ──
    // Arbeitet direkt auf dem einen msgText-Dokument (Kopieren / Alles auswählen).
    // Einheitlich gestylter Eintrag (folgt den Farb-Tokens der Box).
    component CtxItem: MenuItem {
        height: visible ? implicitHeight : 0
        contentItem: Text {
            text: parent.text
            color: parent.enabled
                   ? (parent.highlighted ? root.colAccent : root.colText)
                   : root.colTextMuted
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }
        background: Rectangle {
            color: parent.highlighted ? root.colSurface : "transparent"
        }
    }

    Menu {
        id: ctxMenu
        background: Rectangle {
            implicitWidth: 160
            color: root.colBackground
            border.width: 1
            border.color: root.colBorder
            radius: 6
        }

        CtxItem {
            text: qsTr("Link öffnen")
            visible: root._menuLink !== ""
            onTriggered: root._openLink(root._menuLink)
        }
        CtxItem {
            text: qsTr("Link kopieren")
            visible: root._menuLink !== ""
            onTriggered: root._copyToClipboard(root._menuLink)
        }
        CtxItem {
            text: qsTr("Kopieren")
            enabled: msgText.selectedText.length > 0
            onTriggered: msgText.copy()
        }
        CtxItem {
            text: qsTr("Alles auswählen")
            onTriggered: msgText.selectAll()
        }
    }

    // Kopiert beliebigen Text in die Zwischenablage. QML hat keine direkte
    // Clipboard-API – ein unsichtbares TextEdit (selectAll + copy) ist der
    // übliche Weg.
    function _copyToClipboard(text) {
        clipHelper.text = text
        clipHelper.selectAll()
        clipHelper.copy()
        clipHelper.text = ""
    }
    TextEdit { id: clipHelper; visible: false }

    // ── Bearbeiten-Menü für das Eingabefeld (Rechtsklick) ──
    Menu {
        id: editMenu
        background: Rectangle {
            implicitWidth: 160
            color: root.colBackground
            border.width: 1
            border.color: root.colBorder
            radius: 6
        }

        CtxItem {
            text: qsTr("Ausschneiden")
            enabled: !inputField.readOnly && inputField.selectedText.length > 0
            onTriggered: inputField.cut()
        }
        CtxItem {
            text: qsTr("Kopieren")
            enabled: inputField.selectedText.length > 0
            onTriggered: inputField.copy()
        }
        CtxItem {
            text: qsTr("Einfügen")
            enabled: !inputField.readOnly && inputField.canPaste
            onTriggered: inputField.paste()
        }
        CtxItem {
            text: qsTr("Alles auswählen")
            enabled: inputField.length > 0
            onTriggered: inputField.selectAll()
        }
    }
}
