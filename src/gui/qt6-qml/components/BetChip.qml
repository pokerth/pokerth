import QtQuick

// Einsatz-Anzeige: Chip-Symbol + „$Betrag". Poppt beim Erscheinen kurz auf.
// Sichtbarkeit/Position setzt der Aufrufer (z.B. visible: amount > 0).
Row {
    id: betChip
    property int amount: 0
    property color textColor: "#eff1f5"
    spacing: 2
    transformOrigin: Item.Center

    onVisibleChanged: if (visible) betPop.restart()
    SequentialAnimation {
        id: betPop
        NumberAnimation { target: betChip; property: "scale"; from: 0.5; to: 1.15; duration: 110; easing.type: Easing.OutQuad }
        NumberAnimation { target: betChip; property: "scale"; to: 1.0; duration: 130; easing.type: Easing.OutBack }
    }

    Image {
        width: 16; height: 16
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:resources/chipStack.svg"
        fillMode: Image.PreserveAspectFit
    }
    AppText {
        anchors.verticalCenter: parent.verticalCenter
        color: betChip.textColor
        font.pixelSize: 11
        font.bold: true
        text: "$" + betChip.amount
    }
}
