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

    // Is the action available for this row? An inactive icon is NOT hidden
    // (visible: false would have removed it from the row and made all following
    // icons move up), instead it keeps its place and is merely switched
    // invisible+dead. That way every icon sits at the same x position in all
    // rows. Whether the column exists at all is still decided by the caller
    // via visible.
    property bool active: true

    signal triggered()

    implicitWidth: visible ? iconSize + 6 : 0
    implicitHeight: iconSize + 6
    opacity: active ? 1.0 : 0.0

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
        enabled: root.active
        hoverEnabled: root.active
        cursorShape: Qt.PointingHandCursor
        onClicked: root.triggered()
    }

    ToolTip.text: root.tooltipText
    ToolTip.visible: ma.containsMouse && root.tooltipText !== ""
                     && Config.Parameters.showTooltips
    ToolTip.delay: 400
}
