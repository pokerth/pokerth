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

    property ListModel selectionModel: ListModel {}
    property string titleString
    property string valueFromParent
    property string returnValue
    property bool valueIsIndex

    function show(title, list, vl, valueIndex) {
        titleString = title;
        valueFromParent = vl;
        returnValue = vl; // <-- initial the returnValue with the preselection to prevent a NULL return when pressing Cancel
        valueIsIndex = valueIndex;
        //fill the model
        selectionModel.clear();
        for (var i=0; i < list.count; i++) {
            selectionModel.append({"valueString": list.get(i).value});
        }
        visible = true;
    }

    function reject() {
        visible = false;
    }

    function selected(newString, index) {
        if(valueIsIndex) {
            returnValue = index.toString();
        }
        else {
            returnValue = newString
        }
        visible = false;
    }

    MouseArea {
        //set empty MouseArea to prevent the background to be clicked
        anchors.fill: parent
    }

    Rectangle {
        id: selectionBox
        visible: true
        color: "white"
        height: Math.round(parent.height*0.9)
        width: Math.round(parent.width*0.6)
        x: Math.round(parent.width*0.5 - width*0.5)
        y: Math.round(parent.height*0.5 - height*0.5)
        radius: Math.round(parent.height*0.01)

        ColumnLayout {
            id: content
            anchors.fill: parent
            spacing: Math.round(selectionBox.height*0.06)
            Text {
                id: titleText
                font.pixelSize: AppStyle.selectorTitleFontSize
                font.bold: true
                text: titleString
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: 30
                anchors.topMargin: 10
                Layout.preferredHeight: contentHeight
            }
            ScrollView {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.leftMargin: 50
                Layout.fillHeight: true
                ListView {
                    anchors.fill: parent
                    spacing: Math.round(appWindow.height*0.05)
                    model: selectionModel
                    delegate: RadioButton {
                        id: radioBtn
                        //check of value is index type and do the corresponding checked? test
                        checked: valueIsIndex ? (parseInt(valueFromParent) == index ? true : false) : (valueFromParent == valueString ? true : false)
                        onClicked: {
                            selector.selected(valueString, index)
                        }
                        style: MyRadioButtonStyle {
                            myRadioBtn: radioBtn
                            labelString: valueString
                        }
                    }
                }
            }
            Text {
                id: cancelButton
                font.pixelSize: AppStyle.selectorButtonFontSize
                font.bold: true
                text: qsTr("CANCEL")
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 30
                anchors.bottomMargin: 20
                Layout.preferredHeight: contentHeight

                MouseArea {
                    id: cancelMouse
                    anchors.fill: parent
                    onClicked: {
                        reject()
                    }
                }
            }
        }
    }
}

