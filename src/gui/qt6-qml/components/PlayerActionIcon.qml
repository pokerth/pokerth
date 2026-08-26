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

    // Ist die Aktion für diese Zeile verfügbar? Ein inaktives Icon wird NICHT
    // ausgeblendet (visible: false hätte es aus der Row entfernt und alle
    // folgenden Icons nachrücken lassen), sondern behält seinen Platz und wird
    // nur unsichtbar+tot geschaltet. So sitzt jedes Icon in allen Zeilen an
    // derselben x-Position. Ob die Spalte überhaupt existiert, entscheidet der
    // Aufrufer weiterhin über visible.
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
