import QtQuick
import QtQuick.Controls

import "../config" as Config

// PokerTH base button — built on AbstractButton for keyboard + accessibility support.
// Touch target is at least Theme.touchTarget (48dp on mobile, 44dp on desktop).
AbstractButton {
    id: customButton

    implicitWidth:  Config.Theme.buttonWidth < 0 ? 160 : Config.Theme.buttonWidth
    implicitHeight: Config.Theme.touchTarget

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

        // Fokusrahmen selbst zeichnen statt über den Universal-Stil
        // (useSystemFocusVisuals): Dessen FocusRectangle hängt an der
        // ApplicationWindow und erscheint nur auf Seiten – in Popups blieb der
        // Tastaturfokus unsichtbar (im echten Client nachgemessen: Tab in der
        // "Spiel verlassen"-Abfrage wechselte den Button ohne jede Anzeige).
        // visualFocus statt activeFocus: nur bei Tastaturfokus, nicht nach
        // einem Mausklick – wie :focus-visible im Web.
        border.color: customButton.visualFocus
                      ? Config.Theme.colorAccent
                      : (customButton.hovered || customButton.pressed
                         ? Config.Theme.colorTextPrimary
                         : Config.Theme.colorTextSecondary)
        border.width: customButton.visualFocus ? 2 : 1

        Behavior on color { ColorAnimation { duration: 100 } }
        Behavior on border.color { ColorAnimation { duration: 100 } }
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

