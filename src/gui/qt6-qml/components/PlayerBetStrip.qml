import QtQuick

import "../config" as Config

// Einsatz-Sockel INNERHALB einer Spielerbox (Sitz-Stil "inset"). Sitzt bündig
// am unteren Boxrand, 1 px innerhalb des Rahmens von PlayerBoxBackground,
// damit dessen Rand (bei Tisch-Stilen mit <PlayerBoxAccent> deutlich sichtbar)
// nicht überdeckt wird.
//
// Der Sockel ist IMMER da, sobald der Stil aktiv ist – auch ohne Einsatz. Nur
// so bleibt die Boxhöhe über die ganze Hand konstant; sonst würde jede Aktion
// eines Spielers die Box wachsen/schrumpfen lassen und – über die Bisektion in
// GamePage.boxScale – den gesamten Tisch neu skalieren. Der Betrag selbst wird
// nur bei bet > 0 eingeblendet.
Item {
    id: strip

    property int amount: 0

    // Höhe abzüglich der 1 px Einrückung nach unten (s. anchors unten).
    height: Math.max(0, Config.SeatStyle.betStripHeight - 1)

    Rectangle {
        anchors.fill: parent
        // Nur unten runden – oben schließt der Sockel bündig an den Boxkörper an.
        bottomLeftRadius: 5
        bottomRightRadius: 5
        color: Qt.rgba(0, 0, 0, 0.26)

        // Trennlinie zum Info-Bereich darüber: markiert den Sockel auch dann,
        // wenn (noch) kein Einsatz drinsteht.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: Qt.rgba(1, 1, 1, 0.10)
        }
    }

    BetChip {
        anchors.centerIn: parent
        visible: strip.amount > 0
        amount: strip.amount
        iconSize: 15
        fontSize: 13
        textColor: "#eff1f5"
    }
}
