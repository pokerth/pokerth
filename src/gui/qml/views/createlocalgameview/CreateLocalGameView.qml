import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import "../../components"

Rectangle {
    id: createLocalGameViewRoot
    width: parent.width
    height: parent.height
    color: "#FAFAFA"

    ScrollView {
        width: parent.width
        height: parent.height

        ListView {
            id: createLocalGameViewList
            anchors.fill: parent
            model: ListModel {
                id: createLocalGameViewModel
                ListElement {
                    myId: "comboBox_NumberOfPlayers"
                    myTitle: qsTr("Number of players")
                    myType: "ComboBox"
                    myValuesList: [
                        ListElement { value: "10" },
                        ListElement { value: "9" },
                        ListElement { value: "8" },
                        ListElement { value: "7" },
                        ListElement { value: "6" },
                        ListElement { value: "5" },
                        ListElement { value: "4" },
                        ListElement { value: "3" },
                        ListElement { value: "2" }
                    ]
                    myValueIsIndex: false
                    myValue: ""
                }
                ListElement {
                    myId: "spinBox_StartCash"
                    myTitle: qsTr("Start cash")
                    myType: "SpinBox"
                    myMaxValue: 1000000
                    myMinValue: 1000
                    myPrefix: "$"
                    myValue: ""
                }
                ListElement {
                    myId: "comboBox_Blinds"
                    myTitle: qsTr("Blinds")
                    myType: "ComboBox"
                    myValuesList: [
                        ListElement { value: qsTr("Use saved blinds settings") },
                        ListElement { value: qsTr("Change blinds settings ...") }
                    ]
                    myValue: ""
                    myValueIsIndex: true
                }
                ListElement {
                    myId: "comboBox_GameSpeed"
                    myTitle: qsTr("Game speed")
                    myType: "ComboBox"
                    myValuesList: [
                        ListElement { value: "11" },
                        ListElement { value: "10" },
                        ListElement { value: "9" },
                        ListElement { value: "8" },
                        ListElement { value: "7" },
                        ListElement { value: "6" },
                        ListElement { value: "5" },
                        ListElement { value: "4" },
                        ListElement { value: "3" },
                        ListElement { value: "2" },
                        ListElement { value: "1" }
                    ]
                    myValue: ""
                    myValueIsIndex: false
                }
                ListElement {
                    myId: "comboBox_GameSpeed"
                    myTitle: qsTr("Game speed")
                    myType: "ComboBox"
                    myValuesList: [
                        ListElement { value: "11" },
                        ListElement { value: "10" },
                        ListElement { value: "9" },
                        ListElement { value: "8" },
                        ListElement { value: "7" },
                        ListElement { value: "6" },
                        ListElement { value: "5" },
                        ListElement { value: "4" },
                        ListElement { value: "3" },
                        ListElement { value: "2" },
                        ListElement { value: "1" }
                    ]
                    myValue: ""
                    myValueIsIndex: false
                }

                Component.onCompleted: {
                    createLocalGameViewModel.setProperty(0, "myValue", Config.readConfigInt("NumberOfPlayers"));
                    createLocalGameViewModel.setProperty(1, "myValue", Config.readConfigInt("StartCash"));
                    createLocalGameViewModel.setProperty(2, "myValue", "0");
                    createLocalGameViewModel.setProperty(3, "myValue", Config.readConfigInt("GameSpeed"));
                    createLocalGameViewModel.setProperty(4, "myValue", Config.readConfigInt("GameSpeed"));
                }
            }
            delegate: MyListViewDelegateFactory {
                myListViewsHeight: createLocalGameViewList.height
                myListViewsWidth: createLocalGameViewList.width
                myListViewRoot: createLocalGameViewRoot
                myListViewModel: createLocalGameViewModel
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
