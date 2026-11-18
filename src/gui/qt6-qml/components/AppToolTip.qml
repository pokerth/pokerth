import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

// ToolTip whose text is always plain text - the counterpart to AppText for
// tooltips. The ToolTip of every Controls style shows its text in a Text with
// the default Text.AutoText, which renders anything that looks like markup as
// styled text, including <img src="http://..."> that is then fetched from the
// network. Use this one (instead of the attached ToolTip.text) whenever the text
// contains player names, game names or other text chosen by other people.
//
// The content item has to be replaced: switching the format afterwards is too
// late, the image is requested as soon as the text is first laid out. It is the
// one of the Universal style (the app's style, see qtquickcontrols2.conf and
// main()), with only the text format added.
ToolTip {
    id: control

    contentItem: Text {
        text: control.text
        textFormat: Text.PlainText
        font: control.font
        wrapMode: Text.Wrap
        opacity: enabled ? 1.0 : 0.2
        color: control.Universal.foreground
    }
}
