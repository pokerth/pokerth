import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "../js/tools.js" as GlobalTools
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
    property bool ready: false

    function show(type, hands, minutes) {
        raiseOnHandsType = type;
        raiseOnHandsInterval = hands;
        raiseOnMinutesInterval = minutes;
        visible = true;

        if(raiseOnHandsType == "1") {
            textFieldHandsInterval.focus = true;
            radioBtnRaiseOnHands.checked = true;
        }
        else {
            textFieldMinutesInterval.focus = true;
            radioBtnRaiseOnMinutes.checked = true;
        }
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
                        exclusiveGroup: raiseTypeGroup
                        style: MyRadioButtonStyle {
                            myRadioBtn: radioBtnRaiseOnHands;
                            labelString: "Every"
                        }
                        onClicked: textFieldHandsInterval.focus = true;
                    }

                    TextField {
                        id: textFieldHandsInterval
                        width: appWindow.textFieldFontSize*2
                        validator: IntValidator{bottom: raiseOnHandsMinimum; top: raiseOnHandsMaximum;}
                        text: raiseOnHandsInterval
                        function correctValue() {
                            text = GlobalTools.correctTextFieldIntegerValue(selector, text, raiseOnHandsMinimum);
                        }
                        onFocusChanged: { if(selector.ready && !focus) correctValue(); }
                        onEditingFinished: { if(selector.ready) correctValue(); }

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
                        width: appWindow.textFieldFontSize*2
                        function correctValue() {
                            text = GlobalTools.correctTextFieldIntegerValue(selector, text, raiseOnMinutesMinimum);

                        }
                        onFocusChanged: { if(selector.ready && !focus) correctValue(); }
                        onEditingFinished: { if(selector.ready) correctValue(); }

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
    Component.onCompleted: ready = true;
}
