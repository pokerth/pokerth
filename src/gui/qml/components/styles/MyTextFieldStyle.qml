import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../../js/colors.js" as GlobalColors
import "."

TextFieldStyle {
    property string prefix: ""
    property int prefixIndent: prefix != "" ? AppStyle.textFieldFontSize*0.33 : 0
    property var myTextField

    textColor: "black"
    font.pixelSize: AppStyle.textFieldFontSize
    background: Item { //bottom line with colored focus indicator
        implicitHeight: AppStyle.textFieldFontSize + Math.round(AppStyle.textFieldFontSize*0.25)
        implicitWidth: myTextField.width
        Text { //prefix
            visible: prefix != "" ? true : false
            color: "black"
            font.pixelSize: AppStyle.textFieldFontSize
            text: prefix
            anchors.left: parent.left
            anchors.leftMargin: -prefixIndent
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Math.round(AppStyle.appHeight*0.008)
        }
        Rectangle {
            color: GlobalColors.accentColor
            opacity: myTextField.activeFocus ? 0.8 : 0
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: - prefixIndent
            width: myTextField.width + prefixIndent
            height: Math.round(AppStyle.appHeight*0.005)
        }
        Rectangle {
            color: "black"
            opacity: myTextField.activeFocus ? 0.6 : 0.4
            anchors.bottom: parent.bottom
            anchors.bottomMargin: Math.round(AppStyle.appHeight*0.005)
            anchors.left: parent.left
            anchors.leftMargin: - prefixIndent
            width: myTextField.width + prefixIndent
            height: Math.round(AppStyle.appHeight*0.005)
        }
    }

}
