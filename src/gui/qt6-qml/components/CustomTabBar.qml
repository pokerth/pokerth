import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config

TabBar {
    id: customTabBar

    property alias model: tabButtons.model
    // Tab height/font size configurable (default = previous look).
    property int tabHeight: 24
    property real tabFontPointSize: 10

    // ── Colour tokens (overridable) ─────────────────────────────────────────
    // Default = global palette (light/dark mode of the app). Bars that sit on a
    // foreign surface – such as the info panel at the table, which follows the
    // table theme – switch them to its colours; otherwise a light table theme
    // would put the light dark-mode text on a light background.
    property color colTextActive: Config.StaticData.palette.secondary.col100
    property color colTextIdle:   Config.StaticData.palette.secondary.col200
    property color colTabActive:  Config.StaticData.palette.secondary.col500
    property color colTabIdle:    Config.StaticData.palette.secondary.col600

    Layout.fillWidth: true
    padding: 0
    currentIndex: 0

    background: Rectangle {
        color: customTabBar.colTabIdle
    }

    Repeater {
        id: tabButtons

        TabButton {
            id: tabButton

            property bool isHovered: false

            width: tabButtons.model.length > 0 ? Math.floor(customTabBar.width / tabButtons.model.length) : implicitWidth
            // If the tab width is not enough for the text (narrow portrait
            // windows, long translations), the caption wraps onto a second
            // line and the bar grows with it – previously the text was
            // cut off instead ("Third party li…").
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
                color: customTabBar.currentIndex === index || tabButton.isHovered ? customTabBar.colTextActive : customTabBar.colTextIdle
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: customTabBar.currentIndex === index || tabButton.isHovered ? customTabBar.colTabActive : customTabBar.colTabIdle
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
