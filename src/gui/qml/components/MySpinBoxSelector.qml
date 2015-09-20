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
    property string prefix: ""
    property string returnValue: ""
    property string myIdString: ""
    property int minValue: 0
    property int maxValue: 0
    property int initialTextFieldValue: 0
    property int initialSliderValue: 0
    property bool ready: false

    signal accepted

    function show(myId, myTitle, myMinValue, myMaxValue, myValue, myPrefix) {
        myIdString = myId;
        titleString = myTitle;
        minValue = myMinValue;
        maxValue = myMaxValue;
        returnValue = myValue;
        textField.text = myValue;
        prefix = myPrefix;

        mySelector.show()
//        textField.focus = true;
    }

    function selected(newSelection) {
        returnValue = newSelection;
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
            textField.correctValue();
            root.selected(textField.text);
        }

        container: ColumnLayout {
            anchors.fill: parent
            TextField {
                id: textField
                anchors.left: parent.left
                anchors.right: parent.right
                //make space for the prefix
                anchors.leftMargin: AppStyle.spinBoxSelectorTextFieldPrefixExtraLeftMargin
                validator: IntValidator{bottom: minValue; top: maxValue;}

                function correctValue() {
                    text = GlobalTools.correctTextFieldIntegerValue(root, text, minValue);
                }

                onFocusChanged: { if(root.ready && !focus) correctValue(); }
                onEditingFinished: { if(root.ready) correctValue(); }

                style: MyTextFieldStyle {
                    prefix: root.prefix
                    myTextField: textField
                }
            }
            Slider {
                id: slider
                width: parent.width
                anchors.left: parent.left
                anchors.right: parent.right
                maximumValue: maxValue
                minimumValue: minValue
                //don't initialize the slider value to avoid the stepSize kills the preselection Value
                //because of the binding to the TextField value
                stepSize: minimumValue * 10
                on__HandlePosChanged: {
                    if(root.ready) { //wait until the whole component is ready, otherwise setting min and max changes text field value
                        if (slider.value > minimumValue && slider.value < maximumValue)
                            textField.text = slider.value - minimumValue
                        else
                            textField.text = slider.value
                    }
                }
                style: SliderStyle {
                    handle: Rectangle {
                        width: AppStyle.sliderHandleWidth
                        height: width
                        radius: width
                        antialiasing: true
                        color: Qt.lighter(GlobalColors.accentColor, 1.2)
                    }
                    groove: Item {
                        implicitHeight: AppStyle.sliderGrooveHeight
                        implicitWidth: slider.width
                        Rectangle {
                            height: AppStyle.sliderGrooveRectHeight
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
        }
    }
    Component.onCompleted: ready = true
}
