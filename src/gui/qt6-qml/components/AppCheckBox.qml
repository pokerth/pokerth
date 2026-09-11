import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

// CheckBox whose caption WRAPS instead of being cut off on the right.
// The basis for all options of the settings pages (directly or via
// ConfigCheckBox).
//
// The style default (Universal/CheckBox.qml) creates its contentItem as a
// single-line text: if the caption does not fit into the column width, the
// rest is simply gone. In narrow portrait (Android phone, ~360 dp) that hits
// almost every longer option.
//
// Two things belong together for that – which is why they stand here once
// instead of at every place of use:
//   • wrapMode: Text.Wrap – allows the wrap in the first place.
//   • Layout.fillWidth    – enforces it as well. The implicitWidth of a CheckBox
//     stays the UNwrapped text width; without fillWidth the ColumnLayout
//     creates the box at exactly that width, it sticks out beyond the column and
//     the ScrollView (clip: true) cuts it off – the wrap would never take
//     effect. That was exactly the reason for the cut-off lines.
// A width threshold is not needed: if there is enough room, Text.Wrap does not
// wrap, and the box behaves exactly as before.
CheckBox {
    id: control

    Layout.fillWidth: true

    contentItem: Text {
        text: control.text
        wrapMode: Text.Wrap
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
        // Pass the font through: the style default sets font: control.font. Without
        // that, this text falls back to the default font size (measured:
        // 12 instead of 13) and the wrapping labels appear larger than all the
        // others.
        font: control.font
        // Universal.foreground instead of palette.windowText: the palette follows the
        // SYSTEM palette and not the Universal.theme that pokerth.qml derives from the
        // DarkMode setting. On iOS/iPadOS windowText is black →
        // black text on a dark background. Universal.foreground is exactly
        // what the style default uses – wrapping and single-line labels
        // thus look identical.
        color: control.Universal.foreground
    }
}
