import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: nicknameAvatarSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: nicknameAvatarSettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Nicknames/Avatars"); topGap: 4 }

        ScrollView {
            id: nickScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width - 12

                // Mein Name / Avatar
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("My Player")

                    ColumnLayout {
                        anchors.fill: parent

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                Layout.preferredWidth: 120
                                text: qsTr("My Nickname:")
                                color: Config.StaticData.palette.secondary.col200
                            }

                            TextField {
                                id: myNicknameField
                                Layout.fillWidth: true
                                maximumLength: 64
                                text: SettingsManager ? SettingsManager.readConfigString("MyName") : ""
                                onEditingFinished: {
                                    if (SettingsManager) SettingsManager.writeConfigString("MyName", text.trim())
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 56
                                Layout.preferredHeight: 56
                                Layout.alignment: Qt.AlignVCenter
                                radius: 6
                                color: Config.StaticData.palette.secondary.col600
                                clip: true

                                Image {
                                    id: myAvatarPreview
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    source: SettingsManager
                                            ? SettingsManager.avatarDisplayUrl(myAvatarField.text)
                                            : ""
                                    fillMode: Image.PreserveAspectCrop
                                    smooth: true
                                    visible: status === Image.Ready
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    text: qsTr("My Avatar:")
                                    color: Config.StaticData.palette.secondary.col200
                                }

                                AppText {
                                    Layout.fillWidth: true
                                    text: myAvatarField.text.length > 0
                                          ? myAvatarField.text.split("/").pop()
                                          : qsTr("No Avatar Selected")
                                    font.pointSize: 10
                                    color: Config.StaticData.palette.secondary.col400
                                    elide: Text.ElideLeft
                                }
                            }

                            TextField {
                                id: myAvatarField
                                visible: false
                                text: SettingsManager ? SettingsManager.readConfigString("MyAvatar") : ""
                            }

                            ColumnLayout {
                                spacing: 4

                                Button {
                                    Layout.fillWidth: true
                                    text: qsTr("Select...")
                                    onClicked: {
                                        if (!SettingsManager) return
                                        let path = SettingsManager.pickImageFile(qsTr("Select avatar"))
                                        if (path) {
                                            myAvatarField.text = path
                                            SettingsManager.writeConfigString("MyAvatar", path)
                                        }
                                    }
                                }

                                Button {
                                    Layout.fillWidth: true
                                    text: qsTr("Examples...")
                                    onClicked: {
                                        avatarPicker.onPicked = function(path) {
                                            myAvatarField.text = path
                                            if (SettingsManager) SettingsManager.writeConfigString("MyAvatar", path)
                                        }
                                        avatarPicker.openPicker()
                                    }
                                }
                            }
                        }
                    }
                }

                // Opponents 1-9 names and avatars
                Repeater {
                    model: 9
                    delegate: GroupBox {
                        Layout.fillWidth: true
                        title: qsTr("Opponent %1").arg(index + 1)

                        ColumnLayout {
                            anchors.fill: parent

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    Layout.preferredWidth: 120
                                    text: qsTr("Nickname:")
                                    color: Config.StaticData.palette.secondary.col200
                                }

                                TextField {
                                    id: opponentNameField
                                    Layout.fillWidth: true
                                    maximumLength: 64
                                    text: SettingsManager ? SettingsManager.readConfigString("Opponent" + (index + 1) + "Name") : ""
                                    onEditingFinished: {
                                        if (SettingsManager) SettingsManager.writeConfigString("Opponent" + (index + 1) + "Name", text.trim())
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Rectangle {
                                    Layout.preferredWidth: 56
                                    Layout.preferredHeight: 56
                                    Layout.alignment: Qt.AlignVCenter
                                    radius: 6
                                    color: Config.StaticData.palette.secondary.col600
                                    clip: true

                                    Image {
                                        id: opponentAvatarPreview
                                        anchors.fill: parent
                                        anchors.margins: 2
                                        source: SettingsManager
                                                ? SettingsManager.avatarDisplayUrl(opponentAvatarField.text)
                                                : ""
                                        fillMode: Image.PreserveAspectCrop
                                        smooth: true
                                        visible: status === Image.Ready
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        text: qsTr("Avatar:")
                                        color: Config.StaticData.palette.secondary.col200
                                    }

                                    AppText {
                                        Layout.fillWidth: true
                                        text: opponentAvatarField.text.length > 0
                                              ? opponentAvatarField.text.split("/").pop()
                                              : qsTr("No Avatar Selected")
                                        font.pointSize: 10
                                        color: Config.StaticData.palette.secondary.col400
                                        elide: Text.ElideLeft
                                    }
                                }

                                TextField {
                                    id: opponentAvatarField
                                    visible: false
                                    text: SettingsManager ? SettingsManager.readConfigString("Opponent" + (index + 1) + "Avatar") : ""
                                }

                                ColumnLayout {
                                    spacing: 4

                                    Button {
                                        Layout.fillWidth: true
                                        text: qsTr("Select...")
                                        onClicked: {
                                            if (!SettingsManager) return
                                            let path = SettingsManager.pickImageFile(qsTr("Select avatar"))
                                            if (path) {
                                                opponentAvatarField.text = path
                                                SettingsManager.writeConfigString("Opponent" + (index + 1) + "Avatar", path)
                                            }
                                        }
                                    }

                                    Button {
                                        Layout.fillWidth: true
                                        text: qsTr("Examples...")
                                        onClicked: {
                                            avatarPicker.onPicked = function(path) {
                                                opponentAvatarField.text = path
                                                if (SettingsManager) SettingsManager.writeConfigString("Opponent" + (index + 1) + "Avatar", path)
                                            }
                                            avatarPicker.openPicker()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    // Gallery of the bundled example avatars. One shared picker for all
    // player fields; the field to be addressed is set via onPicked before
    // opening it.
    ExampleAvatarPicker {
        id: avatarPicker

        property var onPicked: null

        onSelected: function(path) {
            if (onPicked) onPicked(path)
        }
    }
}
