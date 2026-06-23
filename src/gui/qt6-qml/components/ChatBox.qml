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
    // Nachrichten-Hintergrund (Bubbles); Default: nur Text auf Box-Hintergrund.
    property bool showBubbles: false
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
    function scrollToEnd() { msgList.positionViewAtEnd() }

    implicitWidth: 200
    implicitHeight: 160

    // Beim Aufklappen des Inline-Pickers schrumpft die Liste – ans Ende
    // scrollen, damit die letzten Nachrichten sichtbar bleiben.
    onShowEmojiPickerChanged: {
        if (showEmojiPicker && !emojiPickerAsPopup)
            Qt.callLater(msgList.positionViewAtEnd)
    }

    // ── History + Tab-Vervollständigung ──────────────────────────────────
    // History-Speicher (gesendete Nachrichten, max. 50). Default: eigenes
    // Array pro Instanz. Mehrere ChatBoxen desselben Chat-Kanals (z. B.
    // Lobby compact/wide + GameWait) können hier DASSELBE Array binden und
    // teilen sich damit die History – der Navigationsindex bleibt lokal.
    property var historyStore: []
    property int _historyIndex: 0
    property var _nickState: ({ counter: 0, base: "", matches: [] })

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

        // ── Nachrichtenliste ──
        ListView {
            id: msgList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: root.showBubbles ? 3 : 1
            model: root.chatModel
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: msgList.contentHeight > msgList.height + 4
                        ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
            }
            // Auto-Scroll: pausiert beim Hochscrollen, Position bleibt bei
            // neuen Zeilen erhalten (das Model wird als QVariantList komplett
            // ersetzt → die View würde sonst nach oben springen), nach 15 s
            // Inaktivität wieder ans Ende.
            property bool autoScroll: true
            property real savedContentY: 0
            Timer {
                id: autoScrollTimer
                interval: 15000
                onTriggered: { msgList.autoScroll = true; msgList.positionViewAtEnd() }
            }
            function restoreScroll() {
                contentY = Math.min(savedContentY, Math.max(0, contentHeight - height))
            }
            onContentYChanged: {
                if (!moving) return
                savedContentY = contentY
                if (atYEnd) { autoScroll = true; autoScrollTimer.stop() }
                else        { autoScroll = false; autoScrollTimer.restart() }
            }
            onCountChanged: {
                if (autoScroll) positionViewAtEnd()
                else Qt.callLater(restoreScroll)
            }

            delegate: Item {
                required property var modelData
                width: ListView.view.width
                implicitHeight: bubbleRect.height

                Rectangle {
                    id: bubbleRect
                    width: parent.width
                    height: msgText.implicitHeight + (root.showBubbles ? 4 : 2)
                    radius: root.showBubbles ? 6 : 0
                    color: root.showBubbles
                           ? Config.Theme.withAlpha(root.colSurface, 0.55)
                           : "transparent"

                    // TextEdit statt Text: macht die Nachricht per Maus
                    // selektier-/kopierbar (Ctrl+C; Selektion je Nachricht).
                    TextEdit {
                        id: msgText
                        anchors {
                            left: parent.left; right: parent.right; top: parent.top
                            leftMargin: root.showBubbles ? 6 : 0
                            // Rechts zusätzlich Platz für die Scrollbar lassen, damit
                            // sie den Text nicht überlappt (nur wenn sie sichtbar ist).
                            rightMargin: (root.showBubbles ? 6 : 0)
                                         + (msgList.contentHeight > msgList.height + 4 ? 12 : 0)
                            topMargin: root.showBubbles ? 2 : 1
                        }
                        text: modelData
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                        readOnly: true
                        selectByMouse: true
                        color: root.colText
                        selectionColor: root.colAccent
                        selectedTextColor: "#101010"
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: root.messageFontSize
                        onLinkActivated: (link) => Qt.openUrlExternally(link)

                        // Cursor: Zeiger über Links, sonst Text-Auswahl-Balken.
                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.NoButton
                            cursorShape: msgText.hoveredLink !== ""
                                         ? Qt.PointingHandCursor : Qt.IBeamCursor
                        }
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
}
