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
                    myMaxValue: "1000000"
                    myMinValue: "1000"
                    myPrefix: "$"
                    myValue: ""
                }
                ListElement {
                    myId: "spinBox_StartBlind"
                    myTitle: qsTr("Start blind")
                    myType: "SpinBox"
                    myMaxValue: "20000"
                    myMinValue: "5"
                    myPrefix: "$"
                    myValue: ""
                }                
                ListElement {
                    myId: "selector_BlindsRaiseInterval"
                    myTitle: qsTr("Blinds raise interval")
                    myType: "BlindsRaiseInterval"
                    myRaiseOnHandsType: "" //if false it is raise on minutes type
                    myRaiseOnHandsInterval: ""
                    myRaiseOnMinutesInterval: ""
                }
                ListElement {
                    myId: "selector_BlindsRaiseMode"
                    myTitle: qsTr("Blinds raise mode")
                    myType: "BlindsRaiseMode"
                    myAlwaysDoubleBlinds: "" //if false it is "Manual Blinds Order"
                    myManualBlindsList: []
                    myAfterMBAlwaysDoubleBlinds: ""
                    myAfterMBAlwaysRaiseAbout: ""
                    myAfterMBAlwaysRaiseValue: ""
                    myAfterMBStayAtLastBlind: ""
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
                    //set Config Values from config file
                    createLocalGameViewModel.setProperty(0, "myValue", Config.readConfigIntString("NumberOfPlayers"));
                    createLocalGameViewModel.setProperty(1, "myValue", Config.readConfigIntString("StartCash"));
                    createLocalGameViewModel.setProperty(2, "myValue", Config.readConfigIntString("FirstSmallBlind"));
                    createLocalGameViewModel.setProperty(3, "myRaiseOnHandsType", Config.readConfigIntString("RaiseBlindsAtHands"));
                    createLocalGameViewModel.setProperty(3, "myRaiseOnHandsInterval", Config.readConfigIntString("RaiseSmallBlindEveryHands"));
                    createLocalGameViewModel.setProperty(3, "myRaiseOnMinutesInterval", Config.readConfigIntString("RaiseSmallBlindEveryMinutes"));
                    createLocalGameViewModel.setProperty(4, "myAlwaysDoubleBlinds", Config.readConfigIntString("AlwaysDoubleBlinds"));

                    var tempList = Config.readConfigIntStringList("ManualBlindsList");
                    for(var i=0; i < tempList.length; i++) {
                        createLocalGameViewModel.get(4).myManualBlindsList.append({"blindValue": tempList[i].toString()})
                    }

                    createLocalGameViewModel.setProperty(4, "myAfterMBAlwaysDoubleBlinds", Config.readConfigIntString("AfterMBAlwaysDoubleBlinds"));
                    createLocalGameViewModel.setProperty(4, "myAfterMBAlwaysRaiseAbout", Config.readConfigIntString("AfterMBAlwaysRaiseAbout"));
                    createLocalGameViewModel.setProperty(4, "myAfterMBAlwaysRaiseValue", Config.readConfigIntString("AfterMBAlwaysRaiseValue"));
                    createLocalGameViewModel.setProperty(4, "myAfterMBStayAtLastBlind", Config.readConfigIntString("AfterMBStayAtLastBlind"));
                    createLocalGameViewModel.setProperty(5, "myValue", Config.readConfigIntString("GameSpeed"));
                }
            }
            delegate: MyListViewDelegateFactory {
                myListViewsHeight: createLocalGameViewList.height
                myListViewsWidth: createLocalGameViewList.width
                myListViewRoot: createLocalGameViewRoot
                myListViewModel: createLocalGameViewModel

//                TODO --> Blindsfelder immer mit anzeigen spezialfeld manual blinds settings direkt hier inline implementieren


            }
        }
    }

    function getListElementValueString(elementName) {
        //read model and check for the elementName
        for (var i = 0; i < createLocalGameViewModel.count; i++ ) {
            if(createLocalGameViewModel.get(i).myId == elementName) {
                return createLocalGameViewModel.get(i).myValue;
            }
        }
    }

    function setupToolBar() {
        toolbar.visible = true;
        toolBarRightButton.myIconName = "accept";
        toolBarLeftButton.myIconName = "back";
        //send start signal to the session
        toolBarRightButton.clicked.connect(StartViewImpl.startLocalGame);
    }

    function clearToolBar() {
        toolBarRightButton.clicked.disconnect(StartViewImpl.startLocalGame);
    }

    Component.onCompleted: {
        setupToolBar();
    }
    onVisibleChanged: {
        if(visible) setupToolBar();
        else clearToolBar();
    }

}
