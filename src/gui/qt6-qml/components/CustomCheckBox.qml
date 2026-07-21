import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

Rectangle {
    id: checkBox

    property bool defaultValue: true
    property bool isChecked: Config.Parameters[checkBox.objectName] ?? defaultValue
    property alias label: checkBoxLabel.text
    
    // Compatibility alias for standard CheckBox API
    property alias checked: checkBox.isChecked

    Layout.fillWidth: true
    Layout.fillHeight: false
    Layout.preferredHeight: checkBoxLayout.implicitHeight + 8
    Layout.topMargin: 4
    color: "transparent"

    RowLayout {
        id: checkBoxLayout
        spacing: 8
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter

        SvgIcon {
            id: customCheck
            source: checkBox.isChecked ? "../resources/checkSquare.svg" : "../resources/square.svg"
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
            Layout.alignment: Qt.AlignVCenter
            // Einfärbung per layer.effect statt MultiEffect-Kind: VectorImage
            // (Qt >= 6.8) ist kein Texture-Provider und darf nicht per source
            // referenziert werden (sonst schwarz).
            layer.enabled: true
            layer.effect: MultiEffect {
                colorization: 1.0 // opacity equivalent
                colorizationColor: Config.StaticData.palette.secondary.col200
            }
        }

        Label {
            id: checkBoxLabel
            color: Config.StaticData.palette.secondary.col200
            text: qsTr("CheckBox LabelText")
            font.pointSize: 12
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: {
            checkBox.isChecked = !checkBox.isChecked;
            Config.Parameters[checkBox.objectName] = checkBox.isChecked;
        }
    }
}
