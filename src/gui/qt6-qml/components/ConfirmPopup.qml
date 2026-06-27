import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Generisches Ja/Nein-Bestätigungs-Popup (analog zum MessageBox::question
// des Qt-Widgets-Clients). Vor dem Öffnen Titel/Text über openWith() setzen;
// bei Bestätigung wird das confirmed()-Signal emittiert.
Popup {
    id: root

    // Auf das Fenster-Overlay zentrieren, damit das Popup auch dann mittig
    // erscheint, wenn es in einem kleinen Delegate (z. B. GameListItem)
    // instanziiert wird.
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 20
    width: Math.min((parent ? parent.width : 360) * 0.85, 340)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string title: ""
    property string message: ""
    property string confirmText: qsTr("Yes")

    signal confirmed()

    function openWith(t, m, ct) {
        title = t
        message = m
        if (ct !== undefined) confirmText = ct
        open()
    }

    background: Rectangle {
        color: Config.StaticData.palette.secondary.col700
        border.color: Config.StaticData.palette.secondary.col400
        border.width: 1
        radius: 8
    }

    ColumnLayout {
        spacing: 12
        width: root.availableWidth

        AppLabel {
            Layout.fillWidth: true
            text: root.title
            color: Config.StaticData.palette.secondary.col100
            font.pixelSize: 15
            font.bold: true
            wrapMode: Text.WordWrap
        }

        AppLabel {
            Layout.fillWidth: true
            text: root.message
            color: Config.StaticData.palette.secondary.col300
            font.pixelSize: 12
            wrapMode: Text.WordWrap
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
                text: root.confirmText
                Layout.fillWidth: true
                onClicked: {
                    root.confirmed()
                    root.close()
                }
            }
        }
    }
}
