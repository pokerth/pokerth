import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.VectorImage
import QtQuick.Effects

import "../config" as Config

Rectangle {
    id: toggle

    property bool isToggled: true
    property alias label: toggleLabel.text
    
    // Compatibility alias for standard CheckBox/Toggle API
    property alias checked: toggle.isToggled

    Layout.fillWidth: true
    Layout.fillHeight: false
    Layout.preferredHeight: 24
    Layout.topMargin: 8
    color: "transparent"

    RowLayout {
        spacing: 8
        VectorImage {
            id: toggleIcon
            source: toggle.isToggled ? "../resources/toggleRight.svg" : "../resources/toggleLeft.svg"
            Layout.preferredWidth: 32
            Layout.preferredHeight: 24
            visible: true

            MultiEffect {
                id: toggleIconCol
                source: toggleIcon
                anchors.fill: toggleIcon
                colorization: 1.0 // opacity equivalent
                colorizationColor: Config.StaticData.palette.secondary.col200
            }
        }

        Label {
            id: toggleLabel
            color: Config.StaticData.palette.secondary.col200
            text: qsTr("CheckBox LabelText")
            font.pointSize: 12
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: {
            toggle.isToggled = !toggle.isToggled;
        }
    }
}
