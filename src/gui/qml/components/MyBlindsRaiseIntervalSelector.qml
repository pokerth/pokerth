import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "../js/tools.js" as GlobalTools
import "styles/"

Item {
    id: root
    anchors.fill: parent

    property string titleString: ""
    property string raiseOnHandsType: ""
    property string raiseOnHandsInterval: ""
    property string raiseOnMinutesInterval: ""
    property int raiseOnHandsMinimum: 1
    property int raiseOnHandsMaximum: 99
    property int raiseOnMinutesMinimum: 1
    property int raiseOnMinutesMaximum: 99
    property bool ready: false

    signal accepted

    function show(title, type, hands, minutes) {
        titleString = title
        raiseOnHandsType = type;
        raiseOnHandsInterval = hands;
        raiseOnMinutesInterval = minutes;

        mySelector.show()

        if(raiseOnHandsType == "1") {
//            textFieldHandsInterval.focus = true;
            radioBtnRaiseOnHands.checked = true;
        }
        else {
//            textFieldMinutesInterval.focus = true;
            radioBtnRaiseOnMinutes.checked = true;
        }
    }

    function selected(handsType, handsInterval, minutesInterval) {
        raiseOnHandsType = handsType;
        raiseOnHandsInterval = handsInterval;
        raiseOnMinutesInterval = minutesInterval;

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
            textFieldHandsInterval.correctValue();
            textFieldMinutesInterval.correctValue();
            //return values
            root.selected(radioBtnRaiseOnHands.checked ? "1": "0", textFieldHandsInterval.text, textFieldMinutesInterval.text);
        }

        container: ColumnLayout {
            anchors.fill: parent
            RowLayout { //Hands interval section
                spacing: AppStyle.rowLayoutSpacing
                ExclusiveGroup { id: raiseTypeGroup }
                RadioButton {
                    id: radioBtnRaiseOnHands
                    exclusiveGroup: raiseTypeGroup
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtnRaiseOnHands;
                        labelString: "Every"
                    }
                    onClicked: textFieldHandsInterval.focus = true;
                }

                TextField {
                    id: textFieldHandsInterval
                    width: AppStyle.textFieldFontSize*2
                    validator: IntValidator{bottom: raiseOnHandsMinimum; top: raiseOnHandsMaximum;}
                    text: raiseOnHandsInterval
                    function correctValue() {
                        text = GlobalTools.correctTextFieldIntegerValue(root, text, raiseOnHandsMinimum);
                    }
                    onFocusChanged: { if(root.ready && !focus) correctValue(); }
                    onEditingFinished: { if(root.ready) correctValue(); }

                    style: MyTextFieldStyle {
                        myTextField: textFieldHandsInterval
                    }
                }
                Text {
                    id: onHandsString
                    font.pixelSize: AppStyle.selectorBoxValueFontSize
                    text: qsTr("hands")
                }
            }
            RowLayout { //Minutes interval section
                spacing: AppStyle.rowLayoutSpacing
                RadioButton {
                    id: radioBtnRaiseOnMinutes
                    exclusiveGroup: raiseTypeGroup
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtnRaiseOnMinutes;
                        labelString: "Every"
                    }
                    onClicked: textFieldMinutesInterval.focus = true;
                }
                TextField {
                    id: textFieldMinutesInterval
                    validator: IntValidator{bottom: raiseOnMinutesMinimum; top: raiseOnMinutesMaximum;}
                    text: raiseOnMinutesInterval
                    width: AppStyle.textFieldFontSize*2
                    function correctValue() {
                        text = GlobalTools.correctTextFieldIntegerValue(root, text, raiseOnMinutesMinimum);

                    }
                    onFocusChanged: { if(root.ready && !focus) correctValue(); }
                    onEditingFinished: { if(root.ready) correctValue(); }

                    style: MyTextFieldStyle {
                        myTextField: textFieldMinutesInterval
                    }
                }
                Text {
                    id: onMinutesString
                    font.pixelSize: AppStyle.selectorBoxValueFontSize
                    text: qsTr("minutes")
                }
            }
        }
    }
    Component.onCompleted: ready = true;
}
