import QtQuick
import QtQuick.Controls

import "../config" as Config

// PokerTH base button — built on AbstractButton for keyboard + accessibility support.
// Touch target is at least Theme.touchTarget (48dp on mobile, 44dp on desktop).
AbstractButton {
    id: customButton

    implicitWidth:  Config.Theme.buttonWidth < 0 ? 160 : Config.Theme.buttonWidth
    implicitHeight: Config.Theme.touchTarget

    // Keyboard operation as in web forms and Win32 dialogs: space already
    // triggers AbstractButton itself, Return/Enter is added here. The event is
    // thereby consumed, so that a focused button takes precedence over the
    // default button of the surrounding form.
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

        // Draw the focus frame ourselves instead of using the universal style
        // (useSystemFocusVisuals): its FocusRectangle hangs off the
        // ApplicationWindow and only appears on pages – in popups the keyboard
        // focus stayed invisible (measured in the real client: Tab in the
        // "leave game" prompt changed the button without any indication).
        // visualFocus instead of activeFocus: only on keyboard focus, not after
        // a mouse click – like :focus-visible on the web.
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

