import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "styles/"

Rectangle {
    id: selector
    z:1000
    visible: false
    anchors.fill: parent
    color: "#88000000" //dark transparent background

    property string titleString: ""
    property string raiseOnHandsType: ""
    property string raiseOnHandsInterval: ""
    property string raiseOnMinutesInterval: ""
    property int raiseOnHandsMinimum: 1
    property int raiseOnHandsMaximum: 99
    property int raiseOnMinutesMinimum: 1
    property int raiseOnMinutesMaximum: 99

    function show(type, hands, minutes) {
        raiseOnHandsType = type;
        raiseOnHandsInterval = hands;
        raiseOnMinutesInterval = minutes;
        visible = true;
    }

    function reject() {
        visible = false;
    }

    function selected(handsType, handsInterval, minutesInterval) {
        raiseOnHandsType = handsType;
        raiseOnHandsInterval = handsInterval;
        raiseOnMinutesInterval = minutesInterval;
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

        Rectangle {
            id: selectionBoxcontent
            anchors.fill: parent
            anchors.margins: Math.round(appWindow.height*0.05)

            ColumnLayout {
                id: content
                anchors.fill: parent
                spacing: appWindow.columnLayoutSpacing
                Text {
                    id: titleText
                    font.pixelSize: appWindow.selectorTitleFontSize
                    font.bold: true
                    text: qsTr("Raise blinds")
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.topMargin: -Math.round(appWindow.height*0.035)
                    Layout.preferredHeight: contentHeight
                }
                ExclusiveGroup { id: raiseTypeGroup }

                RowLayout { //Hands interval section
                    spacing: appWindow.rowLayoutSpacing

                    RadioButton {
                        id: radioBtnRaiseOnHands
                        checked: raiseOnHandsType ? true : false
                        exclusiveGroup: raiseTypeGroup
                        style: MyRadioButtonStyle {
                            myRadioBtn: radioBtnRaiseOnHands;
                            labelString: "Every"
                        }
                        onCheckedChanged: checked ? textFieldHandsInterval.focus = true : textFieldHandsInterval.focus = false
                    }

                    TextField {
                        id: textFieldHandsInterval
                        width: appWindow.textFieldFontSize*2
                        //                validator: IntValidator{bottom: minValue; top: maxValue;} TODO: test validator in later QtVersions > 5.5.0
                        text: raiseOnHandsInterval
                        focus: raiseOnHandsType ? true : false
                        function correctValue() {
                            if(text != "") {
                                if(parseInt(text) > raiseOnHandsMaximum) text = raiseOnHandsMaximum
                                else if(parseInt(text) < raiseOnHandsMinimum) text = raiseOnHandsMinimum
                                else {
                                    //remove leading "0000"
                                    var temp = parseInt(text)
                                    text = temp
                                }
                            }
                        }
                        onFocusChanged: {
                            correctValue()
                            focus ? radioBtnRaiseOnHands.checked = true : radioBtnRaiseOnHands.checked = false
                        }
                        onEditingFinished: correctValue()

                        style: MyTextFieldStyle {
                            myTextField: textFieldHandsInterval
                        }
                    }
                    Text {
                        id: onHandsString
                        font.pixelSize: appWindow.selectorValueFontSize
                        text: qsTr("hands")
                    }
                }
                RowLayout { //Minutes interval section
                    spacing: appWindow.rowLayoutSpacing
                    RadioButton {
                        id: radioBtnRaiseOnMinutes
                        checked: raiseOnHandsType ? false : true
                        exclusiveGroup: raiseTypeGroup
                        style: MyRadioButtonStyle {
                            myRadioBtn: radioBtnRaiseOnMinutes;
                            labelString: "Every"
                        }
                        onCheckedChanged: checked ? textFieldMinutesInterval.focus = true : textFieldMinutesInterval.focus = false
                    }

                    TextField {
                        id: textFieldMinutesInterval
      //                validator: IntValidator{bottom: minValue; top: maxValue;} TODO: test validator in later QtVersions > 5.5.0
                        text: raiseOnMinutesInterval
                        width: appWindow.textFieldFontSize*2
                        focus: raiseOnHandsType ? false : true
                        function correctValue() {
                            if(text != "") {
                                if(parseInt(text) > raiseOnMinutesMaximum) text = raiseOnMinutesMaximum
                                else if(parseInt(text) < raiseOnMinutesMinimum) text = raiseOnMinutesMinimum
                                else {
                                    //remove leading "0000"
                                    var temp = parseInt(text)
                                    text = temp
                                }
                            }
                        }
                        onFocusChanged: {
                            correctValue();
                            focus ? radioBtnRaiseOnMinutes.checked = true : radioBtnRaiseOnMinutes.checked = false
                        }
                        onEditingFinished: correctValue()

                        style: MyTextFieldStyle {
                            myTextField: textFieldMinutesInterval
                        }
                    }
                    Text {
                        id: onMinutesString
                        font.pixelSize: appWindow.selectorValueFontSize
                        text: qsTr("minutes")
                    }
                }

                RowLayout { //bottom button line
                    width: parent.width
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: -Math.round(appWindow.height*0.017)
                    spacing: okButton.contentWidth
                    Layout.preferredHeight: cancelButton.contentHeight

                    Text {
                        id: cancelButton
                        font.pixelSize: appWindow.selectorButtonFontSize
                        font.bold: true
                        text: qsTr("CANCEL")
                        Layout.preferredHeight: contentHeight
                        MouseArea {
                            id: cancelMouse
                            anchors.fill: parent
                            onClicked: {
                                reject()
                            }
                        }
                    }
                    Text {
                        id: okButton
                        font.pixelSize: appWindow.selectorButtonFontSize
                        font.bold: true
                        color: GlobalColors.accentColor
                        text: qsTr("OK")
                        Layout.preferredHeight: contentHeight
                        MouseArea {
                            id: okMouse
                            anchors.fill: parent
                            onClicked: {
                                textFieldHandsInterval.correctValue();
                                textFieldMinutesInterval.correctValue();
                                //return values
                                selected(radioBtnRaiseOnHands.checked ? "1": "0", textFieldHandsInterval.text, textFieldMinutesInterval.text);
                            }
                        }
                    }
                }
            }
        }
    }
}
