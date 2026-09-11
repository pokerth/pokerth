import QtQuick

import "../config" as Config

// Clickable, sortable column header for the ranking tables
// (RankingPage / CommunityRankingView). Shows `label` and – if this column is
// the active sort – an up/down arrow. A tap reports the `sortKey`; the caller
// decides whether to sort by that field or merely to reverse the direction.
// Non-sortable columns leave `sortKey` empty (no hover/tap/arrow), so that the
// cell behaves like an ordinary header label.
AppLabel {
    id: cell

    property string label: ""
    property string sortKey: ""
    property string activeKey: ""
    // Accepts "asc"/"desc" as well as the server spelling "ascending"/"descending".
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
