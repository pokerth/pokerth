import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

Item {
    Image {
        id: background
        source: "../../images/startViewBackground.png"
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
    }
    ColumnLayout {
        anchors.fill: parent
        StartButton {
            text: qsTr("Local game")
            page: "../createlocalgameview/CreateLocalGameView.qml"
        }

        StartButton {
            text: qsTr("Internet game")
            page: "JoinInternetGameView.qml"
        }

        StartButton {
            text: qsTr("Logs")
            page: "LogsView.qml"
        }
    }

    function setupToolBar() {
        toolBarRightButton.myIconName = "more";
        toolBarLeftButton.myIconName = "PokerTH";
    }

    Component.onCompleted: {
        setupToolBar();
    }
    onVisibleChanged: {
        if(visible) setupToolBar()
    }
}
