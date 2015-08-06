import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import "views/startview"

ApplicationWindow {
    id: appWindow
    title: qsTr("PokerTH - The open source poker app")
    width: 1024
    height: 600
    minimumWidth: 320
    minimumHeight: 240
    visible: true

    toolBar: Rectangle {
        id: toolbar
        opacity: 1
        Behavior on opacity { NumberAnimation{} }
        color: "#FF9800"
        width: appWindow.width
        height: appWindow.height*0.15
        Behavior on height { NumberAnimation{} }
        Rectangle { //Shadow
            y: parent.height
            width: parent.width
            height: parent.height*0.07
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0,0,0,0.5) }
                GradientStop { position: 1.0; color: Qt.rgba(0,0,0,0.0) }
            }
        }

        Rectangle {
            id: toolBarLeftButton
            anchors.verticalCenter: parent.verticalCenter
            antialiasing: true
            height: parent.height
            width: parent.height
            radius: 4
            color: leftMouse.pressed ? Qt.rgba(0,0,0,0.3) : "transparent"
            property string myIconName: ""
            property string myIconPath: ""
            function setIconPath(name) {
                switch(name) {
                    case "PokerTH": myIconPath = "images/pokerth-logo.svg"
                    break;
                    case "back": myIconPath = "images/arrow_back.svg"
                    break;
                    default: myIconPath = ""
                }
            }
            onMyIconNameChanged: setIconPath(myIconName)


            Image {
                sourceSize.height: parent.height*0.8
                fillMode: Image.PreserveAspectFit
                anchors.centerIn: parent
                source: parent.myIconPath
            }
            MouseArea {
                id: leftMouse
                anchors.fill: parent
                anchors.margins: -10
                onClicked: stackView.pop()
            }
        }

        Text {
            id: toolBarText
            font.pixelSize: Math.min(parent.height*0.5, parent.width*0.08)
            x: toolBarLeftButton.x + toolBarLeftButton.width*1.5
            anchors.verticalCenter: parent.verticalCenter
            color: "white"
            text: "PokerTH"

        }

        Rectangle {
            id: toolBarRightButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            antialiasing: true
            height: parent.height
            width: parent.height
            radius: 4
            color: rightMouse.pressed ? Qt.rgba(0,0,0,0.3) : "transparent"

            property string myIconName: ""
            property string myIconPath: ""
            function setIconPath(name) {
                switch(name) {
                    case "accept": myIconPath = "images/check.svg"
                    break;
                    case "more": myIconPath = "images/more_vert.svg"
                    break;
                    default: myIconPath = ""
                }
            }
            onMyIconNameChanged: setIconPath(myIconName)

            Image {
                sourceSize.height: parent.height*0.8
                fillMode: Image.PreserveAspectFit
                anchors.centerIn: parent
                source: parent.myIconPath
            }
            MouseArea {
                id: rightMouse
                anchors.fill: parent
                anchors.margins: -10
                onClicked: stackView.pop()
            }
        }
    }

    StackView {
        id: stackView
        anchors.fill: parent
        // Implements back key navigation
        focus: true
        Keys.onReleased: if (event.key === Qt.Key_Back && stackView.depth > 1) {
                             stackView.pop();
                             event.accepted = true;
                         }

        initialItem: StartView { }

        //        delegate: StackViewDelegate {
        //                function transitionFinished(properties)
        //                {
        //                    properties.exitItem.opacity = 1
        //                }

        //                pushTransition: StackViewTransition {
        //                    PropertyAnimation {
        //                        target: enterItem
        //                        property: "opacity"
        //                        from: 0
        //                        to: 1
        //                    }
        //                    PropertyAnimation {
        //                        target: exitItem
        //                        property: "opacity"
        //                        from: 1
        //                        to: 0
        //                    }
        //                }
        //            }
    }
}
