import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Generic yes/no confirmation popup (analogous to MessageBox::question of the
// Qt widgets client). Set the title/text via openWith() before opening; on
// confirmation the confirmed() signal is emitted.
Popup {
    id: root

    // Centre on the window overlay so that the popup also appears in the middle
    // when it is instantiated inside a small delegate (e.g. GameListItem).
    // it is instantiated.
    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 20
    width: Math.min((parent ? parent.width : 360) * 0.85, 340)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    // Without focus:true the popup gets no keyboard input: Escape would be
    // swallowed (the popup stays open, the Escape shortcut of the window does
    // not fire either) and Enter would go nowhere.
    focus: true

    // Initial focus on the confirm button, as in a Windows message box:
    // Enter confirms, Tab moves to cancel, Escape closes.
    onOpened: confirmButton.forceActiveFocus()

    property string title: ""
    property string message: ""
    property string confirmText: qsTr("Yes")
    // false = pure notice dialog: only the confirm button (e.g. "OK"),
    // no cancel (analogous to MessageBox::information/warning).
    property bool showCancel: true

    signal confirmed()

    function openWith(t, m, ct) {
        title = t
        message = m
        if (ct !== undefined) confirmText = ct
        open()
    }

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
                id: cancelButton
                visible: root.showCancel
                text: qsTr("Cancel")
                Layout.fillWidth: true
                onClicked: root.close()
            }

            CustomButton {
                id: confirmButton
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
