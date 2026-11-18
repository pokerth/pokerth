import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: resetSettings
    Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
    color: "transparent"

    property bool resetDone: false

    ColumnLayout {
        id: resetSettingsContent
        anchors.fill: parent

        SettingsHeader { title: qsTr("Default Settings") }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: 16

                Label {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    text: qsTr("All settings will be reset to their default values. This action cannot be undone.")
                    wrapMode: Text.Wrap
                    color: Config.StaticData.palette.secondary.col200
                    font.pointSize: 11
                }

                CustomButton {
                    id: resetButton
                    text: qsTr("Reset to Factory Defaults")
                    enabled: !resetSettings.resetDone
                    Layout.topMargin: 4
                    implicitWidth: contentItem.implicitWidth + 24

                    background: Rectangle {
                        radius: Config.Theme.radiusSmall
                        color: resetButton.enabled
                               ? (resetButton.pressed
                                  ? Config.Theme.colorButtonDangerPress
                                  : resetButton.hovered
                                    ? Config.Theme.colorButtonDangerHover
                                    : Config.Theme.colorButtonDangerNormal)
                               : Config.StaticData.palette.secondary.col600
                        border.color: resetButton.enabled
                                      ? (resetButton.hovered || resetButton.pressed ? Config.Theme.colorButtonDangerBorderHover : Config.Theme.colorButtonDangerBorder)
                                      : Config.StaticData.palette.secondary.col500
                        border.width: 1
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }

                    onClicked: {
                        if (SettingsManager) {
                            SettingsManager.resetToDefaults()
                            resetSettings.resetDone = true
                        }
                    }
                }

                Label {
                    visible: resetSettings.resetDone
                    Layout.fillWidth: true
                    text: qsTr("Settings have been reset. Please restart PokerTH for all changes to take effect.")
                    wrapMode: Text.Wrap
                    color: Config.Theme.colorSuccessMessage
                    font.pointSize: 11
                }
            }
        }
    }
}
