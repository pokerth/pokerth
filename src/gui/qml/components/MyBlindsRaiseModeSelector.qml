import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "../js/tools.js" as GlobalTools
import "styles"

Item {
    id: root
    anchors.fill: parent

    property string titleString: ""
    property bool alwaysDoubleBlinds: false
    property var manualBlindsList
    property ListModel internalManualBlindsList: ListModel {}
    property bool afterMBAlwaysDoubleBlinds: false
    property bool afterMBAlwaysRaiseAbout: false
    property bool afterMBStayAtLastBlind: false
    property string afterMBAlwaysRaiseValue: "0"
    property var parentListViewModel
    property var parentListViewRoot
    property int addBlindTextFieldMinValue: 5
    property int addBlindTextFieldMaxValue: 1000000

    property bool ready: false

    signal accepted

    function show(title, doubleBlinds, list, afterMBDouble, afterMBRaise, afterMBRaiseValue, afterMBStay, parentListModel, parentListRoot) {
        //initial the class values
        titleString = title
        alwaysDoubleBlinds = doubleBlinds;
        manualBlindsList = list;
        //to easily edit the array put it into an temporary listmodel
        internalManualBlindsList.clear();
        for (var i=0; i < list.length; i++) {
            internalManualBlindsList.append({"value": list[i]});
        }

        afterMBAlwaysDoubleBlinds = afterMBDouble;
        afterMBAlwaysRaiseAbout = afterMBRaise;
        afterMBAlwaysRaiseValue = afterMBRaiseValue;
        afterMBStayAtLastBlind = afterMBStay;
        parentListViewModel = parentListModel;
        parentListViewRoot = parentListRoot;

        //show the abstract selector with below defined specific content
        mySelector.show()

        //set the config options to radiobuttons and textfields
        if(alwaysDoubleBlinds) {
            radioBtnBlindsRaiseModeAlwaysDouble.checked = true;
            radioBtnBlindsRaiseModeManualBlindsList.checked = false;
        }
        else {
            radioBtnBlindsRaiseModeManualBlindsList.checked = true;
            radioBtnBlindsRaiseModeAlwaysDouble.checked = false;
        }

        //if we are in a createLocalGameView Dialog set min and max from corresponding values in the same list
        if(parentListViewRoot.myReadableId == "createLocalGameViewRoot") {
            //get current start blind as minimum and cash as maximum for the textfield
            var currentStartBlind = 0;
            var startCash = 0;
            for (var i=0; i < parentListViewModel.length; i++) {
                if(parentListViewModel[i].myId == "spinBox_FirstSmallBlind") {
                    currentStartBlind = parentListViewModel[i].myValue;
                }
                else if (parentListViewModel[i].myId == "spinBox_StartCash") {
                    startCash = parentListViewModel[i].myValue;

                }
            }
            addBlindTextFieldMinValue = currentStartBlind;
            addBlindTextFieldMaxValue = startCash;

            textFieldAddListBlind.text = addBlindTextFieldMinValue;
        }

        if(afterMBAlwaysDoubleBlinds) {
            radioBtnAfterMBAlwaysDouble.checked = true
            radioBtnAfterMBAlwaysRaiseAbout.checked = false
            radioBtnAfterMBKeepLastBlind.checked = false
        }
        else if (afterMBAlwaysRaiseAbout) {
            radioBtnAfterMBAlwaysDouble.checked = false
            radioBtnAfterMBAlwaysRaiseAbout.checked = true
            radioBtnAfterMBKeepLastBlind.checked = false
        }
        else {
            radioBtnAfterMBAlwaysDouble.checked = false
            radioBtnAfterMBAlwaysRaiseAbout.checked = false
            radioBtnAfterMBKeepLastBlind.checked = true
        }

        textFieldAfterMBAlwaysRaiseAboutValue.text = afterMBAlwaysRaiseValue
    }


    function selected(doubleBlinds, afterMBDouble, afterMBRaise, afterMBRaiseValue, afterMBStay) {
        alwaysDoubleBlinds = doubleBlinds;
        afterMBAlwaysDoubleBlinds = afterMBDouble;
        afterMBAlwaysRaiseAbout = afterMBRaise;
        afterMBAlwaysRaiseValue = afterMBRaiseValue;
        afterMBStayAtLastBlind = afterMBStay;

        mySelector.hide()
        root.accepted()
    }


    MyAbstractSelector {
        id: mySelector
        titleText: titleString
        button1Text: qsTr("CANCEL")
        onButton1Clicked: hide()
        button2Text: qsTr("OK")
        onButton2Clicked: {
            textFieldAfterMBAlwaysRaiseAboutValue.correctValue();
            //copy editable internal listmodel to returnable list
            manualBlindsList = new Array(); //clear at first
            for (var i=0; i < internalManualBlindsList.count; i++) {
                manualBlindsList.push(internalManualBlindsList.get(i).value.toString());
            }
            //return values
            root.selected(radioBtnBlindsRaiseModeAlwaysDouble.checked, radioBtnAfterMBAlwaysDouble.checked, radioBtnAfterMBAlwaysRaiseAbout.checked, textFieldAfterMBAlwaysRaiseAboutValue.text, radioBtnAfterMBKeepLastBlind.checked);
        }

        container: ScrollView {
            id: scrollView
            anchors.fill: parent
            ColumnLayout {
                width: parent.width
                spacing: AppStyle.columnLayoutSpacing
                ExclusiveGroup { id: blindsRaiseModeGroup }
                RadioButton {
                    id: radioBtnBlindsRaiseModeAlwaysDouble
                    exclusiveGroup: blindsRaiseModeGroup
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtnBlindsRaiseModeAlwaysDouble
                        labelString: qsTr("Always double blinds");
                    }
                }
                RadioButton {
                    id:  radioBtnBlindsRaiseModeManualBlindsList
                    exclusiveGroup: blindsRaiseModeGroup
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtnBlindsRaiseModeManualBlindsList
                        labelString: qsTr("Manual blinds list");
                    }
                }
                RowLayout {
                    id: listControlsRow
                    width: parent.width
                    spacing: AppStyle.rowLayoutSpacing
                    TextField {
                        id: textFieldAddListBlind
                        anchors.left: parent.left
                        //make space for the prefix
                        anchors.leftMargin: AppStyle.spinBoxSelectorTextFieldPrefixExtraLeftMargin
                        width: Math.round(listControlsRow.width * 0.5)
                        style: MyTextFieldStyle {
                            prefix: "$"
                            myTextField: textFieldAddListBlind
                        }
                        function correctValue() {
                            text = GlobalTools.correctTextFieldIntegerValue(root, text, addBlindTextFieldMinValue, addBlindTextFieldMaxValue);
                        }
                        onFocusChanged: { if(root.ready && !focus) correctValue(); }
                        onEditingFinished: { if(root.ready) correctValue(); }
                    }
                    Text {
                        id: btnAddNewListBlind
                        font.pixelSize: AppStyle.selectorBoxButtonFontSize
                        font.bold: true
                        width: Math.round(listControlsRow.width * 0.5)
                        text: qsTr("ADD BLIND")

                        MouseArea {
                            id: mouseAddNewListBlind
                            anchors.fill: parent
                            anchors.margins: AppStyle.selectorBoxButtonMouseAreaMargin
                            onClicked: {
                                textFieldAddListBlind.correctValue();
                                internalManualBlindsList.append({"value": textFieldAddListBlind.text.toString()})
                            }
                            Rectangle {
                                anchors.fill: parent
                                color: mouseAddNewListBlind.pressed ? "#55555555" : "#00ffffff"
                            }
                        }
                    }
                }
                Repeater {
                    id: manualBlindListRepeater
                    width: parent.width
                    model: internalManualBlindsList
                    delegate: RowLayout {
                        spacing: AppStyle.rowLayoutSpacing
                        Text {
                            id: blindInList
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize: AppStyle.radioButtonLabelFontSize
                            text: "$"+model.value
                        }
                        Text {
                            id: btnRemoveListBlind
                            anchors.verticalCenter: parent.verticalCenter
                            property int modelIndex: model.index
                            font.pixelSize: AppStyle.selectorBoxButtonFontSize
                            font.bold: true
                            width: Math.round(listControlsRow.width * 0.5)
                            text: qsTr("REMOVE")

                            MouseArea {
                                id: mouseRemoveListBlind
                                anchors.fill: parent
                                anchors.margins: AppStyle.selectorBoxButtonMouseAreaMargin
                                onClicked: {
                                    internalManualBlindsList.remove(btnRemoveListBlind.modelIndex)
                                }
                                Rectangle {
                                    anchors.fill: parent
                                    color: mouseRemoveListBlind.pressed ? "#55555555" : "#00ffffff"
                                }
                            }
                        }
                    }
                }

                Text {
                    font.pixelSize: AppStyle.selectorBoxValueFontSize
                    text: qsTr("Afterwards:")
                }

                ExclusiveGroup { id: afterMBModeGroup }
                RadioButton {
                    id: radioBtnAfterMBAlwaysDouble
                    exclusiveGroup: afterMBModeGroup
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtnAfterMBAlwaysDouble
                        labelString: qsTr("always double blinds");
                    }
                }
                RadioButton {
                    id: radioBtnAfterMBAlwaysRaiseAbout
                    exclusiveGroup: afterMBModeGroup
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtnAfterMBAlwaysRaiseAbout
                        labelString: qsTr("always raise about");
                    }
                }
                TextField {
                    id: textFieldAfterMBAlwaysRaiseAboutValue
                    //make space for the prefix
                    anchors.left: parent.left
                    anchors.leftMargin: AppStyle.spinBoxSelectorTextFieldPrefixExtraLeftMargin
                    width: radioBtnAfterMBAlwaysRaiseAbout.width
                    style: MyTextFieldStyle {
                        prefix: "$"
                        myTextField: textFieldAfterMBAlwaysRaiseAboutValue
                    }
                    function correctValue() {
                        text = GlobalTools.correctTextFieldIntegerValue(root, text, addBlindTextFieldMinValue, addBlindTextFieldMaxValue);
                    }
                    onFocusChanged: { if(root.ready && !focus) correctValue(); }
                    onEditingFinished: { if(root.ready) correctValue(); }
                }

                RadioButton {
                    id: radioBtnAfterMBKeepLastBlind
                    exclusiveGroup: afterMBModeGroup
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtnAfterMBKeepLastBlind
                        labelString: qsTr("keep last blind");
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        ready = true;
    }
}
