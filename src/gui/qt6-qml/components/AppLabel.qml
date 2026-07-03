import QtQuick.Controls

import "../config" as Config

// Label (QtQuick.Controls) mit dem App-Font voreingestellt – das Pendant zu
// AppText für Stellen, die bewusst ein Control-Label nutzen. Alle übrigen
// Eigenschaften setzt der Aufrufer wie bei einem gewöhnlichen Label.
Label {
    font.family: Config.StaticData.loadedFont.font.family
}
