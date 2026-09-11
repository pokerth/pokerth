import QtQuick

import "../config" as Config

// Input bar for the player rating (0–5 stars) in the note dialog.
// Deliberately Unicode glyphs (★/☆) instead of SVG icons – as in the Qt widgets
// client: they scale with the font size and can be colourised without needing
// a separate raster image for every size.
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

            // A click on star i sets the rating to i; clicking the last set star
            // again takes it back (i−1). That way you get back to 0 without an
            // additional button.
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
