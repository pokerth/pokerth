import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

Button {
    property string page

    width: parent.width*0.50
    height: parent.height/7
    anchors.horizontalCenter: parent.horizontalCenter

    Component {
        id: myStyle
        ButtonStyle {
            panel: Item {
                implicitHeight: height
                implicitWidth: width
                BorderImage {
                    anchors.fill: parent
                    antialiasing: true
                    border.bottom: 8
                    border.top: 8
                    border.left: 8
                    border.right: 8
                    anchors.margins: control.pressed ? -4 : 0
                    source: control.pressed ? "../../images/button-pressed.png" : "../../images/button-default.png"
                    Text {
                        text: control.text
                        anchors.centerIn: parent
                        color: "white"
                        font.pixelSize: Math.min(parent.width*0.10, parent.height*0.40)
                        renderType: Text.NativeRendering
                    }
                }
            }
        }
    }

    style: myStyle
}

