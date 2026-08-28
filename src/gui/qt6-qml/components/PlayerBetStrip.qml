import QtQuick

import "../config" as Config

// Einsatz-Sockel INNERHALB einer Spielerbox (Sitz-Stil "inset"). Sitzt bündig
// am unteren Boxrand, 1 px innerhalb des Rahmens von PlayerBoxBackground,
// damit dessen Rand (bei Tisch-Stilen mit <PlayerBoxAccent> deutlich sichtbar)
// nicht überdeckt wird.
//
// Der Sockel klappt nur auf, solange der Spieler tatsächlich etwas gesetzt hat
// (`open`); die Box wächst dabei um seine Höhe. Der PLATZ dafür ist am Tisch
// permanent reserviert (s. tableZone.betStripH in GamePage) – das Auf- und
// Zuklappen verschiebt also weder die Nachbarboxen noch die Tisch-Skalierung.
// clip, damit der Inhalt während der Animation nicht aus der Box ragt.
Item {
    id: strip

    property int amount: 0
    // Aufgeklappt? Steuert die Höhe – die Box folgt ihr.
    property bool open: false

    // Volle Sockelhöhe im aufgeklappten Zustand – exakt die Höhe, die am Tisch
    // dafür reserviert ist. Die 1 px Einrückung zum Boxrand steckt im inneren
    // Rechteck, damit die Höhenrechnung der Box glatt bleibt.
    readonly property int openHeight: Config.SeatStyle.betStripHeight

    height: open ? openHeight : 0
    clip: true
    Behavior on height { NumberAnimation { duration: 140; easing.type: Easing.OutQuad } }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 1
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        height: strip.openHeight - 1
        // Nur unten runden – oben schließt der Sockel bündig an den Boxkörper an.
        bottomLeftRadius: 5
        bottomRightRadius: 5
        color: Qt.rgba(0, 0, 0, 0.26)

        // Trennlinie zum Info-Bereich darüber.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: Qt.rgba(1, 1, 1, 0.10)
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
}
