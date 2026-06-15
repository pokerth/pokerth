import QtQuick

import "../config" as Config

// Auswahl-Panel für Emoji-Reaktionen (Grid, 6 Spalten – wie der Reaction-
// Picker des Web-Clients mit 30 Emojis). Sichtbarkeit/Position/z setzt der
// Aufrufer; ausgewählte Emojis werden über picked() gemeldet.
Rectangle {
    id: root

    signal picked(string emoji)

    width: reactionGrid.width + 16
    height: reactionGrid.height + 16
    radius: 8
    color: Qt.rgba(0, 0, 0, 0.88)
    border.color: Qt.rgba(1, 1, 1, 0.12)
    border.width: 1

    Grid {
        id: reactionGrid
        anchors.centerIn: parent
        columns: 6
        spacing: 3

        Repeater {
            model: ["🎉", "🥳", "👏", "🙌", "💪", "🤣",
                    "😂", "😬", "🤦", "😴", "👍", "😎",
                    "🤩", "👀", "🤔", "😱", "😡", "😤",
                    "🔥", "😮", "💰", "💎", "🎰", "🍀",
                    "🃏", "💀", "🤑", "🫵", "🫡", "🤫"]
            delegate: Rectangle {
                required property string modelData
                width: 36; height: 36; radius: 6
                color: reactArea.containsPress ? Qt.rgba(1, 1, 1, 0.25)
                     : reactArea.containsMouse ? Qt.rgba(1, 1, 1, 0.12)
                     : "transparent"
                scale: reactArea.containsMouse && !reactArea.containsPress ? 1.15 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    text: parent.modelData
                    font.family: Config.StaticData.emojiFamily
                    font.pixelSize: 19
                }
                MouseArea {
                    id: reactArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.picked(parent.modelData)
                }
            }
        }
    }
}
