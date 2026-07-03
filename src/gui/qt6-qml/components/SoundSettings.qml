import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: soundSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: soundSettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Sound") }

        ScrollView {
            id: soundScrollView
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
                spacing: 12

                // Hauptschalter
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Klangeffekte")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        CustomToggle {
                            id: playSounds
                            label: qsTr("Klangeffekte aktivieren")
                            checked: SettingsManager ? SettingsManager.soundEnabled : false
                            onCheckedChanged: {
                                if (SettingsManager) SettingsManager.soundEnabled = checked
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            enabled: playSounds.checked

                            Label {
                                text: qsTr("Lautstärke:")
                                color: Config.StaticData.palette.secondary.col200
                                opacity: playSounds.checked ? 1.0 : 0.4
                            }

                            Slider {
                                id: soundVolumeSlider
                                Layout.fillWidth: true
                                from: 1
                                to: 10
                                stepSize: 1
                                value: SettingsManager ? SettingsManager.readConfigInt("SoundVolume") : 8
                                enabled: playSounds.checked
                                onMoved: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("SoundVolume", Math.round(value))
                                }
                            }

                            Label {
                                text: Math.round(soundVolumeSlider.value)
                                color: Config.StaticData.palette.secondary.col200
                                opacity: playSounds.checked ? 1.0 : 0.4
                                Layout.minimumWidth: 20
                            }
                        }
                    }
                }

                // Klang-Kategorien
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Klang-Kategorien")
                    enabled: playSounds.checked

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 4

                        ConfigCheckBox {
                            text: qsTr("Spielaktionen (Check, Call, Raise ...)")
                            configKey: "PlayGameActions"
                        }

                        ConfigCheckBox {
                            text: qsTr("Lobby-Chat-Benachrichtigungen")
                            configKey: "PlayLobbyChatNotification"
                        }

                        ConfigCheckBox {
                            text: qsTr("Netzwerkspiel-Benachrichtigungen")
                            configKey: "PlayNetworkGameNotification"
                        }

                        ConfigCheckBox {
                            text: qsTr("Blind-Erhöhungs-Benachrichtigung")
                            configKey: "PlayBlindRaiseNotification"
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
