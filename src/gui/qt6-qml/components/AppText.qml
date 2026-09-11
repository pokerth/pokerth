import QtQuick

import "../config" as Config

// Text with the app font preset. Saves repeating
// `font.family: Config.StaticData.loadedFont.font.family` everywhere. All
// remaining properties (color, font.pixelSize, font.bold, …) are set by the
// caller as on an ordinary Text.
Text {
    // User-controlled names and messages are plain text by default. Callers
    // that intentionally render trusted markup can opt in explicitly.
    textFormat: Text.PlainText
    font.family: Config.StaticData.loadedFont.font.family
}
