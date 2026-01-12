import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../config" as Config
import "../components"

Rectangle {
    id: lobbyPage
    width: mainWindow.width
    height: mainWindow.height
    color: Config.StaticData.palette.secondary.col700

    // Mock data for development
    property int connectedPlayers: 42
    property int runningGames: 7
    property int openGames: 3

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Top section: Filters and search
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: searchPlayerField
                Layout.preferredWidth: 200
                placeholderText: qsTr("search for player ...")
                font.family: Config.StaticData.loadedFont.font.family
                color: Config.StaticData.palette.secondary.col200
                background: Rectangle {
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.3)
                    radius: 3
                }
                placeholderTextColor: Qt.lighter(Config.StaticData.palette.secondary.col200, 1.5)
            }

            ComboBox {
                id: gameListFilter
                Layout.fillWidth: true
                font.family: Config.StaticData.loadedFont.font.family
                model: [
                    qsTr("No game list filter"),
                    qsTr("Show open games"),
                    qsTr("Show open & non-full games"),
                    qsTr("Show open & non-full & non-private games"),
                    qsTr("Show open & non-full & private games"),
                    qsTr("Show open & non-full & ranking games")
                ]
            }
        }

        // Main content area
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // Left side: Player List
            Rectangle {
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                radius: 5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5

                    Label {
                        text: qsTr("Available Players")
                        font.family: Config.StaticData.loadedFont.font.family
                        font.bold: true
                        color: Config.StaticData.palette.secondary.col200
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ListView {
                        id: playerListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        
                        model: Lobby.playerListModel

                        delegate: ItemDelegate {
                            width: playerListView.width
                            height: 30
                            
                            contentItem: Text {
                                text: model.playerName
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 12
                                color: model.isAdmin ? "#FFD700" : Config.StaticData.palette.secondary.col200
                                font.bold: model.isAdmin
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            background: Rectangle {
                                color: parent.hovered ? Qt.lighter(Config.StaticData.palette.secondary.col700, 1.2) : "transparent"
                                radius: 3
                            }
                        }
                    }
                }
            }

            // Center: Game List and Game Info
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 5

                // Game List
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 5

                        // Game List Header
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Label {
                                text: qsTr("Game")
                                font.family: Config.StaticData.loadedFont.font.family
                                font.bold: true
                                color: Config.StaticData.palette.secondary.col200
                                Layout.fillWidth: true
                            }
                            Label {
                                text: qsTr("Players")
                                font.family: Config.StaticData.loadedFont.font.family
                                font.bold: true
                                color: Config.StaticData.palette.secondary.col200
                                Layout.preferredWidth: 60
                            }
                            Label {
                                text: qsTr("State")
                                font.family: Config.StaticData.loadedFont.font.family
                                font.bold: true
                                color: Config.StaticData.palette.secondary.col200
                                Layout.preferredWidth: 80
                            }
                            Label {
                                text: qsTr("Blind")
                                font.family: Config.StaticData.loadedFont.font.family
                                font.bold: true
                                color: Config.StaticData.palette.secondary.col200
                                Layout.preferredWidth: 60
                            }
                        }

                        // Game List Items
                        ListView {
                            id: gameListView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            model: Lobby.gameListModel

                            delegate: ItemDelegate {
                                width: gameListView.width
                                height: 35

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 5

                                    Text {
                                        text: model.gameName || ("Game #" + model.gameId)
                                        font.family: Config.StaticData.loadedFont.font.family
                                        color: Config.StaticData.palette.secondary.col200
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: (model.playerCount || 0) + "/" + (model.maxPlayers || 10)
                                        font.family: Config.StaticData.loadedFont.font.family
                                        color: Config.StaticData.palette.secondary.col200
                                        Layout.preferredWidth: 60
                                    }
                                    Text {
                                        text: (model.playerCount || 0) < (model.maxPlayers || 10) ? "Open" : "Full"
                                        font.family: Config.StaticData.loadedFont.font.family
                                        color: (model.playerCount || 0) < (model.maxPlayers || 10) ? "#4CAF50" : "#FFC107"
                                        Layout.preferredWidth: 80
                                    }
                                    Text {
                                        text: "10/20"
                                        font.family: Config.StaticData.loadedFont.font.family
                                        color: Config.StaticData.palette.secondary.col200
                                        Layout.preferredWidth: 60
                                    }
                                }

                                background: Rectangle {
                                    color: parent.hovered ? Qt.lighter(Config.StaticData.palette.secondary.col700, 1.3) : "transparent"
                                    radius: 3
                                }
                                
                                onClicked: {
                                    Lobby.joinGame(model.gameId)
                                }
                            }
                        }
                    }
                }

                // Action Buttons
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

