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
    property bool afterMBAlwaysDoubleBlinds: false
    property bool afterMBAlwaysRaiseAbout: false
    property bool afterMBStayAtLastBlind: false
    property int afterMBAlwaysRaiseValue: 0
    property bool ready: false

    signal accepted

    function show(title, doubleBlinds, list, afterMBDouble, afterMBRaise, afterMBRaiseValue, afterMBStay) {
        titleString = title
        alwaysDoubleBlinds = doubleBlinds;
        manualBlindsList = list;
        afterMBAlwaysDoubleBlinds = afterMBDouble;
        afterMBAlwaysRaiseAbout = afterMBRaise;
        afterMBAlwaysRaiseValue = afterMBRaiseValue;
        afterMBStayAtLastBlind = afterMBStay;

        mySelector.show()

        //        if(raiseOnHandsType == "1") {
        //            textFieldHandsInterval.focus = true;
        //            radioBtnRaiseOnHands.checked = true;
        //        }
        //        else {
        //            textFieldMinutesInterval.focus = true;
        //            radioBtnRaiseOnMinutes.checked = true;
        //        }
    }


    function selected(doubleBlinds, list, afterMBDouble, afterMBRaise, afterMBRaiseValue, afterMBStay) {
        alwaysDoubleBlinds = doubleBlinds;
        manualBlindsList = list;
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
            //            textFieldHandsInterval.correctValue();
            //            textFieldMinutesInterval.correctValue();
            //return values
            //            root.selected(radioBtnRaiseOnHands.checked ? "1": "0", textFieldHandsInterval.text, textFieldMinutesInterval.text);
        }

        container: ScrollView {
            anchors.fill: parent
            ColumnLayout {
                width: parent.width
//                TODO Hier gehts mit dem Inhalt weiter
            }
        }

    }
    Component.onCompleted: ready = true;
}
