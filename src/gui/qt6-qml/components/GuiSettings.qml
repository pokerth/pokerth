import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: guiSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    ColumnLayout {
        id: guiSettingsContent
        anchors.fill: parent

        Label {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 8
            Layout.bottomMargin: 0
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.fillHeight: false
            horizontalAlignment: Text.AlignLeft
            text: qsTr("Benutzeroberfläche")
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

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12

            CustomTabBar {
                id: guiSettingsTabBar
                model: [qsTr("Gemeinsam"), qsTr("Netzwerk-/Internetspiel")]
            }

            StackLayout {
                width: parent.width
                currentIndex: guiSettingsTabBar.currentIndex

                ColumnLayout {
                    id: generalTab

                    RowLayout {
                        id: language
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.topMargin: 16

                        Label {
                            Layout.preferredHeight: 24
                            Layout.fillHeight: false
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("Sprache:")
                            color: Config.StaticData.palette.secondary.col200
                            font.pointSize: 12
                        }

                        CustomComboBox {
                            id: languageSelector
                            model: Config.StaticData.languages
                            Component.onCompleted: {
                                if (SettingsManager) {
                                    var lang = SettingsManager.language
                                    var idx = model.indexOf(lang)
                                    if (idx >= 0) currentIndex = idx
                                }
                            }
                            onCurrentTextChanged: {
                                if (SettingsManager && currentText && currentText !== "")
                                    SettingsManager.language = currentText
                            }
                        }
                    }

                    CustomCheckBox {
                        objectName: "displayRightToolboxCheckbox"
                        label: qsTr("Rechte Toolbox anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowRightToolBox") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowRightToolBox", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "displayLeftToolboxCheckbox"
                        label: qsTr("Linke Toolbox anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowLeftToolBox") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowLeftToolBox", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "fadeOutLosingCardsAnimationCheckbox"
                        label: qsTr("Ausblend-Abination für Verliererkarten")
                        checked: SettingsManager ? SettingsManager.readConfigInt("FadeOutPlayersAnimation") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("FadeOutPlayersAnimation", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "animatedCardsCheckbox"
                        label: qsTr("Animierte Karten")
                        checked: SettingsManager ? SettingsManager.readConfigInt("FlipCardsAnimation") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("FlipCardsAnimation", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "reverseFKeysOrderCheckbox"
                        label: qsTr("F-Tasten-Reihenfolge umkehren (F1 - F4)")
                        checked: SettingsManager ? SettingsManager.readConfigInt("AntiPeekMode") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("AntiPeekMode", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "showBlindButtonsCheckbox"
                        label: qsTr("Symbole für Small Blind und Big Blind anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowBlindButtons") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowBlindButtons", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "showHandChanceMonitorCheckbox"
                        label: qsTr("Kartenchancenmonitor anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowCardsChanceMonitor") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowCardsChanceMonitor", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "showOwnCardsOnMouseClickCheckbox"
                        label: qsTr("Eigene Karten nur bei Mausklick anzeigen anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowMyCardsOnClick") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowMyCardsOnClick", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "disableSplashScreenOnStartupCheckbox"
                        label: qsTr("Startbildschirm beim Startvorgang deaktivieren")
                        checked: SettingsManager ? SettingsManager.disableSplashScreen : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.disableSplashScreen = checked }
                    }

                    CustomCheckBox {
                        objectName: "doNotTranslatePokerTermsCheckbox"
                        label: qsTr("Pokerausdrücke - wie Check, Call und Raise - beim Spieltisch-Stil nicht übersetzen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DontTranslateInternationalPokerActionsInChat") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("DontTranslateInternationalPokerActionsInChat", checked ? 1 : 0) }
                    }
                }

                ColumnLayout {
                    id: networkTab

                    CustomCheckBox {
                        objectName: "showCountryFlagOnAvatarCheckbox"
                        label: qsTr("Landesflagge in der Ecke des Avatars anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowCountryFlagInAvatar") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowCountryFlagInAvatar", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "showNetworkStatusColorOnAvatarCheckbox"
                        label: qsTr("Netzwerkstatus-Farbe in der Ecke des Avatars anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowPingStateInAvatar") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowPingStateInAvatar", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "focusBetInputOnTurnCheckbox"
                        label: qsTr("Cursor ins \"Biete\"-Eingabefeld setzen, wenn Sie an der Reihe sind")
                        checked: SettingsManager ? SettingsManager.readConfigInt("AccentMyTurnPulseOnOff") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("AccentMyTurnPulseOnOff", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "preventAccidentalCallAfterBigRaiseCheckbox"
                        label: qsTr("Versehentliches Call nach einem großen Raise verhindern")
                        checked: SettingsManager ? SettingsManager.readConfigInt("AntiPeekMode") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("AntiPeekMode", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "doNotHideIgnoredPlayerAvatarsCheckbox"
                        label: qsTr("Avatare von ignorierten Spielern nicht verbergen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DontHideAvatarsOfIgnored") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("DontHideAvatarsOfIgnored", checked ? 1 : 0) }
                    }

                    CustomCheckBox {
                        objectName: "showLobbyChatCheckbox"
                        label: qsTr("Lobby-Chat anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowGameChatOnly") === 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowGameChatOnly", checked ? 0 : 1) }
                    }

                    CustomCheckBox {
                        objectName: "disableEmoticonsInChatCheckbox"
                        label: qsTr("Emoticons im Chat deaktivieren")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DisableChatEmoticons") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("DisableChatEmoticons", checked ? 1 : 0) }
                    }
                }
            }
        }
    }
}
