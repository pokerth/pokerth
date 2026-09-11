import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Inbox for private messages: on the left the conversation partners, on the right the
// history with an input field. Unlike the chat history (in which PMs additionally
// appear as a line), every conversation stays readable on its own here and can be
// continued.
//
// The instance belongs to the main window (pokerth.qml), so that it is the same
// across the pages: the icon in the header and the letter symbol in the
// player list open the same dialog.
Popup {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    // Without focus:true the initial focus from onOpened had no effect and the
    // popup swallowed Escape without closing.
    focus: true

    width: Math.min((parent ? parent.width : 600) * 0.92, 600)
    height: Math.min((parent ? parent.height : 520) * 0.85, 520)

    // Current conversation partner (player name).
    property string activePartner: ""

    readonly property int pmRevision: (typeof Lobby !== "undefined" && Lobby)
                                      ? Lobby.privateMessagesRevision : 0
    readonly property int playerRevision: (typeof Lobby !== "undefined" && Lobby)
                                          ? Lobby.playerListRevision : 0

    readonly property var partners: {
        var _pm = pmRevision, _pl = playerRevision
        return (typeof Lobby !== "undefined" && Lobby) ? Lobby.privateConversationPartners() : []
    }
    readonly property var messages: {
        var _pm = pmRevision
        return (typeof Lobby !== "undefined" && Lobby && activePartner !== "")
                ? Lobby.privateConversation(activePartner) : []
    }
    // 0 = the partner is currently not in the lobby (then nothing can be sent).
    readonly property int activePartnerId: {
        var _pm = pmRevision, _pl = playerRevision
        return (typeof Lobby !== "undefined" && Lobby && activePartner !== "")
                ? Lobby.playerIdByName(activePartner) : 0
    }
    readonly property bool guestMode: (typeof Lobby !== "undefined" && Lobby)
                                      ? Lobby.isMyPlayerGuest : false
    // Guests cannot chat at all on the server side and therefore receive no
    // private messages either – an existing conversation stays readable, but dead.
    readonly property bool partnerIsGuest: {
        var _pm = pmRevision, _pl = playerRevision
        return (typeof Lobby !== "undefined" && Lobby && activePartnerId !== 0)
                ? Lobby.isPlayerGuest(activePartnerId) : false
    }
    // At a running table PMs are deliberately blocked (collusion); the server
    // does not deliver them there anyway.
    readonly property bool atTable: (typeof Lobby !== "undefined" && Lobby)
                                    ? Lobby.atRunningTable : false

    // ── Translation of incoming messages ─────────────────────────────────────
    // Split up as with the chat history: the STATE (translation, currently running,
    // shown/hidden, failed) lives in the handler and travels along in the
    // message entries; QML only re-renders as soon as the handler increments its
    // revision. So only the visibility of the symbol stands here – the switch is
    // the same one as in the chat ("AllowChatTranslation").
    readonly property bool canTranslate: (typeof Translator !== "undefined" && Translator)
                                         ? Translator.enabled : false

    readonly property int maxBytes: 128
    // UTF-8 length (not characters), because the server limits in bytes.
    readonly property int usedBytes: {
        var s = messageInput.text
        var b = 0
        for (var i = 0; i < s.length; ++i) {
            var c = s.charCodeAt(i)
            if (c < 0x80) b += 1
            else if (c < 0x800) b += 2
            else if (c >= 0xd800 && c <= 0xdbff) { b += 4; ++i }  // Surrogat-Paar
            else b += 3
        }
        return b
    }
    readonly property bool canSend: activePartnerId !== 0 && !guestMode && !atTable
                                    && !partnerIsGuest
                                    && messageInput.text.trim() !== ""
                                    && usedBytes <= maxBytes

    // Narrow windows (portrait/phone): the partner list gives way to a selection box.
    readonly property bool wideLayout: width > 430

    // An empty playerName => open the conversation that was active last.
    function openWith(playerName) {
        if (playerName && playerName !== "") {
            if (Lobby) Lobby.ensurePrivateConversation(playerName)
            activePartner = playerName
        } else if (activePartner === "" || !hasPartner(activePartner)) {
            activePartner = partners.length > 0 ? partners[0].name : ""
        }
        markActiveRead()
        open()
    }

    function hasPartner(name) {
        for (var i = 0; i < partners.length; ++i)
            if (partners[i].name === name)
                return true
        return false
    }

    function markActiveRead() {
        if (Lobby && activePartner !== "")
            Lobby.markPrivateConversationRead(activePartner)
    }

    // The inbox belongs to the account you are logged in with: if you log in with a
    // different user, the conversations of the previous one disappear – one that is
    // still selected must not stay then.
    onPartnersChanged: {
        if (activePartner !== "" && !hasPartner(activePartner))
            activePartner = partners.length > 0 ? partners[0].name : ""
    }

    function sendMessage() {
        if (!canSend)
            return
        Lobby.sendPrivateMessageToName(activePartner, messageInput.text.trim())
        messageInput.clear()
    }

    onOpened: {
        markActiveRead()
        messageInput.forceActiveFocus()
        conversationView.positionViewAtEnd()
    }
    onActivePartnerChanged: {
        markActiveRead()
        conversationView.positionViewAtEnd()
    }

    // If a new message of the open conversation arrives, it counts as read
    // immediately (the counter at the top should only count what you do NOT see).
    Connections {
        target: (typeof Lobby !== "undefined") ? Lobby : null
        enabled: root.visible
        function onPrivateMessagesChanged() {
            root.markActiveRead()
            conversationView.positionViewAtEnd()
        }
    }

    // If a hand begins while the inbox is open, it closes:
    // at the table nobody should write privately in the first place.
    onAtTableChanged: if (atTable) close()

    background: Rectangle {
        color: Config.Theme.colorBox
        border.color: Config.StaticData.palette.secondary.col400
        border.width: 1
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ── Kopfzeile ────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 14
            Layout.bottomMargin: 8
            spacing: 8

            SvgIcon {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                source: "qrc:/resources/mail.svg"
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: Config.StaticData.chartColor(3, true)
                }
            }

            AppLabel {
                Layout.fillWidth: true
                text: qsTr("Private messages")
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: 15
                font.bold: true
            }

            // Partner selection on narrow windows (instead of the list on the left).
            ComboBox {
                id: partnerCombo
                visible: !root.wideLayout && root.partners.length > 1
                Layout.preferredWidth: 150
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                model: {
                    var l = []
                    for (var i = 0; i < root.partners.length; ++i) {
                        var p = root.partners[i]
                        l.push(p.unread > 0 ? (p.name + " (" + p.unread + ")") : p.name)
                    }
                    return l
                }
                currentIndex: {
                    for (var i = 0; i < root.partners.length; ++i)
                        if (root.partners[i].name === root.activePartner)
                            return i
                    return -1
                }
                onActivated: (index) => {
                    if (index >= 0 && index < root.partners.length)
                        root.activePartner = root.partners[index].name
                }
            }

            PlayerActionIcon {
                visible: root.activePartner !== ""
                iconSize: 16
                source: "qrc:/resources/trash.svg"
                baseColor: Config.Theme.colorDanger
                tooltipText: qsTr("Delete conversation")
                onTriggered: deleteConversationPopup.openWith(
                    qsTr("Delete conversation"),
                    qsTr("Delete the conversation with \"%1\"?").arg(root.activePartner),
                    qsTr("Delete"))
            }

            PlayerActionIcon {
                iconSize: 16
                source: "qrc:/resources/close.svg"
                baseColor: Config.StaticData.palette.secondary.col200
                tooltipText: qsTr("Close")
                onTriggered: root.close()
            }
        }

        SectionDivider { Layout.fillWidth: true }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Conversation partners ────────────────────────────────────
            Rectangle {
                visible: root.wideLayout && root.partners.length > 1
                Layout.preferredWidth: 160
                Layout.fillHeight: true
                color: Config.Theme.colorPanelRow

                ListView {
                    id: partnerView
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    spacing: 2
                    model: root.partners
                    ScrollBar.vertical: ScrollBar {}

                    delegate: Rectangle {
                        required property var modelData
                        width: partnerView.width
                               - (partnerView.ScrollBar.vertical.visible ? 12 : 0)
                        height: 42
                        radius: 4
                        color: modelData.name === root.activePartner
                               ? Config.Theme.colorHover
                               : (partnerMouse.containsMouse ? Config.Theme.colorHover : "transparent")

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 7
                            anchors.rightMargin: 7
                            spacing: 5

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                AppText {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    font.pixelSize: 12
                                    font.bold: modelData.unread > 0
                                    color: Config.StaticData.palette.secondary.col100
                                    elide: Text.ElideRight
                                }

                                AppText {
                                    Layout.fillWidth: true
                                    text: modelData.lastText
                                    font.pixelSize: 10
                                    color: Config.Theme.colorTextMuted
                                    elide: Text.ElideRight
                                    visible: text !== ""
                                }
                            }

                            Rectangle {
                                visible: modelData.unread > 0
                                Layout.preferredHeight: 14
                                Layout.preferredWidth: Math.max(14, unreadLabel.implicitWidth + 7)
                                radius: 7
                                color: Config.Theme.colorDanger

                                AppText {
                                    id: unreadLabel
                                    anchors.centerIn: parent
                                    text: modelData.unread > 9 ? "9+" : modelData.unread
                                    color: "#FFFFFF"
                                    font.pixelSize: 9
                                    font.bold: true
                                }
                            }
                        }

                        MouseArea {
                            id: partnerMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.activePartner = modelData.name
                        }
                    }
                }
            }

            // ── History + input ──────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    Layout.topMargin: 8
                    spacing: 6

                    AppLabel {
                        Layout.fillWidth: true
                        text: root.activePartner !== ""
                              ? root.activePartner : qsTr("No conversation yet")
                        font.pixelSize: 13
                        font.bold: true
                        color: Config.StaticData.palette.secondary.col200
                        elide: Text.ElideRight
                    }

                    AppText {
                        visible: root.activePartner !== "" && root.activePartnerId === 0
                        text: qsTr("not in the lobby")
                        font.pixelSize: 11
                        color: Config.Theme.colorTextMuted
                    }
                }

                ListView {
                    id: conversationView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    clip: true
                    spacing: 6
                    model: root.messages
                    ScrollBar.vertical: ScrollBar { id: convScrollBar }
                    onCountChanged: positionViewAtEnd()

                    delegate: Item {
                        required property var modelData
                        width: conversationView.width - (convScrollBar.visible ? 12 : 0)
                        height: bubble.height

                        Rectangle {
                            id: bubble

                            readonly property int hPadding: 9
                            readonly property int vPadding: 6
                            readonly property real maxWidth: parent.width * 0.85
                            // The natural (unwrapped) text width is MEASURED
                            // instead of read off the wrapped text:
                            // bubbleText hangs off the bubble width via
                            // Layout.fillWidth. If that in turn came from its
                            // implicitWidth, that would be a loop – Qt resolves it
                            // towards the smaller size, and what was left was the width
                            // of the timestamp, inside which even "hello" wrapped
                            // mid-word. TextMetrics hangs off nothing.
                            readonly property real textWidth:
                                Math.min(Math.ceil(textMetrics.advanceWidth),
                                         maxWidth - 2 * hPadding)

                            // Translation: incoming messages only, and only
                            // if the function is active in the options.
                            readonly property bool translationPending: !!modelData.translationPending
                            readonly property bool translationFailed: !!modelData.translationFailed
                            readonly property bool translationShown:
                                !!modelData.showTranslation
                                && (modelData.translation || "").length > 0
                            readonly property bool showGlobe: !modelData.fromMe
                                                              && root.canTranslate
                                                              && (modelData.text || "").length > 0
                            readonly property int globeSize: 13
                            readonly property int footerSpacing: 4
                            readonly property real footerWidth: timeText.implicitWidth
                                + (showGlobe ? globeSize + footerSpacing : 0)

                            // The bubble has to carry BOTH lines: with a short
                            // message ("hey!") the footer of timestamp
                            // (and possibly the globe) is the wider element – without taking it
                            // into account it would run out of the bubble.
                            readonly property real contentWidth:
                                Math.max(textWidth, footerWidth)

                            width: contentWidth + 2 * hPadding
                            height: bubbleColumn.implicitHeight + 2 * vPadding
                            radius: 8
                            anchors.right: modelData.fromMe ? parent.right : undefined
                            anchors.left: modelData.fromMe ? undefined : parent.left
                            color: modelData.fromMe
                                   ? Config.StaticData.chartColor(3, false)
                                   : Config.Theme.colorPanelRow
                            border.width: 1
                            border.color: modelData.fromMe
                                          ? Config.StaticData.chartColor(3, true)
                                          : Config.StaticData.palette.secondary.col500

                            // Measures the DISPLAYED text without wrapping (see
                            // textWidth above) – including the translation, so that
                            // the bubble grows along when switching.
                            TextMetrics {
                                id: textMetrics
                                font: bubbleText.font
                                text: bubbleText.text
                            }

                            ColumnLayout {
                                id: bubbleColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.leftMargin: bubble.hPadding
                                anchors.rightMargin: bubble.hPadding
                                anchors.topMargin: bubble.vPadding
                                spacing: 2

                                AppText {
                                    id: bubbleText
                                    Layout.fillWidth: true
                                    text: bubble.translationShown ? modelData.translation
                                                                  : (modelData.text || "")
                                    // Italics mark the translation – as in the
                                    // chat history (ChatTranslatorCore.styledTranslation).
                                    font.italic: bubble.translationShown
                                    font.pixelSize: 12
                                    color: Config.StaticData.palette.secondary.col100
                                    wrapMode: Text.Wrap
                                }

                                // Footer: timestamp, for incoming
                                // messages followed by the translate globe in
                                // the lower right corner of the bubble.
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: bubble.footerSpacing

                                    Item { Layout.fillWidth: true }

                                    AppText {
                                        id: timeText
                                        text: modelData.time
                                        font.pixelSize: 9
                                        // On your own coloured bubble the fixed grey of
                                        // colorTextMuted would have too little contrast.
                                        color: modelData.fromMe
                                               ? Config.StaticData.palette.secondary.col200
                                               : Config.Theme.colorTextMuted
                                    }

                                    Item {
                                        visible: bubble.showGlobe
                                        implicitWidth: bubble.globeSize
                                        implicitHeight: bubble.globeSize
                                        Layout.alignment: Qt.AlignVCenter
                                        // Pale and dead during the query, otherwise
                                        // highlighted while the translation stands.
                                        opacity: bubble.translationPending ? 0.4
                                                 : (globeArea.containsMouse || bubble.translationShown ? 1.0 : 0.75)

                                        SvgIcon {
                                            id: globeIcon
                                            anchors.fill: parent
                                            source: "qrc:/resources/globe.svg"
                                            smooth: true
                                            // Colourising via layer.effect instead of a MultiEffect child:
                                            // VectorImage is not a texture provider (see PlayerListItem).
                                            layer.enabled: true
                                            layer.effect: MultiEffect {
                                                colorization: 1.0
                                                colorizationColor: bubble.translationFailed
                                                    ? Config.StaticData.chartColor(5, true)
                                                    : bubble.translationShown
                                                      ? Config.StaticData.chartColor(3, true)
                                                      : Config.Theme.colorTextMuted
                                            }
                                        }

                                        MouseArea {
                                            id: globeArea
                                            anchors.fill: parent
                                            enabled: !bubble.translationPending
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                // The unique msgId of the entry,
                                                // not the list index.
                                                if (Lobby)
                                                    Lobby.togglePrivateMessageTranslation(
                                                        root.activePartner,
                                                        modelData.msgId || 0)
                                            }
                                        }

                                        ToolTip.text: bubble.translationFailed
                                                      ? qsTr("Translation failed.")
                                                      : bubble.translationShown
                                                        ? qsTr("Show original")
                                                        : qsTr("Translate message")
                                        ToolTip.visible: globeArea.containsMouse
                                                         && Config.Parameters.showTooltips
                                        ToolTip.delay: 400
                                    }
                                }
                            }
                        }
                    }
                }

                // A notice instead of the input while nothing can be sent.
                AppText {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    visible: root.guestMode || root.atTable || root.partnerIsGuest
                    text: root.atTable
                          ? qsTr("Private messages are not available at the table.")
                          : root.guestMode
                            ? qsTr("Guests cannot send chat messages")
                            : qsTr("Guests cannot receive private messages.")
                    font.pixelSize: 11
                    color: Config.Theme.colorTextMuted
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    Layout.bottomMargin: 12
                    spacing: 8

                    TextField {
                        id: messageInput
                        Layout.fillWidth: true
                        enabled: root.activePartner !== "" && !root.guestMode && !root.atTable
                                 && !root.partnerIsGuest
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        color: Config.StaticData.palette.secondary.col100
                        placeholderText: qsTr("Message …")
                        placeholderTextColor: Config.StaticData.palette.secondary.col400
                        selectByMouse: true
                        background: Rectangle {
                            color: Config.Theme.colorField
                            border.color: messageInput.activeFocus
                                          ? Config.StaticData.palette.secondary.col300
                                          : Config.StaticData.palette.secondary.col500
                            border.width: 1
                            radius: 4
                        }
                        onAccepted: root.sendMessage()
                    }

                    AppText {
                        text: root.usedBytes + "/" + root.maxBytes
                        font.pixelSize: 10
                        color: root.usedBytes > root.maxBytes
                               ? Config.StaticData.chartColor(5, true)
                               : Config.Theme.colorTextMuted
                    }

                    // Sending as in the lobby/table chat (ChatBox): a square
                    // icon button in the field height. The labelled CustomButton was
                    // wider here at 160×48 than half the input line.
                    Button {
                        Layout.preferredWidth: messageInput.implicitHeight
                        Layout.preferredHeight: messageInput.implicitHeight
                        enabled: root.canSend
                        opacity: enabled ? 1.0 : 0.5
                        // No click focus: after sending with the mouse the
                        // cursor stays in the input field, the next message goes
                        // out directly with Enter.
                        focusPolicy: Qt.NoFocus
                        onClicked: root.sendMessage()
                        background: Item {}
                        HoverHandler {
                            id: sendHover
                            cursorShape: Qt.PointingHandCursor
                        }
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
                        ToolTip.text: qsTr("Send")
                        ToolTip.visible: sendHover.hovered && Config.Parameters.showTooltips
                        ToolTip.delay: 400
                    }
                }
            }
        }
    }

    ConfirmPopup {
        id: deleteConversationPopup
        onConfirmed: {
            if (Lobby && root.activePartner !== "") {
                var gone = root.activePartner
                Lobby.deletePrivateConversation(gone)
                root.activePartner = root.partners.length > 0 ? root.partners[0].name : ""
            }
        }
    }
}
