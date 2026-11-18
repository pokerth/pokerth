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

        SettingsHeader { title: qsTr("User Interface") }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12

            CustomTabBar {
                id: guiSettingsTabBar
                model: [qsTr("General"), qsTr("Network")]
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
                            model: [qsTr("Automatic"), qsTr("Light"), qsTr("Dark")]
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
                                    // The lobby chat history carries its colours as
                                    // placeholders; without this signal it would stay in
                                    // the colours of the old mode (light text on a light
                                    // background).
                                    if (typeof Lobby !== "undefined" && Lobby)
                                        Lobby.refreshChatColors()
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
                            text: qsTr("Language:")
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
                                LanguageManager.switchLanguage(code)   // user interface right away
                                // Write the authoritative value into the ConfigFile (short
                                // code, so that the widgets client finds its .qm).
                                // The chat translation reads the key live -> takes effect immediately.
                                if (typeof SettingsManager !== "undefined" && SettingsManager)
                                    SettingsManager.language =
                                        Config.StaticData.localeToConfigLanguage(code)
                            }
                        }
                    }

                    AppCheckBox {
                        objectName: "allowChatTranslationCheckbox"
                        text: qsTr("Offer translation (globe symbol in chat and on forum posts)")
                        checked: (typeof SettingsManager !== "undefined" && SettingsManager)
                                 ? SettingsManager.readConfigInt("AllowChatTranslation") !== 0 : true
                        onToggled: {
                            if (typeof SettingsManager !== "undefined" && SettingsManager)
                                SettingsManager.writeConfigInt("AllowChatTranslation", checked ? 1 : 0)
                            // Real time: adjust the visible history of both chats
                            // immediately (show/hide globe/translations).
                            if (typeof Lobby !== "undefined" && Lobby && Lobby.chatTranslator)
                                Lobby.chatTranslator.refreshEnabled()
                            if (typeof GameTable !== "undefined" && GameTable && GameTable.chatTranslator)
                                GameTable.chatTranslator.refreshEnabled()
                            // ... and the globe of the forum post page.
                            if (typeof Translator !== "undefined" && Translator)
                                Translator.refreshEnabled()
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: 28
                        Layout.bottomMargin: 4
                        wrapMode: Text.Wrap
                        text: qsTr("Translation uses an external service (Google Translate, or MyMemory as a fallback). Nothing is sent until you tap the symbol of a message or a forum post; the target language is the one selected above.")
                        color: Config.StaticData.palette.secondary.col400
                        font.pointSize: 10
                    }

                    ConfigCheckBox {
                        objectName: "showChatTimestampCheckbox"
                        text: qsTr("Show timestamp in chat")
                        configKey: "ShowChatTimestamp"
                    }

                    ConfigCheckBox {
                        objectName: "fadeOutLosingCardsAnimationCheckbox"
                        text: qsTr("Fade-out animation for losing cards")
                        configKey: "ShowFadeOutCardsAnimation"
                    }

                    ConfigCheckBox {
                        objectName: "animatedCardsCheckbox"
                        text: qsTr("Animated Cards (Flip Animation)")
                        configKey: "ShowFlipCardsAnimation"
                    }

                    AppCheckBox {
                        objectName: "reduceEffectsCheckbox"
                        text: qsTr("Reduce graphics effects (shadows/glow) – for low-end systems")
                        checked: SettingsManager ? SettingsManager.readConfigInt("QmlReduceEffects") !== 0 : false
                        onToggled: {
                            if (SettingsManager) SettingsManager.writeConfigInt("QmlReduceEffects", checked ? 1 : 0)
                            Config.Theme.effectsEnabled = !checked
                        }
                    }

                    ConfigCheckBox {
                        objectName: "reverseFKeysOrderCheckbox"
                        text: qsTr("Alternative F-Key Assignment (F1-F4)")
                        configKey: "AlternateFKeysUserActionMode"
                        defaultChecked: false
                    }

                    ConfigCheckBox {
                        objectName: "showBlindButtonsCheckbox"
                        text: qsTr("Show Small Blind and Big Blind symbols")
                        configKey: "ShowBlindButtons"
                    }

                    ConfigCheckBox {
                        objectName: "showPotPercentButtonsCheckbox"
                        text: qsTr("Show pot percent buttons")
                        configKey: "ShowPotPercentButtons"
                    }

                    ConfigCheckBox {
                        objectName: "showOwnCardsOnMouseClickCheckbox"
                        text: qsTr("Anti-Peek: Show own cards only on click")
                        configKey: "AntiPeekMode"
                        defaultChecked: false
                    }

                    AppCheckBox {
                        objectName: "disableSplashScreenOnStartupCheckbox"
                        text: qsTr("Disable splash screen on startup")
                        checked: SettingsManager ? SettingsManager.disableSplashScreen : false
                        onToggled: { if (SettingsManager) SettingsManager.disableSplashScreen = checked }
                    }

                    AppCheckBox {
                        objectName: "doNotTranslatePokerTermsCheckbox"
                        text: qsTr("Do not translate international poker terms (Check, Call, Raise)")
                        checked: SettingsManager ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
                        onToggled: { if (SettingsManager) SettingsManager.writeConfigInt("DontTranslateInternationalPokerStringsFromStyle", checked ? 1 : 0) }
                    }

                    AppCheckBox {
                        objectName: "showTooltipsCheckbox"
                        visible: !Config.Responsive.isMobile
                        height: visible ? implicitHeight : 0
                        text: qsTr("Show tooltips")
                        checked: Config.Parameters.showTooltips
                        onCheckedChanged: Config.Parameters.showTooltips = checked
                    }

                    AppCheckBox {
                        objectName: "guideOnCheckbox"
                        text: qsTr("Ace’s Help: explain each screen the first time")
                        checked: Config.Parameters.guideOn
                        // Through the overlay: it greets on, and says how to come back off.
                        onToggled: {
                            if (typeof guideOverlay === "undefined" || !guideOverlay)
                                Config.Parameters.guideOn = checked
                            else if (checked)
                                guideOverlay.turnOn()
                            else
                                guideOverlay.turnOff()
                        }
                    }

                    AppCheckBox {
                        visible: Config.Responsive.compact
                        height: visible ? implicitHeight : 0
                        text: qsTr("Enable table zoom (swipe & zoom, mobile mode only)")
                        checked: Config.Parameters.tableZoomEnabled
                        onCheckedChanged: Config.Parameters.tableZoomEnabled = checked
                    }

                    AppCheckBox {
                        objectName: "keepEmptySeatsCheckbox"
                        text: qsTr("Keep seats of players who left the table (remaining player boxes stay in place)")
                        checked: Config.Parameters.keepEmptySeats
                        onCheckedChanged: Config.Parameters.keepEmptySeats = checked
                    }

                    // Forum news: newspaper icon in the top bar with a counter of
                    // unread posts. Independent of the community content
                    // (official PokerTH forum, not BBC/WEC).
                    AppCheckBox {
                        objectName: "showForumNewsCheckbox"
                        text: qsTr("Show forum news")
                        checked: Config.Parameters.showForumNews
                        onCheckedChanged: Config.Parameters.showForumNews = checked
                    }

                    AppCheckBox {
                        objectName: "showCommunityContentCheckbox"
                        text: qsTr("Show community content")
                        checked: Config.Parameters.showCommunityContent
                        onCheckedChanged: Config.Parameters.showCommunityContent = checked
                    }

                    // Preselected source for table info and player stats.
                    RowLayout {
                        id: defaultCommunityRow
                        Layout.fillWidth: true
                        Layout.leftMargin: 24
                        visible: Config.Parameters.showCommunityContent
                        Layout.preferredHeight: visible ? implicitHeight : 0

                        Label {
                            Layout.preferredHeight: 24
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("Default community:")
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

                    // Optional admin feature (player suggestion in the waiting room
                    // of your own BBC/WEC invite game) – like the default
                    // community only visible with community content enabled.
                    AppCheckBox {
                        objectName: "showCommunitySuggestCheckbox"
                        Layout.leftMargin: 24
                        visible: Config.Parameters.showCommunityContent
                        Layout.preferredHeight: visible ? implicitHeight : 0
                        text: qsTr("Suggest players in your own community games")
                        checked: Config.Parameters.showCommunitySuggest
                        onCheckedChanged: Config.Parameters.showCommunitySuggest = checked
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
                        text: qsTr("Show network status color in avatar corner")
                        configKey: "ShowPingStateInAvatar"
                    }

                    ConfigCheckBox {
                        objectName: "focusBetInputOnTurnCheckbox"
                        text: qsTr("Set focus to bet input when it's your turn")
                        configKey: "EnableBetInputFocusSwitch"
                        defaultChecked: false
                    }

                    ConfigCheckBox {
                        objectName: "preventAccidentalCallAfterBigRaiseCheckbox"
                        text: qsTr("Prevent accidental call after a big raise")
                        configKey: "AccidentallyCallBlocker"
                    }

                    ConfigCheckBox {
                        objectName: "disableEmojiReactionsCheckbox"
                        text: qsTr("Disable emoji reactions")
                        configKey: "DisableEmojiReactions"
                        defaultChecked: false
                    }
                    } // ColumnLayout
                }
            }
        }
    }
}
