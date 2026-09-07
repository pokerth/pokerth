import QtQuick

import "../config" as Config

// Eingabe-Leiste für die Spieler-Bewertung (0–5 Sterne) im Notiz-Dialog.
// Bewusst Unicode-Glyphen (★/☆) statt SVG-Icons – wie im Qt-Widgets-Client:
// sie skalieren mit der Schriftgröße und lassen sich einfärben, ohne dass für
// jede Größe ein eigenes Raster nötig wäre.
Row {
    id: root

    property int rating: 0
    property int starSize: 22
    property color starColor: Config.Theme.colorAccent

    spacing: Math.round(starSize * 0.25)

    Repeater {
        model: 5

        Item {
            id: cell
            required property int index

            width: star.implicitWidth
            height: star.implicitHeight

            AppText {
                id: star
                anchors.centerIn: parent
                text: cell.index < root.rating ? "★" : "☆"
                color: root.starColor
                font.pixelSize: root.starSize
                opacity: cell.index < root.rating ? 1.0 : 0.5
            }

            // Klick auf Stern i setzt die Bewertung auf i; ein erneuter Klick auf
            // den zuletzt gesetzten Stern nimmt ihn zurück (i−1). So kommt man
            // ohne zusätzlichen Knopf wieder auf 0.
            MouseArea {
                anchors.fill: parent
                anchors.margins: -4
                cursorShape: Qt.PointingHandCursor
                onClicked: root.rating = (root.rating === cell.index + 1)
                                         ? cell.index : cell.index + 1
            }
        }
    }
}
