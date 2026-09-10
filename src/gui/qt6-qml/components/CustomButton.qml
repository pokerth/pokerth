import QtQuick
import QtQuick.Controls

import "../config" as Config

// PokerTH base button — built on AbstractButton for keyboard + accessibility support.
// Touch target is at least Theme.touchTarget (48dp on mobile, 44dp on desktop).
AbstractButton {
    id: customButton

    implicitWidth:  Config.Theme.buttonWidth < 0 ? 160 : Config.Theme.buttonWidth
    implicitHeight: Config.Theme.touchTarget

    // Fokusrahmen des Universal-Stils: Die ApplicationWindow des Stils zeichnet
    // ihn zentral für jedes Control mit dieser Eigenschaft – und nur bei
    // Tastaturfokus (visualFocus), nicht nach einem Mausklick. Ein eigener
    // Rahmen wäre eine Sonderlocke gegenüber allen anderen Controls.
    property bool useSystemFocusVisuals: true

    // Tastaturbedienung wie in Web-Formularen und Win32-Dialogen: Space löst
    // AbstractButton bereits selbst aus, Return/Enter kommt hier dazu. Das Event
    // ist damit verbraucht, sodass ein fokussierter Button Vorrang vor dem
    // Default-Button des umgebenden Formulars hat.
    Keys.onReturnPressed: customButton.clicked()
    Keys.onEnterPressed: customButton.clicked()

    // Visual feedback state
    background: Rectangle {
        radius: Config.Theme.radiusSmall
        color: customButton.pressed
               ? Config.Theme.colorSurface
               : customButton.hovered
                 ? Config.StaticData.palette.secondary.col600
                 : Config.Theme.colorBox
        border.color: customButton.hovered || customButton.pressed
                      ? Config.Theme.colorTextPrimary
                      : Config.Theme.colorTextSecondary
        border.width: 1

        Behavior on color { ColorAnimation { duration: 100 } }
    }

    contentItem: AppText {
        text: customButton.text
        color: customButton.hovered || customButton.pressed
               ? Config.Theme.colorTextPrimary
               : Config.Theme.colorTextSecondary
        font.pixelSize: Config.Theme.fontSizeBody
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment:   Text.AlignVCenter

        Behavior on color { ColorAnimation { duration: 100 } }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}

