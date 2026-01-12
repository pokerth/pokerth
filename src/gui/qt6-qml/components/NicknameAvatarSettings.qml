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

        Label {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            horizontalAlignment: Text.AlignLeft
            text: qsTr("Nicknamen/Avatare")
            font.bold: true
            font.pointSize: 12
            color: Config.StaticData.palette.secondary.col200
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.fillHeight: false
            Layout.topMargin: 0
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.alignment: Qt.AlignTop
            color: Config.StaticData.palette.secondary.col500
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            clip: true

            ColumnLayout {
                width: parent.width

                // Mein Name / Avatar
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Mein Spieler")

                    ColumnLayout {
                        anchors.fill: parent

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                Layout.preferredWidth: 120
                                text: qsTr("Mein Nickname:")
                                color: Config.StaticData.palette.secondary.col200
                            }

                            TextField {
                                id: myNicknameField
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("MyName") : ""
                                onEditingFinished: {
                                    if (SettingsManager) SettingsManager.writeConfigString("MyName", text.trim())
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                Layout.preferredWidth: 120
                                text: qsTr("Mein Avatar:")
                                color: Config.StaticData.palette.secondary.col200
                            }

                            TextField {
                                id: myAvatarField
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("MyAvatar") : ""
                                readOnly: true
                            }

                            Button {
                                text: qsTr("Auswählen...")
                                onClicked: {
                                    // TODO: Avatar-Auswahl-Dialog implementieren
                                }
                            }
                        }
                    }
                }

                // Gegner 1-9 Namen und Avatare
                Repeater {
                    model: 9
                    delegate: GroupBox {
                        Layout.fillWidth: true
                        title: qsTr("Gegner %1").arg(index + 1)

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
                                    text: SettingsManager ? SettingsManager.readConfigString("Opponent" + (index + 1) + "Name") : ""
                                    onEditingFinished: {
                                        if (SettingsManager) SettingsManager.writeConfigString("Opponent" + (index + 1) + "Name", text.trim())
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    Layout.preferredWidth: 120
                                    text: qsTr("Avatar:")
                                    color: Config.StaticData.palette.secondary.col200
                                }

                                TextField {
                                    id: opponentAvatarField
                                    Layout.fillWidth: true
                                    text: SettingsManager ? SettingsManager.readConfigString("Opponent" + (index + 1) + "Avatar") : ""
                                    readOnly: true
                                }

                                Button {
                                    text: qsTr("Auswählen...")
                                    onClicked: {
                                        // TODO: Avatar-Auswahl-Dialog implementieren
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
}
