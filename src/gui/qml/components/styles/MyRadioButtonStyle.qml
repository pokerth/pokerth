import QtQuick 2.4
import QtQuick.Controls.Styles 1.2
import "../../js/colors.js" as GlobalColors
import "."

RadioButtonStyle {
    property var myRadioBtn
    property string labelString: ""
    indicator: Rectangle {
        implicitWidth: Math.round(appWindow.height*0.05)
        implicitHeight: Math.round(appWindow.height*0.05)
        radius: Math.round(selectionBox.height*0.03)
        border.color: myRadioBtn.checked ? GlobalColors.accentColor : "grey"
        border.width: Math.round(appWindow.height*0.005)
        Rectangle {
            anchors.fill: parent
            visible: control.checked
            color: myRadioBtn.checked ? GlobalColors.accentColor : "grey"
            radius: Math.round(appWindow.height*0.03)
            anchors.margins: Math.round(appWindow.height*0.01)
        }
    }
    label: Text {
        anchors.left: parent.left
        anchors.leftMargin: 10
        font.pixelSize: AppStyle.radioButtonLabelFontSize
        text: labelString

        MouseArea {
            anchors.fill: parent
            onClicked: myRadioBtn.clicked()
        }
    }
}
