import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Labelled filter selection field (label + ComboBox) of the community cup
// ranking pages (BBC/WEC). The models consistently use textRole "label" /
// valueRole "value". `activated(value)` fires with the selected value;
// `currentIndex` and `indexOfValue()` pass ComboBox functions through for restoring.
// In compact mode the field and the ComboBox stretch (otherwise a fixed comboWidth).
RowLayout {
    id: field
    property alias label: lbl.text
    property alias model: combo.model
    property alias comboEnabled: combo.enabled
    property real comboWidth: 160
    property bool compact: false
    property alias currentIndex: combo.currentIndex
    signal activated(var value)

    function indexOfValue(v) { return combo.indexOfValue(v) }

    Layout.fillWidth: compact
    spacing: 8

    AppLabel {
        id: lbl
        Layout.alignment: Qt.AlignVCenter
        color: Config.StaticData.palette.secondary.col200
        font.pixelSize: Config.Theme.fontSizeBody
    }
    ComboBox {
        id: combo
        Layout.fillWidth: field.compact
        Layout.preferredWidth: field.comboWidth
        textRole: "label"
        valueRole: "value"
        onActivated: field.activated(currentValue)
    }
}
