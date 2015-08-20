import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "styles"

Rectangle {
    id: selector
    z:1000
    visible: false
    anchors.fill: parent
    color: "#88000000" //dark transparent background

    property alias container: ctr.children

    property string titleText: ""
    property string button1Text: ""
    property string button2Text: ""

    signal button1Clicked
    signal button2Clicked

    function show() {
        visible = true;
    }

    function hide() {
        visible = false;
    }

    MouseArea {
        //set empty MouseArea to prevent the background to be clicked
        anchors.fill: parent
    }

    Rectangle {
        id: selectorBox
        visible: true
        color: "white"
        height: AppStyle.selectorBoxHeight
        width: AppStyle.selectorBoxWidth
        anchors.centerIn: parent;
        radius: AppStyle.selectorBoxRadius

        ColumnLayout {
            id: content
            anchors.fill: parent
            anchors.margins: AppStyle.selectorBoxContentMargins
            spacing: AppStyle.columnLayoutSpacing
            Text {
                id: title
                font.pixelSize: AppStyle.selectorBoxTitleFontSize
                font.bold: AppStyle.selectorBoxTitleFontBold
                text: titleText
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.topMargin: AppStyle.selectorBoxTitleTopMargin
                Layout.preferredHeight: contentHeight
            }

            Rectangle {
                id: preCtr
                Layout.fillHeight: true
                Layout.fillWidth: true
                Rectangle {
                    anchors.fill: preCtr
                    anchors.leftMargin: AppStyle.selectorBoxRealContentLeftMargin
                    anchors.rightMargin: AppStyle.selectorBoxRealContentRightMargin
                    id: ctr
                    //The content goes here
                }
            }

            RowLayout {
                width: parent.width
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: AppStyle.rowLayoutSpacing
                Layout.preferredHeight: btn1.contentHeight

                Text {
                    id: btn1
                    font.pixelSize: AppStyle.selectorBoxButtonFontSize
                    font.bold: true
                    width: button1Text == "" ? 0 : contentWidth
                    text: button1Text

                    signal buttonClicked

                    MouseArea {
                        id: mouse1
                        anchors.fill: parent
                        onClicked: selector.button1Clicked()
                    }
                }
                Text {
                    id: btn2
                    font.pixelSize: AppStyle.selectorBoxButtonFontSize
                    font.bold: true
                    width: button2Text == "" ? 0 : contentWidth
                    text: button2Text

                    MouseArea {
                        id: mouse2
                        anchors.fill: parent
                        onClicked: selector.button2Clicked()
                    }
                }
            }
        }
    }
}

