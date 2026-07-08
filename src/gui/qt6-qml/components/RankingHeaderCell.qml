import QtQuick

import "../config" as Config

// Klickbarer, sortierbarer Spaltenkopf für die Ranglisten-Tabellen
// (RankingPage / CommunityRankingView). Zeigt `label` und – wenn diese Spalte
// aktiv sortiert ist – einen Auf-/Ab-Pfeil. Ein Tap meldet den `sortKey`; der
// Aufrufer entscheidet, ob nach diesem Feld sortiert oder nur die Richtung
// umgekehrt wird. Nicht sortierbare Spalten lassen `sortKey` leer (kein
// Hover/Tap/Pfeil), sodass sich die Zelle wie ein normales Kopf-Label verhält.
AppLabel {
    id: cell

    property string label: ""
    property string sortKey: ""
    property string activeKey: ""
    // Erlaubt "asc"/"desc" ebenso wie die Server-Schreibweise "ascending"/"descending".
    property string sortOrder: "desc"

    readonly property bool sortable: sortKey !== ""
    readonly property bool active: sortable && sortKey === activeKey
    readonly property bool ascending: sortOrder.indexOf("asc") === 0

    signal sortRequested(string key)

    text: label + (active ? (ascending ? "  ▲" : "  ▼") : "")
    color: (active || hover.hovered)
           ? Config.Theme.colorAccent
           : Config.StaticData.palette.secondary.col200
    font.pixelSize: Config.Theme.fontSizeCaption
    font.bold: true

    HoverHandler {
        id: hover
        enabled: cell.sortable
        cursorShape: Qt.PointingHandCursor
    }
    TapHandler {
        enabled: cell.sortable
        onTapped: cell.sortRequested(cell.sortKey)
    }
}
