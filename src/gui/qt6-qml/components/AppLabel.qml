import QtQuick
import QtQuick.Controls

import "../config" as Config

// Label (QtQuick.Controls) with the app font preset – the counterpart to
// AppText for places that deliberately use a control label. All remaining
// properties are set by the caller as on an ordinary Label.
Label {
    // User-controlled names and messages are plain text by default. Callers
    // that intentionally render trusted markup can opt in explicitly.
    textFormat: Text.PlainText
    font.family: Config.StaticData.loadedFont.font.family
}
