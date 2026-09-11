import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Edit the note + rating for a fellow player (port of the star/tooltip system
// from the Qt widgets client, MyAvatarLabel). Saving goes through the
// SettingsManager into the config list "PlayerTooltips" – the same storage
// the widgets client reads and writes.
Popup {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    modal: true
    padding: 20
    width: Math.min((parent ? parent.width : 420) * 0.9, 420)
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    // Without focus:true the initial focus from onOpened had no effect and the
    // popup swallowed Escape without closing.
    focus: true

    // Player whose note is being edited (the key in the storage is the name).
    property string playerName: ""

    // Limit the note length: the entry ends up as a single line in the config.xml.
    readonly property int maxNoteLength: 500

    // Load the existing note/rating and open.
    function openFor(name) {
        playerName = name
        var sm = (typeof SettingsManager !== "undefined") ? SettingsManager : null
        stars.rating = sm ? sm.playerRating(name) : 0
        noteInput.text = sm ? sm.playerNote(name) : ""
        open()
    }

    function save() {
        if (typeof SettingsManager !== "undefined" && SettingsManager)
            SettingsManager.setPlayerNote(root.playerName, noteInput.text, stars.rating)
        close()
    }

    onOpened: noteInput.forceActiveFocus()

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
            text: qsTr("Note about \"%1\"").arg(root.playerName)
            color: Config.StaticData.palette.secondary.col100
            font.pixelSize: 15
            font.bold: true
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            AppLabel {
                text: qsTr("Rating:")
                color: Config.StaticData.palette.secondary.col300
                font.pixelSize: 13
            }

            PlayerRatingStars {
                id: stars
                starSize: 22
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 110
            color: Config.StaticData.palette.secondary.col700
            border.color: noteInput.activeFocus ? Config.Theme.colorAccent
                                                : Config.StaticData.palette.secondary.col500
            border.width: 1
            radius: Config.Theme.radiusSmall

            ScrollView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true

                TextArea {
                    id: noteInput
                    // Multi-line field: Enter makes a paragraph, saving is done
                    // with Ctrl+Enter (as usual in chat/note fields).
                    Keys.onReturnPressed: (event) => {
                        if (event.modifiers & Qt.ControlModifier)
                            root.save()
                        else
                            event.accepted = false
                    }
                    Keys.onEnterPressed: (event) => {
                        if (event.modifiers & Qt.ControlModifier)
                            root.save()
                        else
                            event.accepted = false
                    }
                    background: null
                    selectByMouse: true
                    wrapMode: TextArea.Wrap
                    color: Config.StaticData.palette.secondary.col100
                    placeholderTextColor: Config.Theme.colorTextMuted
                    placeholderText: qsTr("Your private note about this player ...")
                    font.pixelSize: 13
                    // Only your own note – the text is never transmitted.
                    onTextChanged: {
                        if (length > root.maxNoteLength)
                            remove(root.maxNoteLength, length)
                    }
                }
            }
        }

        AppLabel {
            Layout.fillWidth: true
            text: qsTr("Notes and ratings are stored locally and are only visible to you.")
            color: Config.Theme.colorTextMuted
            font.pixelSize: 11
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
                text: qsTr("Save")
                Layout.fillWidth: true
                onClicked: root.save()
            }
        }
    }
}
