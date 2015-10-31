import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import "views/startview"
import "js/colors.js" as GlobalColors
import "components/styles"

ApplicationWindow {
    id: appWindow
    title: qsTr("PokerTH - The open source poker app")
    //send infos about app size to the style
    width: 1024
    height: 600
    minimumWidth: 320
    minimumHeight: 240
    visible: true

    onHeightChanged: { AppStyle.appHeight = height }
    onWidthChanged: { AppStyle.appWidth = width }

    Component.onCompleted: {
        // initial info about app size for the style
        AppStyle.appHeight = height
        AppStyle.appWidth = width
    }


    toolBar: Rectangle {
        id: toolbar
        opacity: 1
        Behavior on opacity { NumberAnimation{} }
        color: GlobalColors.accentColor
        width: appWindow.width
        height: appWindow.height*0.13
        Behavior on y { NumberAnimation{} }

        function show() {
            visible = true;
            y = 0;

        }

        function hide() {
            y = -height;
            visible = false;
        }

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
            color: toolBarLeftButtonMouse.pressed ? Qt.rgba(0,0,0,0.3) : "transparent"
            property string myIconName: ""
            property string myIconPath: ""
            signal clicked();

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
            onClicked: stackView.pop()

            Image {
                sourceSize.height: parent.height*0.8
                fillMode: Image.PreserveAspectFit
                anchors.centerIn: parent
                source: parent.myIconPath
            }
            MouseArea {
                id: toolBarLeftButtonMouse
                anchors.fill: parent
                anchors.margins: -10
                onClicked: parent.clicked()
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
            color: toolBarRightButtonMouse.pressed ? Qt.rgba(0,0,0,0.3) : "transparent"
            property string myIconName: ""
            property string myIconPath: ""
            signal clicked();

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
                id: toolBarRightButtonMouse
                anchors.fill: parent
                anchors.margins: -10
                onClicked: parent.clicked();
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
