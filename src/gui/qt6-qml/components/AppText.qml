import QtQuick

import "../config" as Config

// Text mit dem App-Font voreingestellt. Spart das überall wiederholte
// `font.family: Config.StaticData.loadedFont.font.family`. Alle übrigen
// Eigenschaften (color, font.pixelSize, font.bold, …) setzt der Aufrufer wie
// bei einem gewöhnlichen Text.
Text {
    font.family: Config.StaticData.loadedFont.font.family
}
