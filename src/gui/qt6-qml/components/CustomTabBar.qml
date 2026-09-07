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

    // ── Farb-Tokens (überschreibbar) ────────────────────────────────────────
    // Default = globale Palette (Hell/Dunkel-Modus der App). Leisten, die auf
    // einer fremden Fläche liegen – etwa das Info-Panel am Tisch, das dem
    // Tisch-Theme folgt – setzen sie auf dessen Farben um; sonst stünde bei
    // einem hellen Tisch-Theme die helle Dunkelmodus-Schrift auf hellem Grund.
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
