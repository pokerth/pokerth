import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Effects
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: preLoaderPage
    objectName: "preLoaderPage"
    visible: true
    width: mainWindow.width
    height: mainWindow.height
    color: "transparent"

    // Einstellung „Splash beim Start deaktivieren" (Config-Key
    // DisableSplashScreenOnStartup). Ist sie an, wird der PreLoader gar nicht erst
    // angezeigt, sondern sofort durch die StartPage ersetzt.
    readonly property bool splashDisabled:
        (typeof SettingsManager !== "undefined" && SettingsManager)
            ? SettingsManager.disableSplashScreen : false

    Component.onCompleted: {
        if (preLoaderPage.splashDisabled) {
            preLoaderProgressInfoTextTimer.running = false
            preLoaderDoneTimer.running = false
            // Stack erst nach Abschluss der Konstruktion umbauen (nicht synchron
            // im onCompleted des initialItem).
            Qt.callLater(function() {
                mainStackView.replaceCurrentItem(mainWindow.startPage)
            })
        }
    }

    Image {
        id: preLoaderBackground
        anchors.fill: parent
        source: "../resources/startWindowBackground.png"
        fillMode: Image.PreserveAspectCrop
        visible: false
    }

    MultiEffect {
        source: preLoaderBackground
        anchors.fill: preLoaderBackground
        blurEnabled: true
        blurMax: 64
        blur: 0.3
    }

    ColumnLayout {
        id: preLoaderContentLayout
        anchors.fill: parent
        Rectangle {
            id: progressBox
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: 270
            Layout.preferredHeight: 180
            color: Config.StaticData.palette.secondary.col700
            radius: 5
            opacity: 0.8

            ColumnLayout {
                id: preLoaderProgressRows
                anchors.fill: parent

                SvgIcon {
                    id: preLoaderProgressIconPokerTH
                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 96
                    Layout.topMargin: 10
                    source: "../resources/pokerth.svg"
                }

                ProgressBar {
                    id: preLoaderProgressBar
                    indeterminate: true
                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: parent.width / 6 * 4
                    Universal.accent: Config.StaticData.palette.secondary.col200
                }

                RowLayout {
                    id: preLoaderProgressInfo
                    Layout.alignment: Qt.AlignBottom
                    Layout.fillHeight: true
                    Layout.margins: 8
                    spacing: 8

                    SvgIcon {
                        id: preLoaderProgressInfoIconConsole
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        source: "../resources/terminal.svg"
                        //color: Config.StaticData.palette.secondary.col200
                    }

                    AppText {
                        id: preLoaderProgressInfoText
                        text: qsTr(Config.StaticData.progressMessages[Math.floor(Math.random() * Config.StaticData.progressMessages.length)])
                        color: Config.StaticData.palette.secondary.col200
                        font.pointSize: 12

                        Timer {
                            id: preLoaderProgressInfoTextTimer
                            interval: 1500
                            running: true
                            repeat: true
                            onTriggered: preLoaderProgressInfoText.text = qsTr(Config.StaticData.progressMessages[Math.floor(Math.random() * Config.StaticData.progressMessages.length)])
                        }

                        Timer {
                            id: preLoaderDoneTimer
                            interval: 5000 // @FIXME: increase in productive mode
                            running: true
                            repeat: false
                            onTriggered: {
                                preLoaderProgressInfoTextTimer.running = false;
                                mainStackView.replaceCurrentItem(mainWindow.startPage);
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            id: preLoaderFooter
            Layout.alignment: Qt.AlignBottom
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width
            Layout.leftMargin: parent.width / 2 - preLoaderFooterText.width / 2

            Item {
                Layout.preferredHeight: 16
                Layout.bottomMargin: 12

                AppText {
                    id: preLoaderFooterText
                    text: qsTr("PokerTH - v2.1.0preview")
                    color: Config.StaticData.palette.secondary.col200
                    font.pointSize: 12
                    style: Text.Outline
                    styleColor: Config.StaticData.palette.secondary.col600
                }

                MultiEffect {
                    source: preLoaderFooterText
                    anchors.fill: preLoaderFooterText
                    shadowEnabled: true
                    shadowColor: Config.StaticData.palette.secondary.col700
                    shadowHorizontalOffset: 2
                    shadowVerticalOffset: 2
                    shadowOpacity: 1
                    autoPaddingEnabled: true
                }
            }
        }
    }
}
