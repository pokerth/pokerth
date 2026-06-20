import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import "../config" as Config

Item {
    id: root

    property string source: ""
    property color baseColor: "white"
    property string tooltipText: ""
    property int iconSize: 18

    signal triggered()

    implicitWidth: visible ? iconSize + 6 : 0
    implicitHeight: iconSize + 6

    SvgIcon {
        id: img
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        source: root.source
        smooth: true

        layer.enabled: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: ma.containsMouse
                ? root.baseColor
                : Qt.darker(root.baseColor, 1.5)
            brightness: ma.pressed ? -0.15 : 0.0
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.triggered()
    }

    ToolTip.text: root.tooltipText
    ToolTip.visible: ma.containsMouse && root.tooltipText !== ""
    ToolTip.delay: 400
}
