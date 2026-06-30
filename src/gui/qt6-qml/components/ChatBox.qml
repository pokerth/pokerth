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
    property bool inputEnabled: true
    property string placeholder: qsTr("Nachricht …")
    property int messageFontSize: 12
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

    implicitWidth: 200
    implicitHeight: 160

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
            function scrollToBottom() { contentY = Math.max(0, contentHeight - height) }
            function restoreScroll() {
                contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
            }
            // Hält die View am Ende (Auto-Scroll) bzw. an der gemerkten Position.
            // Per Qt.callLater entkoppelt, damit es NACH dem Layout läuft (finale
            // contentHeight) und mehrere Höhen-Updates zu einem Aufruf bündelt.
            function followBottom() {
                if (autoScroll) scrollToBottom()
                // Pausiert: NUR wiederherstellen, wenn der Nutzer nicht gerade
                // selbst scrollt – sonst klemmt das restoreScroll die Bewegung fest.
                else if (!moving && !msgScrollBar.pressed) restoreScroll()
            }
            // An contentHeight hängen: feuert bei JEDER Höhenänderung – neue Zeile,
            // async umbrechende RichText-Zeilen und komplettes Ersetzen des Texts.
            onContentHeightChanged: Qt.callLater(followBottom)
            // Resize (z. B. geänderte Spieleranzahl) – gleich behandeln.
            onHeightChanged: Qt.callLater(followBottom)
            // Nur benutzergetriebene Bewegungen werten (Flick/Wheel sowie
            // Scrollbar-Ziehen) – NICHT das programmatische Positionieren.
            onContentYChanged: {
                if (!moving && !msgScrollBar.pressed) return
                savedContentY = contentY
                // Mit Toleranz prüfen statt exaktem atYEnd: knapp am Ende reicht
                // (Subpixel/async wachsende RichText-Zeilen), damit der
                // Auto-Scroll am unteren Rand zuverlässig wieder anspringt.
                if (contentY >= contentHeight - height - 4) {
                    autoScroll = true; autoScrollTimer.stop()
                } else {
                    autoScroll = false; autoScrollTimer.restart()
                }
            }

            // Read-only TextEdit hält den gesamten Verlauf als EIN HTML-Dokument.
            // Die einzelnen chatModel-Einträge sind bereits fertiges RichText und
            // werden mit <br> aneinandergereiht.
            TextEdit {
                id: msgText
                // Platz für die Scrollbar lassen, wenn sie sichtbar ist.
                width: msgFlick.width
                       - (msgFlick.contentHeight > msgFlick.height + 4 ? 12 : 0)
                text: root.chatModel.join("<br>")
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                readOnly: true
                selectByMouse: true
                persistentSelection: true
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
                    cursorShape: msgText.hoveredLink !== ""
                                 ? Qt.PointingHandCursor : Qt.IBeamCursor
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
                        if (link !== "")
                            root._openLink(link)
                    }
                }
                // Rechtsklick: Menü öffnen und Link unter dem Cursor merken
                // (für „Link öffnen" / „Link kopieren").
                TapHandler {
                    id: ctxTap
                    acceptedButtons: Qt.RightButton
                    onTapped: {
                        root._menuLink = msgText.linkAt(ctxTap.point.position.x,
                                                        ctxTap.point.position.y)
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
                // Tippt der Nutzer: History-Navigation + Tab-Iteration zurücksetzen.
                onTextEdited: {
                    root._historyIndex = 0
                    root._nickState.counter = 0
                }
                // Tab = Nick-Vervollständigung (iteriert bei wiederholtem Tab);
                // Hoch/Runter = History.
                Keys.onPressed: (event) => {
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
                        if (root._historyIndex + 1 <= root.historyStore.length)
                            root._historyIndex++
                        root._showHistory(root._historyIndex)
                    } else if (event.key === Qt.Key_Down) {
                        event.accepted = true
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
    Rectangle {
        visible: root.showEmojiPicker && root.emojiPickerAsPopup
        y: -height - 10
        width: root.width
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
