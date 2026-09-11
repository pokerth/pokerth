import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Floating side panel for the game history & chat: rounded sheet with a
// header (title + close) and a free content area below it.
//   – landscape/fullscreen: sidebar (~1/3 width) at one side.
//   – portrait: full overlay above the table.
// The content (ListView, ChatBox …) is passed as the default child and ends up
// below the header in the body layout.
Item {
    id: root

    property string title: ""
    // Show/hide the header (title + close + separator). The info panel needs no
    // heading – the tab bar is enough there, and it is closed via the toggle
    // button at the top. The chat still uses the header.
    property bool showHeader: true
    property int edge: Qt.LeftEdge          // Qt.LeftEdge | Qt.RightEdge
    property bool wide: false
    signal closeRequested()

    // Table theme colours (independent of the light/dark mode of the app).
    // The StyleProvider always delivers valid values; the fallback only covers the
    // case that the context property is not set (e.g. in a preview).
    readonly property color colBackground:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBackground : "#1d222b"
    readonly property color colBorder:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogBorder : "#576378"
    readonly property color colTextSecondary:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogTextSecondary : "#cdd3e0"
    // Title colour: the accent of the TABLE theme, not the app accent – on a
    // light panel the app gold would hardly be readable.
    readonly property color colAccent:
        (typeof StyleProvider !== "undefined" && StyleProvider) ? StyleProvider.chatLogAccent : "#E3C800"

    // Default-Inhalt landet unter Header + Trennlinie im Body-Layout.
    default property alias content: bodyLayout.data

    anchors.top: parent.top
    anchors.bottom: parent.bottom
    anchors.left:  edge === Qt.LeftEdge  ? parent.left  : undefined
    anchors.right: edge === Qt.RightEdge ? parent.right : undefined
    width: wide ? Math.max(parent.width / 3, 300) : parent.width

    // Assign the chrome (sheet background, click catcher, header layout)
    // EXPLICITLY as children – otherwise they would be redirected into the
    // content layout via the default property (content → bodyLayout.data). That
    // way only the content declared by the caller flows into bodyLayout.
    children: [
        // Floating sheet: indented, rounded, with elevation.
        Rectangle {
            id: panel
            anchors.fill: parent
            anchors.topMargin: 50   // Distance to the toggle icon above
            anchors.bottomMargin: 10
            anchors.leftMargin: root.wide ? 10 : 8
            anchors.rightMargin: root.wide ? 10 : 8
            radius: 16
            color: Config.Theme.withAlpha(root.colBackground, 0.95)
            border.color: root.colBorder
            border.width: 1

            layer.enabled: Config.Theme.effectsEnabled
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#000000"
                shadowOpacity: 0.55
                shadowBlur: 0.9
                shadowVerticalOffset: 3
                shadowHorizontalOffset: 0
            }
        },

        // Catch clicks inside the sheet (the table next to it stays usable)
        MouseArea { anchors.fill: panel },

        ColumnLayout {
            id: bodyLayout
            anchors.fill: panel
            anchors.margins: 12
            spacing: 8

            // Header: title + close
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                visible: root.showHeader
                AppText {
                    Layout.fillWidth: true
                    text: root.title
                    color: root.colAccent
                    font.pixelSize: 15
                    font.bold: true
                    font.letterSpacing: 0.4
                }
                Rectangle {
                    Layout.preferredWidth: 26
                    Layout.preferredHeight: 26
                    radius: 13
                    color: closeArea.containsMouse
                           ? Config.Theme.withAlpha(root.colBorder, 0.7)
                           : "transparent"
                    SvgIcon {
                        anchors.centerIn: parent
                        width: 14; height: 14
                        source: "../resources/close.svg"
                        layer.enabled: true
                        layer.effect: MultiEffect {
                            colorization: 1.0
                            colorizationColor: root.colTextSecondary
                        }
                    }
                    MouseArea {
                        id: closeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeRequested()
                    }
                }
            }

            // Trennlinie
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Config.Theme.withAlpha(root.colBorder, 0.5)
                visible: root.showHeader
            }
        }
    ]
}
