import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Switch {
    id: control

    // Rückwärtskompatibilität: isToggled und label
    property alias isToggled: control.checked
    property alias label: control.text

    Layout.fillWidth: true
    Layout.fillHeight: false
    Layout.topMargin: 4
}
