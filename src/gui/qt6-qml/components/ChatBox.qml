import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// A common chat box for ALL chats (lobby compact/wide, game wait,
// the game chat overlay, the docked in-game chat):
//   • a message list (rich text lines) with auto-scroll logic
//     (it pauses when scrolling up, the position is kept on new lines,
//     after 15 s of inactivity it goes back to the end)
//   • an emoji picker – inline above the input line or as a popup above
//     the box (a lack of room, e.g. the docked in-game chat)
//   • an input line with an emoji toggle and a send button
//   • a chat history (arrow up/down, at most 50, as in the Qt widgets client)
//   • Tab nickname completion with iteration (Config.StaticData.nickComplete)
Item {
    id: root

    // ── API ──────────────────────────────────────────────────────────────
    // Liste formatierter (RichText-)Zeilen, z. B. GameTable.chatLog / Lobby.chatLog.
    property var chatModel: []
    // The nicknames for the Tab completion.
    property var nickList: []
    // The chat translator of the corresponding handler (Lobby.chatTranslator or
    // GameTable.chatTranslator). Taps on the globe symbol are routed here;
    // null = no translation (then the symbols do not appear at all).
    property var chatTranslator: null
    property bool inputEnabled: true
    property string placeholder: qsTr("Nachricht …")
    property int messageFontSize: 14
    // The emoji picker as a popup ABOVE the box instead of inline above the input line.
    property bool emojiPickerAsPopup: false
    property int pickerInlineHeight: 150
    property int inputHeight: 36
    property bool showEmojiPicker: false

    // ── Colour tokens (overridable) ─────────────────────────────────────────
    // The default = the global palette, so that the lobby/game wait keep following the
    // light/dark mode. At the table they are overridden with the colours of the table theme
    // (StyleProvider.chatLog*) – there only the theme counts.
    property color colText:          Config.StaticData.palette.secondary.col100
    property color colTextSecondary: Config.StaticData.palette.secondary.col200
    property color colTextMuted:     Config.StaticData.palette.secondary.col400
    property color colBorder:        Config.StaticData.palette.secondary.col500
    property color colSurface:       Config.StaticData.palette.secondary.col600
    property color colBackground:    Config.StaticData.palette.secondary.col700
    property color colAccent:        Config.Theme.colorAccent
    // Text ON the accent (highlighted text). It has to go along with the accent:
    // if that is dark with a light table theme, the selection needs light
    // text instead of the dark text of the gold variant.
    property color colAccentText:    "#101010"
    // The send symbol (green). At the table from the table theme, otherwise the app mode.
    property color colSend:          Config.Theme.colorChatSend

    signal sendRequested(string text)

    function closeEmojiPicker() { showEmojiPicker = false }
    function scrollToEnd() { msgFlick.scrollToBottom() }

    // Opens a link in the external browser. NOT Qt.openUrlExternally directly:
    // in the AppImage/bundle QDesktopServices inherits the bundled LD_LIBRARY_PATH/
    // LD_PRELOAD → xdg-open crashes and nothing opens. LobbyHandler.openExternalUrl
    // starts the host tools (xdg-open/gio/kde-open) with a cleaned environment –
    // exactly like the footer (LobbyStatsBar). Qt.openUrlExternally only as a fallback.
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

    // ── The translate symbol only on the line under the mouse cursor ──────
    // The history is ONE rich text document (not a ListView), so a "line" is
    // the index in chatModel. The entries are joined with <br> and are thus present
    // in the document separated by line separators (U+2028): the
    // line index at a position = the number of separators before it. chatModel
    // entries themselves never contain line breaks (one chat line = one
    // entry), so the mapping is 1:1.
    // _hoverFrom/_hoverTo cache the range of the currently marked line, so that
    // only the (cheap) position query is needed per mouse movement and not
    // counting through the whole history.
    property int _hoverFrom: -1
    property int _hoverTo: -1

    function _isLineSep(code) { return code === 0x2028 || code === 0x2029 }

    function _updateHoverLine(x, y) {
        if (!chatTranslator)
            return
        // While something is selected, do NOT write into the document: showing/
        // hiding the symbol replaces msgText.text completely and would destroy the
        // selection (or a drag in progress).
        if (msgText.selectedText.length > 0)
            return
        var pos = msgText.positionAt(x, y)
        if (pos >= _hoverFrom && pos <= _hoverTo)
            return                       // still the same line
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

    // A new message (or a symbol being shown/hidden) shifts all
    // positions behind it → discard the cached line range.
    onChatModelChanged: { _hoverFrom = -1; _hoverTo = -1 }

    // If the box disappears (a page change lobby/waiting room – both hang off the
    // same translator), a symbol would otherwise stay on the line the mouse
    // passed over last.
    onVisibleChanged: if (!visible) _clearHoverLine()
    Component.onDestruction: _clearHoverLine()

    implicitWidth: 200
    implicitHeight: 160

    // Free space on the right for the vertical scrollbar, so that it never
    // overlaps the content (in the overlay style scrollbars otherwise lie above the text).
    readonly property int scrollGutter: 14

    // When the inline picker unfolds, the list shrinks – scroll to the
    // end, so that the last messages stay visible.
    onShowEmojiPickerChanged: {
        if (showEmojiPicker && !emojiPickerAsPopup)
            Qt.callLater(msgFlick.scrollToBottom)
    }

    // ── History + Tab completion ─────────────────────────────────────────
    // The history storage (sent messages, at most 50). The default: an array of
    // its own per instance. Several ChatBoxes of the same chat channel (e.g.
    // the lobby compact/wide + GameWait) can bind THE SAME array here and
    // thereby share the history – the navigation index stays local.
    property var historyStore: []
    property int _historyIndex: 0
    property var _nickState: ({ counter: 0, base: "", matches: [] })
    // The link under the point that was right-clicked last (for the context menu).
    property string _menuLink: ""

    function _showHistory(idx) {
        if (idx > 0 && idx <= historyStore.length)
            inputField.text = historyStore[historyStore.length - idx]
        else
            inputField.text = ""
        inputField.cursorPosition = inputField.text.length
    }

    // The server checks VALIDATE_STRING_SIZE(chattext, 1, MAX_CHAT_TEXT_SIZE=128)
    // and cuts the connection if it is too long. Like the widgets client
    // (ChatTools::checkInputLength) we therefore limit the input to 128
    // UTF-8 bytes – NOT to a fixed number of characters, since umlauts (2 bytes) and
    // emojis (4 bytes) occupy more than one byte. That way only as much can be
    // entered as may really be sent.
    readonly property int maxChatBytes: 128

    // The UTF-8 byte length of a string (JS strings are UTF-16). Surrogate pairs
    // (emojis) count as 4 bytes and are skipped as a unit via i++.
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

    // Truncates the input character by character until it fits into the server byte limit.
    // It runs on EVERY text change (on pasting/an emoji as well), so that oversized
    // text does not even stay there. Surrogate pairs are removed as a whole,
    // so that no half emoji is left behind.
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

    // ── Shortcode auto-completion (":smi…" → 😄) ─────────────────────────
    // The suggestions come from the same C++ map that replaces them when sending
    // (chat_emote_shortcuts.h via Lobby.chatEmoteShortcodes) – so only what really
    // works is offered. The trigger: ":" + at least
    // 2 lower case letters before the cursor (as in Discord; that way ASCII
    // shortcuts such as ":p"/":s" do not collide with the popup).
    property var _emoteCodes: []
    property var _emoteMatches: []
    property int _emoteIndex: 0
    property int _emoteTokenStart: -1
    // Esc hides the popup until the next input.
    property bool _emoteSuppressed: false

    function _emoteList() {
        if (_emoteCodes.length === 0 && typeof Lobby !== "undefined" && Lobby)
            _emoteCodes = Lobby.chatEmoteShortcodes()
        return _emoteCodes
    }

    function _updateEmoteSuggestions() {
        var upto = inputField.text.slice(0, inputField.cursorPosition)
        // The token = ":" (at the beginning or after a space) + 2+ code characters
        // directly before the cursor. The closing ":" ends the token –
        // so finished shortcodes make the popup disappear by themselves.
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

    // Replaces the typed token (":smi") by the emoji of the chosen
    // suggestion – as an emoji instead of ":smile:", exactly like the emoji picker
    // (WYSIWYG and fewer bytes in the 128 byte server limit).
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

        // ── The message history: ONE contiguous rich text document ──
        // Like the QTextBrowser of the widgets client (chattools.cpp) – instead of a
        // ListView with one TextEdit per line. The advantages: continuous mouse
        // selection over ALL messages (instead of an isolated selection per line)
        // and native, reliable link handling via onLinkActivated.
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
                // Dragging the scrollbar handle sets contentY directly and produces
                // NO movementStarted/Ended signals of the Flickable – which is why
                // it is attached to the same evaluation by hand here.
                onPressedChanged: pressed ? msgFlick.userScrollStarted()
                                          : msgFlick.userScrollEnded()
            }

            // Auto-scroll: it pauses when scrolling up, the position is kept on new
            // lines (the text is replaced completely → the view would
            // otherwise jump), after 15 s of inactivity it goes back to the end.
            property bool autoScroll: true
            property real savedContentY: 0
            Timer {
                id: autoScrollTimer
                interval: 15000
                onTriggered: { msgFlick.autoScroll = true; msgFlick.scrollToBottom() }
            }
            // While scrolling, the lines travel under the standing mouse cursor
            // – the translate symbol has to follow the new line.
            // Delayed, so that this does not run per frame during a flick.
            Timer {
                id: hoverRecheckTimer
                interval: 60
                onTriggered: if (msgHover.hovered)
                                 root._updateHoverLine(msgHover.point.position.x,
                                                       msgHover.point.position.y)
            }
            // Stick to the end. pinBottom() checks autoScroll itself, so that a
            // deferred (Qt.callLater) call does nothing if the user has
            // scrolled away meanwhile.
            function pinBottom() {
                if (autoScroll) contentY = Math.max(0, contentHeight - height)
            }
            // An external jump to the end = "follow along again": first switch autoScroll on,
            // then pin.
            function scrollToBottom() { autoScroll = true; pinBottom() }
            function restoreScroll() {
                contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
            }
            // With auto-scroll, pull to the end TWICE: immediately (contentHeight is
            // already the new value in the change handler) AND once via Qt.callLater.
            // The reason: QQuickTextEdit only updates its implicitHeight in the
            // polish phase and QQuickFlickable updates its INTERNAL
            // scroll limit (max. contentY) only there as well. Depending on the
            // order, the immediate setting is clamped downwards by the limit that is still
            // old – then the callLater applies AFTER the polish to the final
            // limit. One of the two passes always lands correctly; setting
            // it twice to the same final value has no consequence.
            // That was the bug: with a pure Qt.callLater the line typed last stayed
            // below the visible area until the next message.
            function followBottom() {
                // Do not touch anything at all during a running user gesture.
                if (moving || msgScrollBar.pressed)
                    return
                if (autoScroll) { pinBottom(); Qt.callLater(pinBottom) }
                // Paused: keep the remembered position while the text is replaced
                // completely.
                else restoreScroll()
            }
            // Hang off contentHeight: it fires on EVERY height change – a new line,
            // rich text lines wrapping asynchronously and a complete replacement of the text.
            onContentHeightChanged: followBottom()
            // A resize (e.g. a changed number of players) – treat it the same way.
            onHeightChanged: followBottom()

            // ── Derive the auto-scroll state ONLY from real user gestures ─────
            // This used to hang off onContentYChanged (filtered via `moving`).
            // That was the second source of errors: `moving` also applies to the
            // position correction that the Flickable performs itself on EVERY
            // height change (fixupY after setContentHeight – e.g. when the
            // translate symbol makes a line wrap or the emoji
            // picker opens). Such an intermediate value inevitably does not lie
            // at the lower edge → autoScroll flipped to false although the
            // user had done nothing. After that restoreScroll() held the view
            // just above the end: the last line stayed cut off
            // until the 15 s timer or the next message caught it
            // again – and enlarging the box only "repaired" it
            // when savedContentY slipped above the new lower limit in the process.
            // Now: a gesture begins → pause, the gesture has come to REST →
            // decide once, cleanly.
            function userScrollStarted() {
                autoScroll = false
                autoScrollTimer.stop()
            }
            function userScrollEnded() {
                savedContentY = contentY
                // Check with a tolerance instead of an exact atYEnd: close to the end is enough
                // (subpixels/rich text lines growing asynchronously), so that the
                // auto-scroll reliably kicks in again at the lower edge.
                autoScroll = contentY >= contentHeight - height - 4
                if (autoScroll) { autoScrollTimer.stop(); pinBottom() }
                else autoScrollTimer.restart()
            }
            onMovementStarted: userScrollStarted()
            onMovementEnded: userScrollEnded()

            // While scrolling, the lines travel under the standing mouse cursor
            // – independently of who scrolled.
            onContentYChanged: hoverRecheckTimer.restart()

            // A read-only TextEdit holds the whole history as ONE HTML document.
            // The individual chatModel entries are already finished rich text and
            // are strung together with <br>.
            TextEdit {
                id: msgText
                // A fixed free space for the scrollbar – NOT depending on whether
                // it is currently needed. Otherwise the text width would hang off
                // msgFlick.contentHeight, that off msgText.implicitHeight and
                // that in turn (QQuickTextEdit recomputes immediately in
                // geometryChange) off the text width: a real, synchronous binding
                // cycle. QML aborts the re-entrant evaluation in that case, and
                // depending on the entry point contentHeight stays at the OLD
                // value while the document is already taller – the
                // scrollbar then stands at the end, the last line is cut off
                // nevertheless, and only the next message (or a
                // resize) recomputes. A constant gutter = a deterministic
                // wrap, as in the GameInfoPanel (root.scrollGutter).
                width: msgFlick.width - root.scrollGutter
                // Build the document ONLY while the box really is visible.
                // The same history hangs in several ChatBoxes at the same time
                // (the lobby chat: LobbyPage compact + wide + GameWaitPage), and the
                // LobbyPage stays below the waiting room in the StackView. An
                // invisible TextEdit parses its rich text completely anew all the same
                // – so every chat line would have paid for two dead document rebuilds
                // whose cost grows with the length of the history.
                // `visible` is the EFFECTIVE visibility (an invisible
                // parent ⇒ false); when it is shown again the history is caught up
                // in one go.
                text: root.visible ? root.chatModel.join("<br>") : ""
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                persistentSelection: true
                // The history must NEVER take the keyboard focus: every
                // press on the text (a selection, drag scrolling, a link/globe tap)
                // would otherwise take it away from the input field – the message
                // already typed would stay, but Enter would go nowhere.
                // The mouse selection keeps working without the focus (persistent
                // selection keeps it visible), inputField intercepts Ctrl+C.
                activeFocusOnPress: false
                color: root.colText
                selectionColor: root.colAccent
                selectedTextColor: root.colAccentText
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: root.messageFontSize
                // IMPORTANT: do NOT open links via onLinkActivated. The TextEdit
                // lies in a Flickable whose childMouseEventFilter intercepts the press
                // (flick detection) – which is why onLinkActivated never fires
                // (just as little as a MouseArea.onClicked). A TapHandler, by contrast,
                // takes part in the grab competition and detects the click reliably,
                // while a drag (a text selection) hands it back to the TextEdit.
                //
                // Hover/the cursor keep working natively (the Flickable only filters
                // mouse buttons, no hover events) → hoveredLink is set and is used
                // directly for opening on a tap (no coordinate mapping).
                HoverHandler {
                    id: msgHover
                    cursorShape: msgText.hoveredLink !== ""
                                 ? Qt.PointingHandCursor : Qt.IBeamCursor
                    // The translate symbol follows the mouse cursor from line to
                    // line (see root._updateHoverLine).
                    onPointChanged: root._updateHoverLine(point.position.x,
                                                          point.position.y)
                    // Hide it when leaving – but not while something is
                    // selected: resetting the text would discard the selection
                    // shortly before it is copied.
                    onHoveredChanged: if (!hovered && msgText.selectedText.length === 0)
                                          root._clearHoverLine()
                }
                // A left click: determine the link via the TAP POSITION (linkAt), NOT
                // via hoveredLink – the latter is cleared on the press (the press/selection
                // grab) and would already be "" in onTapped. The TapHandler
                // itself fires reliably (the right click menu proves it).
                TapHandler {
                    id: linkTap
                    acceptedButtons: Qt.LeftButton
                    onTapped: {
                        const link = msgText.linkAt(linkTap.point.position.x,
                                                    linkTap.point.position.y)
                        if (link === "") {
                            // Without a mouse (touch) there is no hover: a tap on
                            // the line brings out its translate symbol.
                            root._updateHoverLine(linkTap.point.position.x,
                                                  linkTap.point.position.y)
                            return
                        }
                        // The globe symbol is a pseudo link "pokerthtranslate:<id>".
                        // Do NOT open it externally, but have the line translated.
                        if (link.indexOf("pokerthtranslate:") === 0) {
                            if (root.chatTranslator)
                                root.chatTranslator.requestTranslation(
                                    parseInt(link.substring(17)))
                            // The tap on the symbol must not leave a (yellow) text selection
                            // behind – a selection stays possible otherwise, though.
                            // callLater: clean up after the line has been re-rendered.
                            msgText.deselect()
                            Qt.callLater(msgText.deselect)
                        } else {
                            root._openLink(link)
                        }
                    }
                }
                // A right click: open the menu and remember the link under the cursor
                // (for "open link" / "copy link").
                TapHandler {
                    id: ctxTap
                    acceptedButtons: Qt.RightButton
                    onTapped: {
                        const l = msgText.linkAt(ctxTap.point.position.x,
                                                 ctxTap.point.position.y)
                        // The translate pseudo link is no real link → do not offer it
                        // as "open/copy link" in the context menu.
                        root._menuLink = (l.indexOf("pokerthtranslate:") === 0) ? "" : l
                        ctxMenu.popup()
                    }
                }
            }
        }

        // ── The emoji picker inline (above the input line) ──
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
                // No click focus: the focus has to stay in the input field,
                // otherwise Enter no longer sends after folding it in/out.
                focusPolicy: Qt.NoFocus
                onClicked: root.showEmojiPicker = !root.showEmojiPicker
                // A square box like the input field next to it (the same fill,
                // the same border, the same radius): the emoji is a coloured
                // glyph without a border of its own and would otherwise be lost in the
                // background on light table themes. The resting state stands out via its own
                // surface; unfolded/hovered the box becomes stronger.
                background: Rectangle {
                    radius: 6
                    color: Config.Theme.withAlpha(root.colSurface,
                                                  root.showEmojiPicker ? 0.95
                                                  : (emojiHover.hovered ? 0.8 : 0.6))
                    border.width: 1
                    border.color: root.showEmojiPicker || emojiHover.hovered
                                  ? root.colTextSecondary
                                  : Config.Theme.withAlpha(root.colTextMuted, 0.6)
                }
                HoverHandler { id: emojiHover; cursorShape: Qt.PointingHandCursor }
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
                // The limitation to the server byte limit – it fires on
                // pasting and an emoji insert as well (not only on keyboard input).
                // Afterwards update the shortcode suggestions.
                onTextChanged: {
                    root._clampChatInput()
                    root._updateEmoteSuggestions()
                }
                // A cursor movement (arrow left/right, a click) can change the token
                // under the cursor → recompute the suggestions.
                onCursorPositionChanged: root._updateEmoteSuggestions()
                // If the user types: reset the history navigation + the Tab iteration.
                onTextEdited: {
                    root._historyIndex = 0
                    root._nickState.counter = 0
                    root._emoteSuppressed = false
                }
                // An open shortcode popup: up/down = the selection, Tab/Enter =
                // accept, Esc = hide. Otherwise: Tab = nickname
                // completion (it iterates on a repeated Tab), up/down = the history.
                Keys.onPressed: (event) => {
                    // The history never has the focus (activeFocusOnPress: false),
                    // so it would not get a Ctrl+C. If nothing is selected in the input field
                    // itself but something is in the history, Ctrl+C copies the
                    // history selection (otherwise it would do nothing here anyway).
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
                        // A token at the end of the history entry that happens to match
                        // should not open the shortcode popup.
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
                // A right click → the edit menu (cut/copy/paste/
                // select all). Qt Quick Controls TextFields have no
                // context menu of their own; a passive TapHandler does not disturb the input.
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: editMenu.popup()
                }
            }

            Button {
                Layout.preferredWidth: root.inputHeight
                Layout.preferredHeight: root.inputHeight
                enabled: root.inputEnabled && inputField.text.trim().length > 0
                // No click focus: after sending with the mouse the cursor stays
                // in the input field, the next message goes out directly with Enter.
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
                        colorizationColor: root.colSend
                    }
                }
            }
        }
    }

    // ── The emoji picker as a popup above the box (outside the layout) ──
    // Flush with the VISIBLE chat frame, not with the ChatBox: at the table
    // the ChatBox sits with a margin in its dock rectangle – without this compensation
    // the picker looks narrower than the chat box and sits slightly offset.
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
            // The background/border come from the popup wrapper.
            color: "transparent"
            border.width: 0
            onPicked: (emoji) => {
                inputField.insert(inputField.cursorPosition, emoji)
                inputField.forceActiveFocus()
                root.showEmojiPicker = false
            }
        }
    }

    // ── The shortcode suggestion list (above the input line, outside the
    // layout – like the emoji picker popup). Prefix matches stand before
    // substring matches; the selection is made by mouse or up/down + Tab/Enter.
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
            // Keep the selection in the visible area during keyboard navigation.
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
                // A TapHandler instead of a MouseArea: it does not take the
                // focus away from the input field.
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

    // ── The right click context menu for the message history ──
    // It works directly on the one msgText document (copy / select all).
    // A uniformly styled entry (it follows the colour tokens of the box).
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

    // Copies arbitrary text into the clipboard. QML has no direct
    // clipboard API – an invisible TextEdit (selectAll + copy) is the
    // usual way.
    function _copyToClipboard(text) {
        clipHelper.text = text
        clipHelper.selectAll()
        clipHelper.copy()
        clipHelper.text = ""
    }
    TextEdit { id: clipHelper; visible: false }

    // ── The edit menu for the input field (right click) ──
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
