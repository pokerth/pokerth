import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors

Rectangle {
    id: selector
    z:1000
    visible: false
    anchors.fill: parent
    color: "#88000000" //dark transparent background

    property string titleString
    property string prefix
    property int minValue
    property int maxValue
    property string returnValue
    property int initialTextFieldValue
    property int initialSliderValue

    function show(title, min, max, value, pref) {
        titleString = title;
        initialTextFieldValue = parseInt(value);
        initialSliderValue = parseInt(value)
        minValue = min;
        maxValue = max;
        prefix = pref;
        returnValue = value; // <-- initial the returnValue with the preselection to prevent a NULL return when pressing Cancel
        visible = true;
    }

    function reject() {
        visible = false;
    }

    function selected(newSelection) {
        returnValue = newSelection;
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
                font.pixelSize: Math.round(selectionBox.height*0.10)
                font.bold: true
                text: titleString
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: 30
                anchors.topMargin: 10
                Layout.preferredHeight: contentHeight
            }
            TextField{
                id: textField
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 50
                //make space for the prefix
                property int prefixIndent: Math.round(selectionBox.height*0.03)
                anchors.leftMargin: 50 + prefixIndent
//                validator: IntValidator{bottom: minValue; top: maxValue;} TODO: test validator in later QtVersions > 5.5.0
                focus: true
                text: initialTextFieldValue
                function correctValue() {
                    if(text > maxValue) text = maxValue
                    else if(text < minValue) text = minValue
                    else {
                        //remove leading "0000"
                        var temp = parseInt(text)
                        text = temp
                    }
                }
                onFocusChanged: correctValue()
                onEditingFinished: correctValue()

                style: TextFieldStyle {
                    textColor: "black"
                    font.pixelSize: Math.round(selectionBox.height*0.10)
                    background: Item { //bottom line with colored focus indicator
                        implicitHeight: Math.round(selectionBox.height*0.12)
                        implicitWidth: parent.width
                        Text { //prefix
                            color: "black"
                            font.pixelSize: Math.round(selectionBox.height*0.10)
                            text: prefix
                            anchors.left: parent.left
                            anchors.leftMargin: -textField.prefixIndent
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: Math.round(selectionBox.height*0.008)

                        }
                        Rectangle {
                            color: GlobalColors.accentColor
                            opacity: textField.activeFocus ? 0.8 : 0
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.leftMargin: -textField.prefixIndent
                            width: parent.width + textField.prefixIndent
                            height: Math.round(selectionBox.height*0.005)
                        }
                        Rectangle {
                            color: "black"
                            opacity: textField.activeFocus ? 0.6 : 0.4
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: Math.round(selectionBox.height*0.005)
                            anchors.left: parent.left
                            anchors.leftMargin: - textField.prefixIndent
                            width: parent.width + textField.prefixIndent
                            height: Math.round(selectionBox.height*0.005)
                        }
                    }
                }
            }
            Slider {
                id: slider
                width: parent.width
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: 50
                anchors.leftMargin: 50
                maximumValue: maxValue
                minimumValue: minValue
                //don't initialize the slider value to avoid the stepSize kills the preselection Value
                //because of the binding to the TextField value
                stepSize: minimumValue * 10
                on__HandlePosChanged: {
                    if (slider.value > minimumValue && slider.value < maximumValue)
                        textField.text = slider.value - minimumValue
                    else
                        textField.text = slider.value
                }
                style: SliderStyle {
                    handle: Rectangle {
                        width: Math.round(selectionBox.height*0.12)
                        height: width
                        radius: width
                        antialiasing: true
                        color: Qt.lighter(GlobalColors.accentColor, 1.2)
                    }
                    groove: Item {
                        implicitHeight: Math.round(selectionBox.height*0.04)
                        implicitWidth: slider.width
                        Rectangle {
                            height: Math.round(selectionBox.height*0.03)
                            width: slider.width
                            anchors.verticalCenter: parent.verticalCenter
                            color: "#666"
                            Rectangle {
                                antialiasing: true
                                radius: 1
                                color: GlobalColors.accentColor
                                height: parent.height
                                width: parent.width * control.value / control.maximumValue
                            }
                        }
                    }
                }

            }

            RowLayout { //bottom button line
                width: parent.width
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 30
                anchors.bottomMargin: 20
                spacing: okButton.contentWidth
                Layout.preferredHeight: cancelButton.contentHeight

                Text {
                    id: cancelButton
                    font.pixelSize: Math.round(selectionBox.height*0.07)
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
                    font.pixelSize: Math.round(selectionBox.height*0.07)
                    font.bold: true
                    color: GlobalColors.accentColor
                    text: qsTr("OK")
                    Layout.preferredHeight: contentHeight
                    MouseArea {
                        id: okMouse
                        anchors.fill: parent
                        onClicked: {
                            textField.correctValue();
                            selected(textField.text);
                        }
                    }
                }
            }
        }
    }
}

