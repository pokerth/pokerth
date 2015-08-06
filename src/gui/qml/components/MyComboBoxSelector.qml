import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2

Rectangle {
    id: selector
    z:1000
    visible: false
    anchors.fill: parent
    color: "#88000000" //Background

    property ListModel mySelectionModel: ListModel {}
    property string myTitleString
    property var returnSelection

    function show(title, map) {
        //at first fill the model
        myTitleString = title
//        for (var i=0; i < map.count; i++) {
//            mySelectionModel.append({"value": map.get(i).value});
//        }
        visible = true;
    }

    function reject() {
        visible = false;
    }

    function selected() {
        visible = false;
    }

    MouseArea {
        anchors.fill: parent
        onClicked: console.log("1001")
    }

    Rectangle {
        id: selectionBox
        visible: true
        color: "white"
        height: Math.round(parent.height*0.9)
        width: Math.round(parent.width*0.6)
        x: Math.round(parent.width*0.5 - width*0.5)
        y: Math.round(parent.height*0.5 - height*0.5)
        ColumnLayout {
            anchors.fill: parent
            Text {
                id: titleText
                font.pixelSize: Math.round(selectionBox.height*0.10)
                font.bold: true
                text: myTitleString
                anchors.left: parent.left
                anchors.margins: 30
                Layout.preferredHeight: titleText.contentHeight
            }
            ScrollView {
                ListView {
                    anchors.fill: parent
                    model: mySelectionModel
                    delegate: Text {
                        text: modelData
                    }
                }
            }
            Text {
                id: cancelButton
                font.pixelSize: Math.round(selectionBox.height*0.07)
                font.bold: true
                text: qsTr("Cancel")
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 50
                Layout.preferredHeight: cancelButton.contentHeight

                MouseArea {
                    id: cancelMouse
                    anchors.fill: parent
//                    preventStealing: true
                    onClicked: {
                        console.log("1002")
                        reject()
                    }
                }
            }
        }
    }
}

