import QtQuick 2.5
import QtQuick.Controls 1.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.2
import "../js/colors.js" as GlobalColors
import "styles"

Item {
    id: root
    anchors.fill: parent

    property ListModel internalValuesList: ListModel {}
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
        internalValuesList.clear();
        for (var i=0; i < list.length; i++) {
            internalValuesList.append({"value": list[i]});
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
            id: scrollView
            anchors.fill:parent
            property int yOfCheckedRadioButton: 0
            ListView {
                id:listView
                anchors.fill: parent
                spacing: Math.round(appWindow.height*0.05)
                model: internalValuesList
                delegate: RadioButton {
                    id: radioBtn
                    //check of value is index type and do the corresponding checked? test
                    checked: valueIsIndex ? (parseInt(valueFromParent) == index ? true : false) : (valueFromParent == value ? true : false)
                    onClicked: {
                        root.selected(value, index)
                    }
                    style: MyRadioButtonStyle {
                        myRadioBtn: radioBtn
                        labelString: value
                    }
                    Component.onCompleted: {
                        //set the position of the checked RadioButton to scroll to it later onContentHeightChange
                        if(checked) {
                            var checkedRadioBtnPositionY = Math.round((radioBtn.height + listView.spacing) * index - radioBtn.height * 1.5)
                            if( checkedRadioBtnPositionY > 0)
                                scrollView.yOfCheckedRadioButton = checkedRadioBtnPositionY
                            else
                                scrollView.yOfCheckedRadioButton = 0
                        }
                    }
                }
                onContentHeightChanged: {
                    //scroll to the checked RadioButton
                    contentY = scrollView.yOfCheckedRadioButton
                }
            }
        }
    }
}

