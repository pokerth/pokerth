import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

Rectangle {
    id: logsSettings
    //Layout.preferredWidth: parent.width - 8
    //Layout.preferredHeight: parent.height - 8
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    // Notice after changing the log directory (only "OK").
    ConfirmPopup {
        id: logDirRestartHint
        showCancel: false
    }

    ColumnLayout {
        id: logsettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Log Messages"); topGap: 4 }

        ScrollView {
            id: logsScrollView
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
                GroupBox {
                    Layout.fillWidth: true
                    title: qsTr("Log Settings")

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        ConfigCheckBox {
                            id: logOnOff
                            text: qsTr("Enable Logging")
                            configKey: "LogOnOff"
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: qsTr("Log Directory:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: logOnOff.checked
                            }

                            TextField {
                                id: logDirectory
                                Layout.fillWidth: true
                                text: SettingsManager ? SettingsManager.readConfigString("LogDir") : ""
                                enabled: logOnOff.checked
                                readOnly: true
                            }

                            Button {
                                text: qsTr("Browse...")
                                enabled: logOnOff.checked
                                // On mobile devices the log directory lives in
                                // the app storage and cannot be chosen freely (the
                                // system dialog only returns content:// URIs there,
                                // which the engine cannot write to).
                                visible: !Config.Responsive.isMobile
                                onClicked: {
                                    if (!SettingsManager)
                                        return
                                    var dir = SettingsManager.pickDirectory(
                                        qsTr("Select Log Directory"),
                                        logDirectory.text)
                                    if (!dir || dir === logDirectory.text)
                                        return
                                    logDirectory.text = dir
                                    SettingsManager.writeConfigString("LogDir", dir)
                                    // The log is opened once at program start
                                    // (Log::init()), so the new directory only takes
                                    // effect after a restart.
                                    logDirRestartHint.openWith(
                                        qsTr("Log Directory"),
                                        qsTr("The log directory has been changed.\nPlease restart PokerTH so that the log files are stored there."),
                                        qsTr("OK"))
                                }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 8

                            Label {
                                text: qsTr("Log Retention (Days):")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: logOnOff.checked
                            }

                            CustomSpinBox {
                                id: logStoreDuration
                                from: 1
                                to: 365
                                value: SettingsManager ? SettingsManager.readConfigInt("LogStoreDuration") : 2
                                enabled: logOnOff.checked
                                onValueModified: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("LogStoreDuration", value)
                                }
                            }

                            Label {
                                text: qsTr("Log Interval:")
                                color: Config.StaticData.palette.secondary.col200
                                enabled: logOnOff.checked
                            }

                            ComboBox {
                                id: logInterval
                                Layout.fillWidth: true
                                // Index must match the engine's LogInterval values:
                                // 0 = flush after every action, 1 = after every hand,
                                // 2 = after every game (see src/engine/log.cpp).
                                model: [
                                    qsTr("Every Action"),
                                    qsTr("Every Hand"),
                                    qsTr("Every Game")
                                ]
                                currentIndex: SettingsManager ? SettingsManager.readConfigInt("LogInterval") : 1
                                enabled: logOnOff.checked
                                onActivated: {
                                    if (SettingsManager) SettingsManager.writeConfigInt("LogInterval", currentIndex)
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
