import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

Item {
    id: root
    property int myListViewsHeight: 0
    property int myListViewsWidth: 0
    property var myListViewRoot
    property var myListViewModel

    property var componentMap: {
        "ComboBox": myComboBox,
        "SpinBox": mySpinBox,
        "BlindsRaiseInterval": myBlindsRaiseInterval
    }
    width: myListViewsWidth

    Loader {
        id: listViewFactoryLoader
        function setSourceComponent(comp) {
            sourceComponent = comp;
        }
    }

    // ComboBox
    Component {
        id: myComboBox
        Rectangle {
            MyComboBoxSelector {
                id: myComboBoxSelector
                parent: myListViewRoot // this needs to be parent to display the selector over the whole ListView
            }

            id: myComboBoxContent
            property int modelIndex: model.index
            color: mouse.pressed ? "#11000000" : "white"
            width: root.width
            height: Math.round(comboBoxTitle.contentHeight*2.1 + comboBoxValue.contentHeight*1.0)
            Text {
                id: comboBoxTitle
                anchors.left: parent.left
                anchors.margins: 30
                y: myValue != "" ? Math.round(parent.height*0.5 - contentHeight*0.9) : Math.round(parent.height*0.5 - contentHeight*0.5)
                color: "black"
                font.pixelSize: appWindow.listViewTitleFontSize
                text: myTitle
            }
            Text {
                id: comboBoxValue
                anchors.left: parent.left
                anchors.margins: 30
                y: Math.round(parent.height*0.5 + contentHeight*0.15)
                color: "grey"
                font.pixelSize: myValue != "" ? appWindow.listViewValueFontSize : 0
                //setup value from Index if necessary
                text: myValueIsIndex ? myValuesList.get(parseInt(myValue)).value : myValue
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - 30
                height: 1
                color: "lightgrey"
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    //call selector overlay and set inital values
                    myComboBoxSelector.show(myTitle, myValuesList, myValue, myValueIsIndex)
                }
                Connections {
                    target: myComboBoxSelector
                    onVisibleChanged: {
                        if(!myComboBoxSelector.visible) {
                            //call update the currentItem with selection from the comboBox selector
                            myListViewModel.setProperty(myComboBoxContent.modelIndex, "myValue", myComboBoxSelector.returnValue);
                        }
                    }
                }
            }

            Component.onCompleted: {
                //after building the delegate content set the final height to the root item
                root.height = Qt.binding(function() { return height })
            }
        }
    }

    // Spinbox
    Component {
        id: mySpinBox
        Rectangle {
            MySpinBoxSelector {
                id: mySpinBoxSelector
                parent: myListViewRoot // this needs to be parent to display the selector over the whole ListView
            }

            id: mySpinBoxContent
            property int modelIndex: model.index
            color: mouse.pressed ? "#11000000" : "white"
            width: root.width
            height: Math.round(spinBoxTitle.contentHeight*2.1 + spinBoxValue.contentHeight*1.0)
            Text {
                id: spinBoxTitle
                anchors.left: parent.left
                anchors.margins: 30
                y: myValue != "" ? Math.round(parent.height*0.5 - contentHeight*0.9) : Math.round(parent.height*0.5 - contentHeight*0.5)
                color: "black"
                font.pixelSize: appWindow.listViewTitleFontSize
                text: myTitle
            }
            Text {
                id: spinBoxValue
                anchors.left: parent.left
                anchors.margins: 30
                y: Math.round(parent.height*0.5 + contentHeight*0.15)
                color: "grey"
                font.pixelSize: myValue != "" ? appWindow.listViewValueFontSize : 0
                text: myPrefix+myValue
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - 30
                height: 1
                color: "lightgrey"
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    //call selector overlay and set inital values
                    mySpinBoxSelector.show(myTitle, myMinValue, myMaxValue, myValue, myPrefix)
                }
                Connections {
                    target: mySpinBoxSelector
                    onVisibleChanged: {
                        if(!mySpinBoxSelector.visible) {
                            //call update the currentItem with selection from the comboBox selector
                            myListViewModel.setProperty(mySpinBoxContent.modelIndex, "myValue", mySpinBoxSelector.returnValue);
                        }
                    }
                }
            }

            Component.onCompleted: {
                //after building the delegate content set the final height to the root item
                root.height = Qt.binding(function() { return height })
            }
        }
    }

    // BlindsRaiseInterval
    Component {
        id: myBlindsRaiseInterval
        Rectangle {
            MyBlindsRaiseIntervalSelector {
                id: myBlindsRaiseIntervalSelector
                parent: myListViewRoot // this needs to be parent to display the selector over the whole ListView
            }

            id: myBlindsRaiseIntervalContent
            property int modelIndex: model.index
            color: mouse.pressed ? "#11000000" : "white"
            width: root.width
            height: Math.round(blindsRaiseIntevalTitle.contentHeight*2.1 + blindsRaiseIntevalValue.contentHeight*1.0)
            Text {
                id: blindsRaiseIntevalTitle
                anchors.left: parent.left
                anchors.margins: 30
                y: myValue != "" ? Math.round(parent.height*0.5 - contentHeight*0.9) : Math.round(parent.height*0.5 - contentHeight*0.5)
                color: "black"
                font.pixelSize: appWindow.listViewTitleFontSize
                text: myTitle
            }
            Text {
                id: blindsRaiseIntevalValue
                anchors.left: parent.left
                anchors.margins: 30
                y: Math.round(parent.height*0.5 + contentHeight*0.15)
                color: "grey"
                font.pixelSize: myValue != "" ? appWindow.listViewValueFontSize : 0
                text: ""

                function buildTextString() {
                    if(myRaiseOnHandsType == "1") text = qsTr("Raise blinds every")+" "+myRaiseOnHandsInterval+" "+qsTr("hands");
                    else text = qsTr("Raise blinds every")+" "+myRaiseOnMinutesInterval+" "+qsTr("minutes");
                }

                Component.onCompleted: {
                    buildTextString();
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - 30
                height: 1
                color: "lightgrey"
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    //call selector overlay and set inital values
                    myBlindsRaiseIntervalSelector.show(myRaiseOnHandsType, myRaiseOnHandsInterval, myRaiseOnMinutesInterval)
                }
                Connections {
                    target: myBlindsRaiseIntervalSelector
                    onVisibleChanged: {
                        if(!myBlindsRaiseIntervalSelector.visible) {
                            //call update the currentItem with selection from the comboBox selector
                            myListViewModel.setProperty(myBlindsRaiseIntervalContent.modelIndex, "myRaiseOnHandsType", myBlindsRaiseIntervalSelector.raiseOnHandsType);
                            myListViewModel.setProperty(myBlindsRaiseIntervalContent.modelIndex, "myRaiseOnHandsInterval", myBlindsRaiseIntervalSelector.raiseOnHandsInterval);
                            myListViewModel.setProperty(myBlindsRaiseIntervalContent.modelIndex, "myRaiseOnMinutesInterval", myBlindsRaiseIntervalSelector.raiseOnMinutesInterval);
                            //here we also need to trigger the textStringBuild from the model again
                            blindsRaiseIntevalValue.buildTextString();
                        }
                    }
                }
            }

            Component.onCompleted: {
                //after building the delegate content set the final height to the root item
                root.height = Qt.binding(function() { return height })
            }
        }
    }


    Component.onCompleted: {
        listViewFactoryLoader.setSourceComponent(componentMap[myType])
    }
}

