import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config

// Beschriftetes Filter-Auswahlfeld (Label + ComboBox) der Community-Cup-Ranking-
// Seiten (BBC/WEC). Die Modelle nutzen durchgängig textRole "label" / valueRole
// "value". `activated(value)` feuert mit dem gewählten Wert; `currentIndex` und
// `indexOfValue()` reichen ComboBox-Funktionen für das Wiederherstellen durch.
// Im Compact-Modus dehnen sich Feld und ComboBox (sonst feste comboWidth).
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
