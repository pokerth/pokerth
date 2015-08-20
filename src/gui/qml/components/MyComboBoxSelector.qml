import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "styles"

Item {
    id: root
    anchors.fill: parent

    property ListModel selectionModel: ListModel {}
    property string valueFromParent
    property string titleString
    property string returnValue
    property bool valueIsIndex

    signal accepted

    function show(title, list, vl, valueIndex) {
        titleString = title;
        valueFromParent = vl;
        returnValue = vl; // <-- initial the returnValue with the preselection to prevent a NULL return when pressing Cancel
        valueIsIndex = valueIndex;
        //fill the model
        selectionModel.clear();
        for (var i=0; i < list.count; i++) {
            selectionModel.append({"valueString": list.get(i).value});
        }
        mySelector.show()
    }

    function selected(newString, index) {
        if(valueIsIndex) {
            returnValue = index.toString();
        }
        else {
            returnValue = newString
        }
        mySelector.hide()
        root.accepted()
    }

    MyAbstractSelector {
        id: mySelector
        titleText: titleString
        button1Text: qsTr("CANCEL")
        onButton1Clicked: hide()

        container: ScrollView {
            anchors.fill:parent
            ListView {
                anchors.fill: parent
                spacing: Math.round(appWindow.height*0.05)
                model: selectionModel
                delegate: RadioButton {
                    id: radioBtn
                    //check of value is index type and do the corresponding checked? test
                    checked: valueIsIndex ? (parseInt(valueFromParent) == index ? true : false) : (valueFromParent == valueString ? true : false)
                    onClicked: {
                        root.selected(valueString, index)
                    }
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtn
                        labelString: valueString
                    }
                }
            }
        }
    }
}

