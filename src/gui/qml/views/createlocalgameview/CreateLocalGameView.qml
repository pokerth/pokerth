import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import "../../components"

Rectangle {
    id: createLocalGameViewRoot
    property string myReadableId: "createLocalGameViewRoot"
    width: parent.width
    height: parent.height
    color: "#FAFAFA"

    ScrollView {
        width: parent.width
        height: parent.height

        ListView {
            id: createLocalGameViewList
            anchors.fill: parent
            model: CreateLocalGameViewData
            delegate: MyListViewDelegateFactory {
                myListViewsHeight: createLocalGameViewList.height
                myListViewsWidth: createLocalGameViewList.width
                myListViewRoot: createLocalGameViewRoot //set this to be able to display the selector over the whole view
                myListViewModel: CreateLocalGameViewData
            }
            Component.onCompleted: {
                CreateLocalGameViewImpl.readConfigValues()
            }
        }
    }

    function getListElementValueString(elementName) {
        //read model and check for the elementName
        for (var i = 0; i < CreateLocalGameViewData.lenght; i++ ) {
            if(CreateLocalGameViewData[i].myId == elementName) {
                return CreateLocalGameViewDat[i].myValue;
            }
        }
    }

    function setupToolBar() {
        toolbar.visible = true;
        toolBarRightButton.myIconName = "accept";
        toolBarLeftButton.myIconName = "back";
        //send start signal to the session
        toolBarRightButton.clicked.connect(CreateLocalGameViewImpl.startGame);
    }

    function clearToolBar() {
        toolBarRightButton.clicked.disconnect(CreateLocalGameViewImpl.startGame);
    }

    Component.onCompleted: {
        setupToolBar();
    }
    onVisibleChanged: {
        if(visible) setupToolBar();
        else clearToolBar();
    }

}
