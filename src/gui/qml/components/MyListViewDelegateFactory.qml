import QtQuick 2.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "styles"

Item {
    id: root
    property int myListViewsHeight: 0
    property int myListViewsWidth: 0
    property var myListViewRoot
    property var myListViewModel

    property var componentMap: {
        "ComboBox": myComboBox,
                "SpinBox": mySpinBox,
                "BlindsRaiseInterval": myBlindsRaiseInterval,
                "BlindsRaiseMode": myBlindsRaiseMode
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
                onAccepted: myListViewModel.setProperty(myComboBoxContent.modelIndex, "myValue", myComboBoxSelector.returnValue);
            }

            id: myComboBoxContent
            property int modelIndex: model.index
            color: mouse.pressed ? "#11000000" : "white"
            width: root.width
            height: Math.round(textLayout.height*AppStyle.listViewDelegateHeightBasedOnContentFactor)
            ColumnLayout {
                id: textLayout
                spacing: AppStyle.listViewDelegateTitleAndValueSpacing
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: comboBoxTitle
                    anchors.left: parent.left
                    anchors.margins: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "black"
                    font.pixelSize: AppStyle.listViewDelegateTitleFontSize
                    text: myTitle
                    Layout.preferredHeight: contentHeight
                }
                Text {
                    id: comboBoxValue
                    anchors.left: parent.left
                    anchors.margins: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "grey"
                    font.pixelSize: myValue != "" ? AppStyle.listViewDelegateValueFontSize : 0
                    //setup value from Index if necessary
                    text: myValueIsIndex ? myValuesList.get(parseInt(myValue)).value : myValue
                    Layout.preferredHeight: contentHeight
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - AppStyle.listViewDelegateSeperatorLineIndent
                height: AppStyle.listViewDelegateSeperatorLineHeight
                color: "lightgrey"
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    //call selector overlay and set inital values
                    myComboBoxSelector.show(myTitle, myValuesList, myValue, myValueIsIndex)
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
                onAccepted: myListViewModel.setProperty(mySpinBoxContent.modelIndex, "myValue", mySpinBoxSelector.returnValue);
            }

            id: mySpinBoxContent
            property int modelIndex: model.index
            color: mouse.pressed ? "#11000000" : "white"
            width: root.width
            height: Math.round(textLayout.height*AppStyle.listViewDelegateHeightBasedOnContentFactor)
            ColumnLayout {
                id: textLayout
                spacing: AppStyle.listViewDelegateTitleAndValueSpacing
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: spinBoxTitle
                    anchors.left: parent.left
                    anchors.margins: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "black"
                    font.pixelSize: AppStyle.listViewDelegateTitleFontSize
                    text: myTitle
                    Layout.preferredHeight: contentHeight
                }
                Text {
                    id: spinBoxValue
                    anchors.left: parent.left
                    anchors.margins: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "grey"
                    font.pixelSize: myValue != "" ? AppStyle.listViewDelegateValueFontSize : 0
                    text: myPrefix+myValue
                    Layout.preferredHeight: contentHeight
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - AppStyle.listViewDelegateSeperatorLineIndent
                height: AppStyle.listViewDelegateSeperatorLineHeight
                color: "lightgrey"
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    //call selector overlay and set inital value
                    mySpinBoxSelector.show(myId, myTitle, myMinValue, myMaxValue, myValue, myPrefix)
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
            id: myBlindsRaiseIntervalContent
            MyBlindsRaiseIntervalSelector {
                id: myBlindsRaiseIntervalSelector
                parent: myListViewRoot // this needs to be parent to display the selector over the whole ListView
                onAccepted: {
                    //call update the currentItem with selection from the comboBox selector
                    myListViewModel.setProperty(myBlindsRaiseIntervalContent.modelIndex, "myRaiseOnHandsType", myBlindsRaiseIntervalSelector.raiseOnHandsType);
                    myListViewModel.setProperty(myBlindsRaiseIntervalContent.modelIndex, "myRaiseOnHandsInterval", myBlindsRaiseIntervalSelector.raiseOnHandsInterval);
                    myListViewModel.setProperty(myBlindsRaiseIntervalContent.modelIndex, "myRaiseOnMinutesInterval", myBlindsRaiseIntervalSelector.raiseOnMinutesInterval);
                    //here we also need to trigger the textStringBuild from the model again
                    blindsRaiseIntevalValue.buildTextString();
                }
            }

            property int modelIndex: model.index
            color: mouse.pressed ? "#11000000" : "white"
            width: root.width
            height: Math.round(textLayout.height*AppStyle.listViewDelegateHeightBasedOnContentFactor)
            ColumnLayout {
                id: textLayout
                spacing: AppStyle.listViewDelegateTitleAndValueSpacing
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: blindsRaiseIntevalTitle
                    anchors.left: parent.left
                    anchors.margins: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "black"
                    font.pixelSize: AppStyle.listViewDelegateTitleFontSize
                    text: myTitle
                    Layout.preferredHeight: blindsRaiseIntevalTitle.contentHeight
                }
                Text {
                    id: blindsRaiseIntevalValue
                    anchors.left: parent.left
                    anchors.margins: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "grey"
                    font.pixelSize: AppStyle.listViewDelegateValueFontSize
                    text: ""
                    Layout.preferredHeight: blindsRaiseIntevalValue.contentHeight

                    function buildTextString() {
                        if(myRaiseOnHandsType == "1") text = qsTr("Raise blinds every")+" "+myRaiseOnHandsInterval+" "+qsTr("hands");
                        else text = qsTr("Raise blinds every")+" "+myRaiseOnMinutesInterval+" "+qsTr("minutes");
                    }

                    Component.onCompleted: {
                        buildTextString();
                    }
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - AppStyle.listViewDelegateSeperatorLineIndent
                height: AppStyle.listViewDelegateSeperatorLineHeight
                color: "lightgrey"
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    //call selector overlay and set inital values
                    myBlindsRaiseIntervalSelector.show(myTitle, myRaiseOnHandsType, myRaiseOnHandsInterval, myRaiseOnMinutesInterval)
                }
            }

            Component.onCompleted: {
                //after building the delegate content set the final height to the root item
                root.height = Qt.binding(function() { return height })
            }
        }
    }

    Component {
        id: myBlindsRaiseMode // Always double vs. Manual blinds List
        Rectangle {
            id: myBlindsRaiseModeContent
            MyBlindsRaiseModeSelector {
                id: myBlindsRaiseModeSelector
                parent: myListViewRoot // this needs to be parent to display the selector over the whole ListView
                onAccepted: {
                    //TODO update model
                }
            }

            property int modelIndex: model.index
            color: mouse.pressed ? "#11000000" : "white"
            width: root.width
            height: Math.round(textLayout.height*AppStyle.listViewDelegateHeightBasedOnContentFactor)
            ColumnLayout {
                id: textLayout
                spacing: AppStyle.listViewDelegateTitleAndValueSpacing
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    id: blindsRaiseModeTitle
                    anchors.left: parent.left
                    anchors.leftMargin: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "black"
                    font.pixelSize: AppStyle.listViewDelegateTitleFontSize
                    text: myTitle
                    Layout.preferredHeight: contentHeight
                }
                Text {
                    id: blindsRaiseModeValue
                    anchors.left: parent.left
                    anchors.leftMargin: AppStyle.listViewDelegateTitleAndValueMargins
                    color: "grey"
                    font.pixelSize: AppStyle.listViewDelegateValueFontSize
                    text: ""
                    wrapMode: Text.WordWrap //maybe blinds list are too long
                    Layout.preferredHeight: contentHeight

                    function buildTextString() {
                        if(myAlwaysDoubleBlinds == "1") {
                            text = qsTr("Always double blinds");
                        }
                        else {
                            //build manual blindsList
                            var tempString = ""
                            for (var i=0; i < myManualBlindsList.count; i++) {
                                tempString = tempString + "$" + myManualBlindsList.get(i).blindValue.toString() + ", ";
                            }
                            text = tempString.substring(0, tempString.length - 2);
                            if(myAfterMBAlwaysDoubleBlinds == "1") {
                                text = text + "\n" + qsTr("afterwards always double blinds");
                            }
                            else if (myAfterMBAlwaysRaiseAbout == "1") {
                                text = text + "\n" + qsTr("afterwards always raise about $") + myAfterMBAlwaysRaiseValue;
                            }
                            else {
                                text = text + "\n" + qsTr("afterwards keep last blind")
                            }
                        }
                    }

                    Component.onCompleted: {
                        buildTextString();
                    }
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - AppStyle.listViewDelegateSeperatorLineIndent
                height: AppStyle.listViewDelegateSeperatorLineHeight
                color: "lightgrey"
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: {
                    //                    call selector overlay and set inital values
                    myBlindsRaiseModeSelector.show(myTitle, myAlwaysDoubleBlinds, myManualBlindsList, myAfterMBAlwaysDoubleBlinds, myAfterMBAlwaysRaiseAbout, myAfterMBAlwaysRaiseValue, myAfterMBStayAtLastBlind);
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


