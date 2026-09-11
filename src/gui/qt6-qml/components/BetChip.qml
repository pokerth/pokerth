import QtQuick

// Bet display: chip symbol + "$amount". Pops briefly when it appears.
// Visibility/position is set by the caller (e.g. visible: amount > 0).
// The sizes are adjustable so that the same chip can sit both freely next
// to the box (seat style "classic") and in the narrower box base ("inset",
// see PlayerBetStrip).
Row {
    id: betChip
    property int amount: 0
    property color textColor: "#eff1f5"
    property int iconSize: 20
    property int fontSize: 14
    spacing: 2
    transformOrigin: Item.Center

    onVisibleChanged: if (visible) betPop.restart()
    SequentialAnimation {
        id: betPop
        NumberAnimation { target: betChip; property: "scale"; from: 0.5; to: 1.15; duration: 110; easing.type: Easing.OutQuad }
        NumberAnimation { target: betChip; property: "scale"; to: 1.0; duration: 130; easing.type: Easing.OutBack }
    }

    Image {
        width: betChip.iconSize; height: betChip.iconSize
        anchors.verticalCenter: parent.verticalCenter
        source: "qrc:resources/chipStack.svg"
        fillMode: Image.PreserveAspectFit
    }
    AppText {
        anchors.verticalCenter: parent.verticalCenter
        color: betChip.textColor
        font.pixelSize: betChip.fontSize
        font.bold: true
        text: "$" + betChip.amount
    }
}