//        Column {
//            anchors.verticalCenter: parent.verticalCenter
//            anchors.left: parent.left
//            anchors.leftMargin: 30
//            Text {
//                id: comboBoxTitle
//                color: "#0F0F0F"
//                font.pixelSize: parent.height*0.30
//                renderType: Text.NativeRendering
//                text: title
//                anchors.left: parent.left
//            }
//            Text {
//                id: comboBoxValue
//                color: "#0F0F0F"
//                font.pixelSize: parent.height*0.30
//                renderType: Text.NativeRendering
//                text: value
//                anchors.left: parent.left
//            }
//        }
//        Rectangle { //seperator
//            anchors.horizontalCenter: parent.horizontalCenter
//            anchors.bottom: parent.bottom
//            width: parent.width - 30
//            height: 1
//            color: "lightgrey"
//        }
//        MouseArea {
//            id: mouse
//            anchors.fill: parent
//            onClicked: root.clicked()

//        }


//    Image {
//        anchors.right: parent.right
//        anchors.rightMargin: 20
//        anchors.verticalCenter: parent.verticalCenter
//        source: "../../images/navigation_next_item.png"
//    }

////                CheckBox {
////                    id: myCheckBox
////                    anchors.verticalCenter: parent.verticalCenter
////                    x: rect1.width-rect1.width*0.10
////                    style: CheckBoxStyle {
////                        indicator: Rectangle {
////                            color: "#FAFAFA"
////                            implicitWidth: rect1.width*0.04
////                            implicitHeight: rect1.width*0.04
////                            radius: 3
////                            border.color: control.activeFocus ? "darkblue" : "gray"
////                            border.width: 1
////                            Rectangle {
////                                visible: control.checked
////                                color: "#555"
////                                border.color: "#333"
////                                radius: 1
////                                anchors.margins: 4
////                                anchors.fill: parent
////                            }
////                        }
////                    }
////                }