Label {
                            text: qsTr("Player: %1").arg(Lobby.myPlayerName !== "" ? Lobby.myPlayerName : "Guest")
                            font.family: Config.StaticData.loadedFont.font.family
                            font.bold: true
                            font.pixelSize: 14
                            color: Config.StaticData.palette.secondary.col100
                        }

                        Button {
                            text: qsTr("Create Game")
                            font.family: Config.StaticData.loadedFont.font.family
                            Layout.fillWidth: true
                            onClicked: Lobby.createGame()
                    }

                    Button {
                        text: qsTr("Join Game")
                        font.family: Config.StaticData.loadedFont.font.family
                        Layout.fillWidth: true
                        enabled: gameListView.currentIndex >= 0
                        onClicked: {
                            console.log("Join Game clicked")
                        }
                    }
                }
            }

            // Right side: Game Info and Chat
            ColumnLayout {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                spacing: 10

                // Game Info
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 5

                        Label {
                            text: qsTr("Game Info")
                            font.family: Config.StaticData.loadedFont.font.family
                            font.bold: true
                            font.pixelSize: 14
                            color: Config.StaticData.palette.secondary.col200
                        }

                        Label {
                            text: qsTr("Select a game to see details")
                            font.family: Config.StaticData.loadedFont.font.family
                            font.pixelSize: 12
                            color: Config.StaticData.palette.secondary.col300
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }

                // Lobby Chat
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.2)
                    radius: 5

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 5
                        spacing: 5

                        Label {
                            text: qsTr("Lobby Chat")
                            font.family: Config.StaticData.loadedFont.font.family
                            font.bold: true
                            color: Config.StaticData.palette.secondary.col200
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: chatArea
                                readOnly: true
                                wrapMode: TextEdit.Wrap
                                font.family: Config.StaticData.loadedFont.font.family
                                font.pixelSize: 12
                                color: Config.StaticData.palette.secondary.col200
                                background: Rectangle {
                                    color: "transparent"
                                }
                                text: qsTr("Welcome to PokerTH Lobby!\nChat messages will appear here...")
                            }
                        }

                        TextField {
                            id: chatInput
                            Layout.fillWidth: true
                            placeholderText: qsTr("Type your message...")
                            font.family: Config.StaticData.loadedFont.font.family
                            color: Config.StaticData.palette.secondary.col200
                            background: Rectangle {
                                color: Qt.darker(Config.StaticData.palette.secondary.col700, 1.5)
                                radius: 3
                            }
                            placeholderTextColor: Qt.lighter(Config.StaticData.palette.secondary.col200, 1.5)
                            
                            onAccepted: sendChatMessage()
                            
                            Keys.onReturnPressed: sendChatMessage()
                        }
                    }
                }
            }
        }

        // Bottom status bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                id: connectedPlayersLabel
                text: qsTr("connected players: %1").arg(connectedPlayers)
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                color: Config.StaticData.palette.secondary.col300
            }

            Label {
                text: " | " + qsTr("running games: %1").arg(runningGames)
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                color: Config.StaticData.palette.secondary.col300
            }

            Label {
                text: " | " + qsTr("open games: %1").arg(openGames)
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                color: Config.StaticData.palette.secondary.col300
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("<a href='https://www.pokerth.net'>PokerTH.net</a>")
                font.family: Config.StaticData.loadedFont.font.family
                font.pixelSize: 12
                color: Config.StaticData.palette.secondary.col300
                textFormat: Text.RichText
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }
    
    // Chat message handler
    Connections {
        target: Lobby
        function onChatMessageReceived(playerName, message) {
            var timestamp = new Date().toLocaleTimeString(Qt.locale(), "HH:mm:ss")
            chatArea.text += "\n[" + timestamp + "] " + playerName + ": " + message
        }
    }
    
    function sendChatMessage() {
        if(chatInput.text.trim() !== "") {
            Lobby.sendChatMessage(chatInput.text)
            chatInput.text = ""
        }
    }
    
    Component.onCompleted: {
        console.log("LobbyPage loaded")
        console.log("My player name:", Lobby.myPlayerName)
        console.log("Player model count:", Lobby.playerListModel.rowCount())
        console.log("Game model count:", Lobby.gameListModel.rowCount())
    }
}
