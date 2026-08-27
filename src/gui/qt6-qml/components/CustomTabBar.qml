import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

TabBar {
    id: customTabBar

    property alias model: tabButtons.model
    // Höhe/Schriftgröße der Tabs konfigurierbar (Default = bisheriges Aussehen).
    property int tabHeight: 24
    property real tabFontPointSize: 10

    Layout.fillWidth: true
    padding: 0
    currentIndex: 0

    background: Rectangle {
        color: Config.StaticData.palette.secondary.col600
    }

    Repeater {
        id: tabButtons

        TabButton {
            id: tabButton

            property bool isHovered: false

            width: tabButtons.model.length > 0 ? Math.floor(customTabBar.width / tabButtons.model.length) : implicitWidth
            // Reicht die Tab-Breite für den Text nicht (schmale Portrait-
            // Fenster, lange Übersetzungen), bricht die Beschriftung auf eine
            // zweite Zeile um und die Leiste wächst mit – vorher wurde der
            // Text stattdessen abgeschnitten („Third party li…“).
            height: Math.max(customTabBar.tabHeight, tabLabel.implicitHeight)
            implicitHeight: height
            padding: 0
            contentItem: Text {
                id: tabLabel
                text: modelData
                font.pointSize: customTabBar.tabFontPointSize
                width: tabButton.width
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                color: customTabBar.currentIndex === index || tabButton.isHovered ? Config.StaticData.palette.secondary.col100 : Config.StaticData.palette.secondary.col200
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: customTabBar.currentIndex === index || tabButton.isHovered ? Config.StaticData.palette.secondary.col500 : Config.StaticData.palette.secondary.col600
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: customTabBar.currentIndex === index && tabButton.isHovered ? Qt.ArrowCursor : Qt.PointingHandCursor

                onClicked: {
                    customTabBar.currentIndex = index;
                }

                onEntered: {
                    tabButton.isHovered = true;
                }

                onExited: {
                    tabButton.isHovered = false;
                }
            }
        }
    }
}
