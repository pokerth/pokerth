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
                model: [qsTr("Allgemein"), qsTr("Netzwerk")]
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: guiSettingsTabBar.currentIndex

                ScrollView {
                    id: generalTab
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.topMargin: 16

                        Label {
                            text: qsTr("Dark Mode:")
                            color: Config.StaticData.palette.secondary.col200
                            font.pointSize: 12
                        }

                        ComboBox {
                            id: darkModeSelector
                            model: [qsTr("Automatisch"), qsTr("Hell"), qsTr("Dunkel")]
                            // Config: 0=Hell, 1=Dunkel, 2=Auto → Index: 0=Auto, 1=Hell, 2=Dunkel
                            Component.onCompleted: {
                                if (SettingsManager) {
                                    var v = SettingsManager.readConfigInt("DarkMode")
                                    currentIndex = (v === 2) ? 0 : (v === 0) ? 1 : 2
                                }
                            }
                            onActivated: {
                                if (SettingsManager) {
                                    var cfgVal = (currentIndex === 0) ? 2 : (currentIndex === 1) ? 0 : 1
                                    SettingsManager.writeConfigInt("DarkMode", cfgVal)
                                    Config.StaticData.darkMode = cfgVal
                                    Config.Theme.darkMode = cfgVal
                                }
                            }
                        }
                    }

                    RowLayout {
                        id: language
                        Layout.fillWidth: true
                        Layout.fillHeight: false
                        Layout.topMargin: 4

                        Label {
                            Layout.preferredHeight: 24
                            Layout.fillHeight: false
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("Sprache:")
                            color: Config.StaticData.palette.secondary.col200
                            font.pointSize: 12
                        }

                        ComboBox {
                            id: languageSelector
                            model: Config.StaticData.languages
                            textRole: "langName"
                            Component.onCompleted: {
                                var currentCode = Config.Parameters.language
                                for (var i = 0; i < model.length; ++i) {
                                    if (model[i].code === currentCode) {
                                        currentIndex = i
                                        return
                                    }
                                }
                            }
                            onActivated: {
                                var code = model[currentIndex].code
                                Config.Parameters.language = code
                                LanguageManager.switchLanguage(code)
                            }
                        }
                    }

                    CheckBox {
                        objectName: "displayRightToolboxCheckbox"
                        text: qsTr("Rechte Toolbox anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowRightToolBox") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowRightToolBox", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "displayLeftToolboxCheckbox"
                        text: qsTr("Linke Toolbox anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowLeftToolBox") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowLeftToolBox", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "fadeOutLosingCardsAnimationCheckbox"
                        text: qsTr("Ausblend-Animation für Verliererkarten")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowFadeOutCardsAnimation") !== 0 : true
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowFadeOutCardsAnimation", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "animatedCardsCheckbox"
                        text: qsTr("Animierte Karten (Aufdeck-Animation)")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowFlipCardsAnimation") !== 0 : true
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowFlipCardsAnimation", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "reverseFKeysOrderCheckbox"
                        text: qsTr("Alternative F-Tasten-Belegung (F1-F4)")
                        checked: SettingsManager ? SettingsManager.readConfigInt("AlternateFKeysUserActionMode") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("AlternateFKeysUserActionMode", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "showBlindButtonsCheckbox"
                        text: qsTr("Symbole für Small Blind und Big Blind anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowBlindButtons") !== 0 : true
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowBlindButtons", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "showPotPercentButtonsCheckbox"
                        text: qsTr("Pot-Prozent-Schaltflächen anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowPotPercentButtons") !== 0 : true
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowPotPercentButtons", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "showHandChanceMonitorCheckbox"
                        text: qsTr("Kartenchancenmonitor anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowCardsChanceMonitor") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowCardsChanceMonitor", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "showOwnCardsOnMouseClickCheckbox"
                        text: qsTr("Anti-Peek: Eigene Karten erst bei Klick anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("AntiPeekMode") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("AntiPeekMode", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "disableSplashScreenOnStartupCheckbox"
                        text: qsTr("Startbildschirm beim Startvorgang deaktivieren")
                        checked: SettingsManager ? SettingsManager.disableSplashScreen : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.disableSplashScreen = checked }
                    }

                    CheckBox {
                        objectName: "doNotTranslatePokerTermsCheckbox"
                        Layout.fillWidth: true
                        text: qsTr("Internationale Pokerausdrücke (Check, Call, Raise) nicht übersetzen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("DontTranslateInternationalPokerStringsFromStyle", checked ? 1 : 0) }
                        contentItem: Text {
                            text: parent.text
                            wrapMode: Text.Wrap
                            leftPadding: parent.indicator.width + parent.spacing
                            verticalAlignment: Text.AlignVCenter
                            color: parent.palette.windowText
                        }
                    }

                    CheckBox {
                        visible: Config.Responsive.compact
                        height: visible ? implicitHeight : 0
                        Layout.fillWidth: true
                        text: qsTr("Tischzoom aktivieren (Wischen & Zoomen, nur Mobilmodus)")
                        checked: Config.Parameters.tableZoomEnabled
                        onCheckedChanged: Config.Parameters.tableZoomEnabled = checked
                        contentItem: Text {
                            text: parent.text
                            wrapMode: Text.Wrap
                            leftPadding: parent.indicator.width + parent.spacing
                            verticalAlignment: Text.AlignVCenter
                            color: parent.palette.windowText
                        }
                    }
                    } // ColumnLayout
                }

                ScrollView {
                    id: networkTab
                    clip: true
                    contentWidth: availableWidth
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: parent.width

                    CheckBox {
                        objectName: "showCountryFlagOnAvatarCheckbox"
                        text: qsTr("Landesflagge in der Ecke des Avatars anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowCountryFlagInAvatar") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowCountryFlagInAvatar", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "showNetworkStatusColorOnAvatarCheckbox"
                        text: qsTr("Netzwerkstatus-Farbe in der Ecke des Avatars anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowPingStateInAvatar") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowPingStateInAvatar", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "focusBetInputOnTurnCheckbox"
                        text: qsTr("Fokus ins Einsatz-Eingabefeld setzen, wenn Sie an der Reihe sind")
                        checked: SettingsManager ? SettingsManager.readConfigInt("EnableBetInputFocusSwitch") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("EnableBetInputFocusSwitch", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "preventAccidentalCallAfterBigRaiseCheckbox"
                        text: qsTr("Versehentliches Call nach einem großen Raise verhindern")
                        checked: SettingsManager ? SettingsManager.readConfigInt("AccidentallyCallBlocker") !== 0 : true
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("AccidentallyCallBlocker", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "doNotHideIgnoredPlayerAvatarsCheckbox"
                        text: qsTr("Avatare von ignorierten Spielern nicht verbergen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DontHideAvatarsOfIgnored") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("DontHideAvatarsOfIgnored", checked ? 1 : 0) }
                    }

                    CheckBox {
                        objectName: "showLobbyChatCheckbox"
                        text: qsTr("Lobby-Chat anzeigen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("ShowGameChatOnly") === 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("ShowGameChatOnly", checked ? 0 : 1) }
                    }

                    CheckBox {
                        objectName: "disableEmojiReactionsCheckbox"
                        text: qsTr("Emoji-Reaktionen deaktivieren")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DisableEmojiReactions") !== 0 : false
                        onCheckedChanged: { if (SettingsManager) SettingsManager.writeConfigInt("DisableEmojiReactions", checked ? 1 : 0) }
                    }
                    } // ColumnLayout
                }
            }
        }
    }
}
