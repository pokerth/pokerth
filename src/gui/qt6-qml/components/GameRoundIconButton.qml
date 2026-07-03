import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import "../config" as Config

// Rundes Umschalt-Icon am Tischrand (Spielverlauf, Chat …): aktiver Zustand
// hebt sich farblich ab; optional zeigt ein Badge oben rechts die Anzahl
// ungelesener Elemente. Position/z setzt der Aufrufer über anchors.
Rectangle {
    id: root

    property url iconSource: ""
    property bool active: false
    property int unread: 0           // 0 = kein Badge
    property string tooltipText: ""  // leer = kein Tooltip
    signal clicked()

    width: 34; height: 34; radius: 17
    color: active ? Config.Theme.colorAccent : Qt.rgba(0, 0, 0, 0.45)

    ToolTip.visible: mouseArea.containsMouse && root.tooltipText !== ""
                     && !Config.Responsive.isMobile && Config.Parameters.showTooltips
    ToolTip.delay: 600
    ToolTip.text: root.tooltipText

    SvgIcon {
        anchors.centerIn: parent
        width: 20
        height: 20
        source: root.iconSource
        layer.enabled: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: root.active ? "#101010" : "#FFFFFF"
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: root.clicked()
    }

    // Badge mit Anzahl ungelesener Einträge.
    Rectangle {
        visible: root.unread > 0
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: -3
        anchors.rightMargin: -3
        width: Math.max(17, unreadLabel.implicitWidth + 8)
        height: 17
        radius: 8.5
        color: Config.Theme.colorDanger
        border.color: "#1d222b"
        border.width: 1.5

        AppText {
            id: unreadLabel
            anchors.centerIn: parent
            text: root.unread > 99 ? "99+" : root.unread
            color: "#FFFFFF"
            font.pixelSize: 10
            font.bold: true
        }
    }
}
