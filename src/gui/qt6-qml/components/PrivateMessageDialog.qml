import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Posteingang für private Nachrichten: links die Gesprächspartner, rechts der
// Verlauf mit Eingabefeld. Anders als der Chat-Verlauf (in dem PMs zusätzlich
// als Zeile stehen) bleibt hier jedes Gespräch für sich lesbar und lässt sich
// fortsetzen.
//
// Die Instanz gehört dem Hauptfenster (pokerth.qml), damit sie über die Seiten
// hinweg dieselbe ist: Icon in der Kopfzeile und Brief-Symbol in der
// Spielerliste öffnen denselben Dialog.
Popup {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: Math.min((parent ? parent.width : 600) * 0.92, 600)
    height: Math.min((parent ? parent.height : 520) * 0.85, 520)

    // Aktueller Gesprächspartner (Spielername).
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
    // 0 = Partner ist gerade nicht in der Lobby (dann kann nichts gesendet werden).
    readonly property int activePartnerId: {
        var _pm = pmRevision, _pl = playerRevision
        return (typeof Lobby !== "undefined" && Lobby && activePartner !== "")
                ? Lobby.playerIdByName(activePartner) : 0
    }
    readonly property bool guestMode: (typeof Lobby !== "undefined" && Lobby)
                                      ? Lobby.isMyPlayerGuest : false
    // Am laufenden Tisch sind PMs bewusst gesperrt (Absprachen); der Server
    // stellt sie dorthin ohnehin nicht zu.
    readonly property bool atTable: (typeof Lobby !== "undefined" && Lobby)
                                    ? Lobby.atRunningTable : false

    readonly property int maxBytes: 128
    // UTF-8-Länge (nicht Zeichen), weil der Server in Bytes begrenzt.
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
                                    && messageInput.text.trim() !== ""
                                    && usedBytes <= maxBytes

    // Schmale Fenster (Portrait/Handy): Partnerliste weicht einer Auswahlbox.
    readonly property bool wideLayout: width > 430

    // playerName leer => zuletzt aktives Gespräch öffnen.
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

    // Trifft eine neue Nachricht des offenen Gesprächs ein, gilt sie sofort als
    // gelesen (der Zähler oben soll nur zählen, was man NICHT sieht).
    Connections {
        target: (typeof Lobby !== "undefined") ? Lobby : null
        enabled: root.visible
        function onPrivateMessagesChanged() {
            root.markActiveRead()
            conversationView.positionViewAtEnd()
        }
    }

    // Beginnt eine Hand, während der Posteingang offen steht, schließt er sich:
    // am Tisch soll gar nicht erst privat geschrieben werden.
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

            // Partnerwahl auf schmalen Fenstern (statt der Liste links).
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

            // ── Gesprächspartner ─────────────────────────────────────────
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

            // ── Verlauf + Eingabe ────────────────────────────────────────
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
                            // Die natürliche (ungebrochene) Textbreite wird
                            // GEMESSEN statt am gewrappten Text abgelesen:
                            // bubbleText hängt per Layout.fillWidth an der
                            // Blasenbreite. Käme die ihrerseits aus dessen
                            // implicitWidth, wäre das eine Schleife – Qt löst sie
                            // zur kleineren Größe hin auf, übrig blieb die Breite
                            // des Zeitstempels, und darin brach selbst "hello"
                            // mitten im Wort um. TextMetrics hängt an nichts.
                            readonly property real textWidth:
                                Math.min(Math.ceil(textMetrics.advanceWidth),
                                         maxWidth - 2 * hPadding)
                            // Die Blase muss BEIDE Zeilen tragen: bei einer kurzen
                            // Nachricht ("hey!") ist der Zeitstempel das breitere
                            // Element – ohne ihn zu berücksichtigen liefe er aus
                            // der Blase heraus.
                            readonly property real contentWidth:
                                Math.max(textWidth, timeText.implicitWidth)

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

                            // Misst den Text ohne Umbruch (siehe textWidth oben).
                            TextMetrics {
                                id: textMetrics
                                font: bubbleText.font
                                text: modelData.text
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
                                    text: modelData.text
                                    font.pixelSize: 12
                                    color: Config.StaticData.palette.secondary.col100
                                    wrapMode: Text.Wrap
                                }

                                AppText {
                                    id: timeText
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignRight
                                    text: modelData.time
                                    font.pixelSize: 9
                                    // Auf der eingefärbten eigenen Blase wäre das
                                    // feste Grau von colorTextMuted zu kontrastarm.
                                    color: modelData.fromMe
                                           ? Config.StaticData.palette.secondary.col200
                                           : Config.Theme.colorTextMuted
                                }
                            }
                        }
                    }
                }

                // Hinweis statt Eingabe, solange nichts gesendet werden kann.
                AppText {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    visible: root.guestMode || root.atTable
                    text: root.atTable
                          ? qsTr("Private messages are not available at the table.")
                          : qsTr("Guests cannot send chat messages")
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

                    CustomButton {
                        text: qsTr("Send")
                        enabled: root.canSend
                        opacity: enabled ? 1.0 : 0.5
                        onClicked: root.sendMessage()
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
