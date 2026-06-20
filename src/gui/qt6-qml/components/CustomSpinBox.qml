import QtQuick
import QtQuick.Controls
import QtQuick.Effects

import "../config" as Config

// Styled SpinBox: runde Ecken, Chart-Farben für +/−, Dark/Light-reaktiv
SpinBox {
    id: control

    leftPadding:  22
    rightPadding: 22
    topPadding:   0
    bottomPadding: 0

    // ─── Hintergrund ──────────────────────────────────────────────────────────
    background: Rectangle {
        implicitWidth:  100
        implicitHeight: 36
        radius: 8
        color:        Config.StaticData.palette.secondary.col600
        border.color: Config.StaticData.palette.secondary.col400
        border.width: 1

        Behavior on color        { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }

    // ─── Zahl-Anzeige ─────────────────────────────────────────────────────────
    contentItem: TextInput {
        z: 2
        text:  control.displayText
        font.family:    Config.StaticData.loadedFont.font.family
        font.pointSize: 11
        color:               Config.StaticData.palette.secondary.col100
        selectionColor:      Config.StaticData.chartColors[0]
        selectedTextColor:   Config.Theme.colorOnAccent
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment:   Qt.AlignVCenter
        readOnly:            !control.editable
        validator:           control.validator
        inputMethodHints:    Qt.ImhFormattedNumbersOnly
    }

    // ─── Minus-Knopf (links, rot/orange) ──────────────────────────────────────
    down.indicator: Rectangle {
        x: control.mirrored ? parent.width - width : 0
        implicitWidth:  24
        implicitHeight: 36
        topLeftRadius:    8
        bottomLeftRadius: 8
        topRightRadius:    0
        bottomRightRadius: 0

        color: control.down.pressed
            ? Config.StaticData.chartColors[5]
            : control.down.hovered
                ? Qt.rgba(Qt.color(Config.StaticData.chartColors[5]).r,
                          Qt.color(Config.StaticData.chartColors[5]).g,
                          Qt.color(Config.StaticData.chartColors[5]).b, 0.2)
                : "transparent"

        Behavior on color { ColorAnimation { duration: 120 } }

        SvgIcon {
            anchors.centerIn: parent
            width: 14
            height: 14
            source: "../resources/minus.svg"
            layer.enabled: true
            layer.effect: MultiEffect {
                colorization: 1.0
                colorizationColor: control.down.pressed
                    ? Config.Theme.colorOnAccent
                    : control.down.hovered
                        ? Config.StaticData.chartColor(5, true)
                        : Config.StaticData.palette.secondary.col300
            }
        }
    }

    // ─── Plus-Knopf (rechts, grün) ────────────────────────────────────────────
    up.indicator: Rectangle {
        x: control.mirrored ? 0 : parent.width - width
        implicitWidth:   24
        implicitHeight:  36
        topRightRadius:    8
        bottomRightRadius: 8
        topLeftRadius:    0
        bottomLeftRadius: 0

        color: control.up.pressed
            ? Config.StaticData.chartColors[0]
            : control.up.hovered
                ? Qt.rgba(Qt.color(Config.StaticData.chartColors[0]).r,
                          Qt.color(Config.StaticData.chartColors[0]).g,
                          Qt.color(Config.StaticData.chartColors[0]).b, 0.2)
                : "transparent"

        Behavior on color { ColorAnimation { duration: 120 } }

        SvgIcon {
            anchors.centerIn: parent
            width: 14
            height: 14
            source: "../resources/plus.svg"
            layer.enabled: true
            layer.effect: MultiEffect {
                colorization: 1.0
                colorizationColor: control.up.pressed
                    ? Config.Theme.colorOnAccent
                    : control.up.hovered
                        ? Config.StaticData.chartColor(0, true)
                        : Config.StaticData.palette.secondary.col300
            }
        }
    }
}
