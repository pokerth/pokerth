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

        SettingsHeader { title: qsTr("Benutzeroberfläche") }

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

                    ConfigCheckBox {
                        objectName: "fadeOutLosingCardsAnimationCheckbox"
                        text: qsTr("Ausblend-Animation für Verliererkarten")
                        configKey: "ShowFadeOutCardsAnimation"
                    }

                    ConfigCheckBox {
                        objectName: "animatedCardsCheckbox"
                        text: qsTr("Animierte Karten (Aufdeck-Animation)")
                        configKey: "ShowFlipCardsAnimation"
                    }

                    CheckBox {
                        objectName: "reduceEffectsCheckbox"
                        text: qsTr("Grafikeffekte reduzieren (Schatten/Glow) – für schwache Systeme")
                        checked: SettingsManager ? SettingsManager.readConfigInt("QmlReduceEffects") !== 0 : false
                        onToggled: {
                            if (SettingsManager) SettingsManager.writeConfigInt("QmlReduceEffects", checked ? 1 : 0)
                            Config.Theme.effectsEnabled = !checked
                        }
                    }

                    ConfigCheckBox {
                        objectName: "reverseFKeysOrderCheckbox"
                        text: qsTr("Alternative F-Tasten-Belegung (F1-F4)")
                        configKey: "AlternateFKeysUserActionMode"
                        defaultChecked: false
                    }

                    ConfigCheckBox {
                        objectName: "showBlindButtonsCheckbox"
                        text: qsTr("Symbole für Small Blind und Big Blind anzeigen")
                        configKey: "ShowBlindButtons"
                    }

                    ConfigCheckBox {
                        objectName: "showPotPercentButtonsCheckbox"
                        text: qsTr("Pot-Prozent-Schaltflächen anzeigen")
                        configKey: "ShowPotPercentButtons"
                    }

                    ConfigCheckBox {
                        objectName: "showOwnCardsOnMouseClickCheckbox"
                        text: qsTr("Anti-Peek: Eigene Karten erst bei Klick anzeigen")
                        configKey: "AntiPeekMode"
                        defaultChecked: false
                    }

                    CheckBox {
                        objectName: "disableSplashScreenOnStartupCheckbox"
                        text: qsTr("Startbildschirm beim Startvorgang deaktivieren")
                        checked: SettingsManager ? SettingsManager.disableSplashScreen : false
                        onToggled: { if (SettingsManager) SettingsManager.disableSplashScreen = checked }
                    }

                    CheckBox {
                        objectName: "doNotTranslatePokerTermsCheckbox"
                        Layout.fillWidth: true
                        text: qsTr("Internationale Pokerausdrücke (Check, Call, Raise) nicht übersetzen")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
                        onToggled: { if (SettingsManager) SettingsManager.writeConfigInt("DontTranslateInternationalPokerStringsFromStyle", checked ? 1 : 0) }
                        contentItem: Text {
                            text: parent.text
                            wrapMode: Text.Wrap
                            leftPadding: parent.indicator.width + parent.spacing
                            verticalAlignment: Text.AlignVCenter
                            // font durchreichen: der Style-Default (Universal/CheckBox.qml)
                            // setzt font: control.font. Fehlt das, faellt dieser Text auf die
                            // Standard-Schriftgroesse zurueck (gemessen: 12 statt 13) und die
                            // umbrechenden Labels erscheinen groesser als alle anderen.
                            font: parent.font
                            // Universal.foreground statt palette.windowText: palette folgt
                            // der SYSTEM-Palette und nicht dem Universal.theme, das
                            // pokerth.qml aus dem DarkMode-Setting ableitet. Auf iOS/iPadOS
                            // ist windowText schwarz -> schwarze Schrift auf dunklem Grund.
                            // Universal.foreground ist exakt das, was der Style-Default der
                            // uebrigen CheckBoxen nutzt (Universal/CheckBox.qml), also sehen
                            // umbrechende und einzeilige Labels identisch aus.
                            color: parent.Universal.foreground
                        }
                    }

                    CheckBox {
                        objectName: "showTooltipsCheckbox"
                        visible: !Config.Responsive.isMobile
                        height: visible ? implicitHeight : 0
                        Layout.fillWidth: true
                        text: qsTr("Tooltips anzeigen")
                        checked: Config.Parameters.showTooltips
                        onCheckedChanged: Config.Parameters.showTooltips = checked
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
                            // font durchreichen: der Style-Default (Universal/CheckBox.qml)
                            // setzt font: control.font. Fehlt das, faellt dieser Text auf die
                            // Standard-Schriftgroesse zurueck (gemessen: 12 statt 13) und die
                            // umbrechenden Labels erscheinen groesser als alle anderen.
                            font: parent.font
                            // Universal.foreground statt palette.windowText: palette folgt
                            // der SYSTEM-Palette und nicht dem Universal.theme, das
                            // pokerth.qml aus dem DarkMode-Setting ableitet. Auf iOS/iPadOS
                            // ist windowText schwarz -> schwarze Schrift auf dunklem Grund.
                            // Universal.foreground ist exakt das, was der Style-Default der
                            // uebrigen CheckBoxen nutzt (Universal/CheckBox.qml), also sehen
                            // umbrechende und einzeilige Labels identisch aus.
                            color: parent.Universal.foreground
                        }
                    }

                    CheckBox {
                        objectName: "keepEmptySeatsCheckbox"
                        Layout.fillWidth: true
                        text: qsTr("Plätze verlassener Spieler am Tisch freihalten (verbleibende Spielerboxen bleiben an ihrem Platz)")
                        checked: Config.Parameters.keepEmptySeats
                        onCheckedChanged: Config.Parameters.keepEmptySeats = checked
                        contentItem: Text {
                            text: parent.text
                            wrapMode: Text.Wrap
                            leftPadding: parent.indicator.width + parent.spacing
                            verticalAlignment: Text.AlignVCenter
                            // font durchreichen: der Style-Default (Universal/CheckBox.qml)
                            // setzt font: control.font. Fehlt das, faellt dieser Text auf die
                            // Standard-Schriftgroesse zurueck (gemessen: 12 statt 13) und die
                            // umbrechenden Labels erscheinen groesser als alle anderen.
                            font: parent.font
                            // Universal.foreground statt palette.windowText: palette folgt
                            // der SYSTEM-Palette und nicht dem Universal.theme, das
                            // pokerth.qml aus dem DarkMode-Setting ableitet. Auf iOS/iPadOS
                            // ist windowText schwarz -> schwarze Schrift auf dunklem Grund.
                            // Universal.foreground ist exakt das, was der Style-Default der
                            // uebrigen CheckBoxen nutzt (Universal/CheckBox.qml), also sehen
                            // umbrechende und einzeilige Labels identisch aus.
                            color: parent.Universal.foreground
                        }
                    }

                    CheckBox {
                        objectName: "showCommunityContentCheckbox"
                        Layout.fillWidth: true
                        text: qsTr("Community-Inhalte anzeigen")
                        checked: Config.Parameters.showCommunityContent
                        onCheckedChanged: Config.Parameters.showCommunityContent = checked
                    }

                    // Vorausgewählte Quelle für Table Info und Player Stats.
                    RowLayout {
                        id: defaultCommunityRow
                        Layout.fillWidth: true
                        Layout.leftMargin: 24
                        visible: Config.Parameters.showCommunityContent
                        Layout.preferredHeight: visible ? implicitHeight : 0

                        Label {
                            Layout.preferredHeight: 24
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("Standard-Community:")
                            color: Config.StaticData.palette.secondary.col200
                            font.pointSize: 12
                        }

                        ComboBox {
                            id: defaultCommunitySelector
                            objectName: "defaultCommunitySelector"
                            model: Config.Community.entries
                            textRole: "label"
                            Component.onCompleted:
                                currentIndex = Math.max(0, indexForKey(Config.Parameters.defaultCommunity))
                            onActivated:
                                Config.Parameters.defaultCommunity = model[currentIndex].key

                            function indexForKey(key) {
                                for (var i = 0; i < model.length; ++i)
                                    if (model[i].key === key)
                                        return i
                                return -1
                            }
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

                    ConfigCheckBox {
                        objectName: "showNetworkStatusColorOnAvatarCheckbox"
                        text: qsTr("Netzwerkstatus-Farbe in der Ecke des Avatars anzeigen")
                        configKey: "ShowPingStateInAvatar"
                    }

                    ConfigCheckBox {
                        objectName: "focusBetInputOnTurnCheckbox"
                        text: qsTr("Fokus ins Einsatz-Eingabefeld setzen, wenn Sie an der Reihe sind")
                        configKey: "EnableBetInputFocusSwitch"
                        defaultChecked: false
                    }

                    ConfigCheckBox {
                        objectName: "preventAccidentalCallAfterBigRaiseCheckbox"
                        text: qsTr("Versehentliches Call nach einem großen Raise verhindern")
                        configKey: "AccidentallyCallBlocker"
                    }

                    ConfigCheckBox {
                        objectName: "disableEmojiReactionsCheckbox"
                        text: qsTr("Emoji-Reaktionen deaktivieren")
                        configKey: "DisableEmojiReactions"
                        defaultChecked: false
                    }
                    } // ColumnLayout
                }
            }
        }
    }
}
