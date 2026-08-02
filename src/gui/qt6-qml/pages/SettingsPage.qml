import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config
import "../components"

Rectangle {
    id: settingsPage
    objectName: "settingsPage"

    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    // Aktuelle Kategorie für den Compact-Strip
    property int currentCategoryIndex: 0

    // Compact: horizontale Icon-Tabs für Kategorien
    Rectangle {
        id: compactCategoryStrip
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: Config.Theme.touchTarget
        visible: Config.Responsive.compact
        color: Config.Theme.colorBox
        z: 1

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Repeater {
                model: settingsMenuListItems
                delegate: Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        anchors { fill: parent; margins: 3 }
                        radius: Config.Theme.radiusSmall
                        color: settingsPage.currentCategoryIndex === index
                               ? Config.StaticData.palette.secondary.col600
                               : "transparent"

                        Behavior on color { ColorAnimation { duration: 130 } }

                        // Gold-Akzent-Unterstrich beim aktiven Kategorie-Tab
                        Rectangle {
                            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                            anchors.margins: 4
                            height: 2
                            radius: 1
                            color: Config.Theme.colorAccent
                            opacity: settingsPage.currentCategoryIndex === index ? 0.85 : 0
                            Behavior on opacity { NumberAnimation { duration: 130 } }
                        }

                        SvgIcon {
                            anchors.centerIn: parent
                            width: Config.Theme.iconSize
                            height: Config.Theme.iconSize
                            source: "../resources/" + icon + ".svg"
                            layer.enabled: true
                            layer.effect: MultiEffect {
                                colorization: 1.0
                                colorizationColor: settingsPage.currentCategoryIndex === index
                                    ? Config.StaticData.palette.secondary.col100
                                    : Config.StaticData.palette.secondary.col200
                            }
                        }
                    }

                    TapHandler {
                        onTapped: {
                            settingsPage.currentCategoryIndex = index
                            settingsStackView.replaceCurrentItem(
                                "qrc:/components/" + source + "Settings.qml",
                                {}, StackView.Immediate)
                        }
                    }
                }
            }
        }

        Rectangle {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 1
            color: Config.StaticData.palette.secondary.col500
        }
    }

    RowLayout {
        anchors {
            top: Config.Responsive.compact ? compactCategoryStrip.bottom : parent.top
            left: parent.left; right: parent.right; bottom: parent.bottom
        }
        spacing: 0

        Rectangle {
            id: settingsNMenuBox
            visible: !Config.Responsive.compact
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: mainWindow.width / 6
            Layout.minimumWidth: 250
            Layout.preferredHeight: mainWindow.height
            Layout.topMargin: 4
            Layout.leftMargin: 16
            Layout.rightMargin: 8
            Layout.bottomMargin: 16
            border.width: 1
            border.color: Config.StaticData.palette.secondary.col500
            radius: 5
            color: "transparent"

            // Dezenter Schatten ragt nach außen; Innenfläche (1px innerhalb des
            // Rahmens) bleibt in Seitenfarbe, damit die Box-Optik unverändert wirkt.
            PanelShadow {
                anchors.fill: parent
                anchors.margins: 1
                radius: 4
                color: Config.Theme.colorBox
            }

            ListView {
                id: settingsMenuList
                model: settingsMenuListItems
                width: parent.width
                height: parent.height - 16
                anchors.centerIn: parent
                currentIndex: 0

                property int prevIndex: 0

                delegate: Rectangle {
                    id: settingsMenuListItem

                    readonly property bool isCurrent: ListView.isCurrentItem
                    readonly property bool isHighlighted: isCurrent || hoverArea.containsMouse

                    property alias labelText: label.text
                    property alias iconSource: iconImage.source
                    property alias iconWidth: iconImage.width
                    property alias iconHeight: iconImage.height
                    signal clicked

                    labelText: name
                    iconSource: "../resources/" + icon + ".svg"

                    color: isHighlighted ? Config.StaticData.palette.secondary.col600 : "transparent"
                    width: parent.width
                    height: 36

                    Behavior on color { ColorAnimation { duration: 130 } }

                    // Gold-Akzentbalken links beim aktiven Eintrag
                    Rectangle {
                        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                        width: 3
                        radius: width / 2
                        color: Config.Theme.colorAccent
                        opacity: settingsMenuListItem.isCurrent ? 0.85 : 0
                        Behavior on opacity { NumberAnimation { duration: 130 } }
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 6

                        SvgIcon {
                            id: iconImage
                            Layout.leftMargin: 16
                            Layout.topMargin: 4
                            Layout.bottomMargin: 4
                            Layout.alignment: Qt.AlignLeft
                            Layout.preferredHeight: 24
                            Layout.preferredWidth: 24
                            layer.enabled: true
                            layer.effect: MultiEffect {
                                colorization: 1.0
                                colorizationColor: settingsMenuListItem.isHighlighted
                                    ? Config.StaticData.palette.secondary.col100
                                    : Config.StaticData.palette.secondary.col200
                            }
                        }

                        AppText {
                            id: label
                            Layout.alignment: Qt.AlignLeft
                            Layout.fillWidth: true
                            Layout.topMargin: 4
                            Layout.bottomMargin: 4
                            font.pointSize: 12
                            color: settingsMenuListItem.isHighlighted
                                ? Config.StaticData.palette.secondary.col100
                                : Config.StaticData.palette.secondary.col200
                        }
                    }

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            settingsMenuList.currentIndex = index;
                            settingsStackView.replaceCurrentItem("qrc:/components/" + source + "Settings.qml", {}, StackView.Immediate);
                        }
                    }
                }
        }
        }

        Rectangle {
            id: settingsContentBox
            Layout.alignment: Qt.AlignLeft
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.horizontalStretchFactor: 2
            Layout.preferredHeight: mainWindow.height
            Layout.topMargin: 4
            Layout.leftMargin: Config.Responsive.compact ? 0 : 8
            Layout.rightMargin: Config.Responsive.compact ? 0 : 16
            Layout.bottomMargin: Config.Responsive.compact ? 0 : 16
            border.width: Config.Responsive.compact ? 0 : 1
            border.color: Config.StaticData.palette.secondary.col500
            radius: 5
            color: "transparent"

            // Dezenter Schatten – im Compact-Mode (randlos) deaktiviert.
            PanelShadow {
                anchors.fill: parent
                anchors.margins: 1
                radius: 4
                visible: !Config.Responsive.compact
                color: Config.Theme.colorBox
            }

            StackView {
                id: settingsStackView
                anchors.fill: parent
                clip: true
                initialItem: GuiSettings {}
            }
        }
    }

    ListModel {
        id: settingsMenuListItems
        ListElement {
            name: qsTr("Benutzeroberfläche")
            icon: "monitor"
            source: "Gui"
        }
        ListElement {
            name: qsTr("Stil")
            icon: "palette"
            source: "Style"
        }
        ListElement {
            name: qsTr("Sound")
            icon: "speaker"
            source: "Sound"
        }
        ListElement {
            name: qsTr("Lokales Spiel")
            icon: "spade"
            source: "LocalGame"
        }
        ListElement {
            name: qsTr("Netzwerkspiel")
            icon: "network"
            source: "NetworkGame"
        }
        ListElement {
            name: qsTr("Internetspiel")
            icon: "globe"
            source: "InternetGame"
        }
        ListElement {
            name: qsTr("Nicknamen/Avatare")
            icon: "userSquare"
            source: "NicknameAvatar"
        }
        ListElement {
            name: qsTr("Log-Nachrichten")
            icon: "terminalWindow"
            source: "Logs"
        }
        ListElement {
            name: qsTr("Standardeinstellung")
            icon: "arrowUpLeft"
            source: "Reset"
        }
    }
}
