import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Eingabe-Popup für eine server-weite Durchsage (nur Server-Admins).
// Der Server verteilt die Durchsage als Chat-Broadcast an alle Sessions –
// deshalb gilt hier dieselbe 128-Byte-Grenze wie für Chat-Nachrichten.
Popup {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 20
    width: Math.min((parent ? parent.width : 420) * 0.85, 420)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    readonly property int maxBytes: 128
    // UTF-8-Länge (nicht Zeichen), weil der Server in Bytes begrenzt.
    readonly property int usedBytes: {
        var s = noticeInput.text
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
    readonly property bool canSend: noticeInput.text.trim() !== "" && usedBytes <= maxBytes

    signal accepted(string noticeText)

    function openWith() {
        noticeInput.clear()
        open()
    }

    onOpened: noticeInput.forceActiveFocus()

    background: Rectangle {
        color: Config.Theme.colorBox
        border.color: Config.StaticData.palette.secondary.col400
        border.width: 1
        radius: 8
    }

    ColumnLayout {
        spacing: 12
        width: root.availableWidth

        AppLabel {
            Layout.fillWidth: true
            text: qsTr("Global notice")
            color: Config.StaticData.palette.secondary.col100
            font.pixelSize: 15
            font.bold: true
            wrapMode: Text.WordWrap
        }

        AppLabel {
            Layout.fillWidth: true
            text: qsTr("This message is shown to every player on the server, in the lobby and at the tables.")
            color: Config.StaticData.palette.secondary.col300
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        TextField {
            id: noticeInput
            Layout.fillWidth: true
            font.family: Config.StaticData.loadedFont.font.family
            font.pixelSize: 13
            color: Config.StaticData.palette.secondary.col100
            placeholderText: qsTr("Notice text …")
            placeholderTextColor: Config.StaticData.palette.secondary.col400
            selectByMouse: true
            background: Rectangle {
                color: Config.Theme.colorField
                border.color: noticeInput.activeFocus
                              ? Config.StaticData.palette.secondary.col300
                              : Config.StaticData.palette.secondary.col500
                border.width: 1
                radius: 4
            }
            onAccepted: if (root.canSend) root.sendNotice()
        }

        AppLabel {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            text: qsTr("%1 / %2 characters").arg(root.usedBytes).arg(root.maxBytes)
            font.pixelSize: 11
            color: root.usedBytes > root.maxBytes
                   ? Config.StaticData.chartColor(5, true)
                   : Config.StaticData.palette.secondary.col400
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            CustomButton {
                text: qsTr("Cancel")
                Layout.fillWidth: true
                onClicked: root.close()
            }

            CustomButton {
                text: qsTr("Send")
                enabled: root.canSend
                opacity: enabled ? 1.0 : 0.5
                Layout.fillWidth: true
                onClicked: root.sendNotice()
            }
        }
    }

    function sendNotice() {
        if (!canSend)
            return
        accepted(noticeInput.text.trim())
        close()
    }
}
