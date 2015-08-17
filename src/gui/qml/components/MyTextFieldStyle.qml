import QtQuick 2.4
import QtQuick.Controls.Styles 1.2
Component {
    id: myTextFieldStyle
    property string prefix: ""
    property int prefixIndent: Math.round(appWindow.height*0.03)

    TextFieldStyle {
        textColor: "black"
        font.pixelSize: Math.round(appWindow.height*0.10)
        background: Item { //bottom line with colored focus indicator
            implicitHeight: Math.round(appWindow.height*0.12)
            implicitWidth: parent.width
            Text { //prefix
                visible: prefix != "" ? true : false
                color: "black"
                font.pixelSize: Math.round(appWindow.height*0.10)
                text: prefix
                anchors.left: parent.left
                anchors.leftMargin: -prefixIndent
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Math.round(appWindow.height*0.008)

            }
            Rectangle {
                color: GlobalColors.accentColor
                opacity: parent.activeFocus ? 0.8 : 0
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: prefixIndent
                width: parent.width + prefixIndent
                height: Math.round(appWindow.height*0.005)
            }
            Rectangle {
                color: "black"
                opacity: parent.activeFocus ? 0.6 : 0.4
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Math.round(appWindow.height*0.005)
                anchors.left: parent.left
                anchors.leftMargin: - prefixIndent
                width: parent.width + prefixIndent
                height: Math.round(appWindow.height*0.005)
            }
        }
    }
}
