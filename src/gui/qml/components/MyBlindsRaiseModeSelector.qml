import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "../js/tools.js" as GlobalTools
import "styles"

Rectangle {
    id: selector
    z:1000
    visible: false
    anchors.fill: parent
    color: "#88000000" //dark transparent background

    property string titleString: ""
    property bool alwaysDoubleBlinds: false
    property var manualBlindsList
    property bool afterMBAlwaysDoubleBlinds: false
    property bool afterMBAlwaysRaiseAbout: false
    property bool afterMBStayAtLastBlind: false
    property int afterMBAlwaysRaiseValue: 0
    property bool ready: false

    function show(doubleBlinds, list, afterMBDouble, afterMBRaise, afterMBRaiseValue, afterMBStay) {
        alwaysDoubleBlinds = doubleBlinds;
        manualBlindsList = list;
        afterMBAlwaysDoubleBlinds = afterMBDouble;
        afterMBAlwaysRaiseAbout = afterMBRaise;
        afterMBAlwaysRaiseValue = afterMBRaiseValue;
        afterMBStayAtLastBlind = afterMBStay;
        visible = true;

//        if(raiseOnHandsType == "1") {
//            textFieldHandsInterval.focus = true;
//            radioBtnRaiseOnHands.checked = true;
//        }
//        else {
//            textFieldMinutesInterval.focus = true;
//            radioBtnRaiseOnMinutes.checked = true;
//        }
   }

    function reject() {
        visible = false;
   }

    function selected(doubleBlinds, list, afterMBDouble, afterMBRaise, afterMBRaiseValue, afterMBStay) {
        alwaysDoubleBlinds = doubleBlinds;
        manualBlindsList = list;
        afterMBAlwaysDoubleBlinds = afterMBDouble;
        afterMBAlwaysRaiseAbout = afterMBRaise;
        afterMBAlwaysRaiseValue = afterMBRaiseValue;
        afterMBStayAtLastBlind = afterMBStay;
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
                id: contentColumnLayout
                anchors.fill: parent
                spacing: AppStyle.columnLayoutSpacing

//                TODO always double or manual list






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
//                                textFieldHandsInterval.correctValue();
//                                textFieldMinutesInterval.correctValue();
                                //return values
//                                selected(radioBtnRaiseOnHands.checked ? "1": "0", textFieldHandsInterval.text, textFieldMinutesInterval.text);
                            }
                        }
                    }
                }
            }
        }
    }
    Component.onCompleted: ready = true;
}
