import QtQuick.Controls

import "../config" as Config

// Label (QtQuick.Controls) mit dem App-Font voreingestellt – das Pendant zu
// AppText für Stellen, die bewusst ein Control-Label nutzen. Alle übrigen
// Eigenschaften setzt der Aufrufer wie bei einem gewöhnlichen Label.
Label {
    // User-controlled names and messages are plain text by default. Callers
    // that intentionally render trusted markup can opt in explicitly.
    textFormat: Text.PlainText
    font.family: Config.StaticData.loadedFont.font.family
}
