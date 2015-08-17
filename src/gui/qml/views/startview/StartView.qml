import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

Item {
    id: startView
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
            onClicked: stackView.push(Qt.resolvedUrl(page))
            //TODO ^^ --> gleich GameTable anzeigen wenn "ShowGameSettingsDialogOnNewGame" == true
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
        toolbar.visible = true;
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
