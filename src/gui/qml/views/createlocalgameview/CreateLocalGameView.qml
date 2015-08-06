import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import "../../components"

Rectangle {

    width: parent.width
    height: parent.height
    color: "#FAFAFA"

    MyComboBoxSelector {
        id: myComboBoxSelector
    }

    ScrollView {
        width: parent.width
        height: parent.height

        ListView {
            id:myListView
            anchors.fill: parent
            model: ListModel {
                ListElement {
                    myId: "comboBox_NumberOfPlayers"
                    myTitle: qsTr("Number of players")
                    myType: "ComboBox"
                    myValue: "10"
                    //                    property var myValuesInt: [10, 9, 8, 7, 6, 5, 4, 3, 2]
                }
                ListElement {
                    myId: "spinBox_StartCash"
                    myTitle: qsTr("Start cash")
                    myType: "SpinBox"
                    myMaxValue: 10000000
                    myMinValue: 1000
                    myPrefix: "$"
                    myValue: "5000"
                }
                ListElement {
                    myId: "comboBox_Blinds"
                    myTitle: qsTr("Blinds")
                    myType: "ComboBox"
                    myValuesList: [
                       ListElement { value: qsTr("Use saved blinds settings") },
                       ListElement { value: qsTr("Change blinds settings ...") }
                    ]
                    myValue: "0"
                }
                ListElement {
                    myId: "comboBox_GameSpeed"
                    myTitle: qsTr("Game speed")
                    myType: "ComboBox"
                    //                    myValuesInt: [11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
                    myValue: "5"
                }
                ListElement {
                    myId: "comboBox_GameSpeed"
                    myTitle: qsTr("Game speed")
                    myType: "ComboBox"
                    //                    myValuesInt: [11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
                    myValue: "5"
                }
            }
            delegate: ListViewDelegateFactory {
                myListViewsHeight: myListView.height
                myListViewsWidth: myListView.width
            }
        }
    }

    function setupToolBar() {
        toolBarRightButton.myIconName = "accept";
        toolBarLeftButton.myIconName = "back";
    }

    Component.onCompleted: {
        setupToolBar();
    }
    onVisibleChanged: {
        if(visible) setupToolBar()
    }

}
