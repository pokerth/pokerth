import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// One completed season result of the PokerTH player page.
// The season list from /pthranking/player/show is global (all seasons ever
// rated), not player specific – whether the player played in a season at all
// is only revealed by
//   GET /pthranking/player/season/get/<playerId>/<season>
// via `status`. That is why every card loads by itself and hides completely
// on `status: false` (exactly as the website renders a separate component per
// season). Expanded, it shows key figures and the placement charts of the
// season.
ColumnLayout {
    id: card

    property int playerId: 0
    property string season: ""
    // Preformatted label ("2026 Q2") – the page knows the conversion.
    property string title: ""
    property string baseUrl: "https://www.pokerth.net"

    property var ranking: null
    property int pos: 0
    property var barStats: []
    property var stats: []
    property bool expanded: false
    property bool requested: false

    readonly property bool compact: Config.Responsive.compact

    // Tells the page that this season has a result – from that it counts
    // whether the season block gets a heading at all.
    signal resultAvailable()

    function score2(v) { return (Number(v) / 100).toFixed(2) }

    // Load once as soon as the player ID and the season are known – the page can
    // also be opened with just a nickname, then the ID only arrives later.
    function loadOnce() {
        if (requested || playerId <= 0 || season === "")
            return
        requested = true
        var xhr = new XMLHttpRequest()
        xhr.open("GET", baseUrl + "/pthranking/player/season/get/"
                        + playerId + "/" + season)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || xhr.status !== 200)
                return
            try {
                var res = JSON.parse(xhr.responseText)
                if (!res.status)
                    return
                card.ranking = (res.player && res.player.ranking) ? res.player.ranking : null
                card.pos = res.pos || 0
                card.barStats = res.bar_stats || []
                card.stats = res.stats || []
                if (card.ranking)
                    card.resultAvailable()
            } catch (e) {
                // A single season without a result we let disappear silently –
                // an error text per card would only be noise here.
            }
        }
        xhr.send()
    }

    Component.onCompleted: loadOnce()
    onPlayerIdChanged: loadOnce()

    Layout.fillWidth: true
    visible: ranking !== null
    spacing: 8

    // ── Header: season + short result, expands the details ──────────────────
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        radius: 6
        color: Config.StaticData.palette.secondary.col600
        border.color: Config.StaticData.palette.secondary.col500
        border.width: 1

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: card.expanded = !card.expanded
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10

            AppLabel {
                text: card.title
                color: Config.StaticData.palette.secondary.col100
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            AppLabel {
                text: card.pos > 0 ? ("#" + card.pos) : ""
                color: Config.Theme.colorAccent
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }
            AppLabel {
                // In compact mode the rank is enough – expanded, the score
                // is in the tile anyway.
                visible: !card.compact && card.ranking
                text: card.ranking ? card.score2(card.ranking.final_score) : ""
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeBody
            }
            SvgIcon {
                id: expanderCaret
                source: "qrc:/resources/caretLeft.svg"
                rotation: card.expanded ? -90 : 180
                Behavior on rotation { NumberAnimation { duration: 150 } }
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                Layout.alignment: Qt.AlignVCenter
                // Colourising via layer.effect instead of a MultiEffect child: VectorImage
                // (Qt >= 6.8) is not a texture provider and must not be referenced via
                // source (it would render black/torn).
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: Config.Theme.colorTextMuted
                }
            }
        }
    }

    // ── Details ────────────────────────────────────────────────────────────
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.bottomMargin: 6
        visible: card.expanded
        spacing: 10

        GridLayout {
            Layout.fillWidth: true
            columns: card.compact ? 2 : 4
            columnSpacing: 8
            rowSpacing: 8

            Repeater {
                model: {
                    var r = card.ranking
                    return [
                        { label: qsTr("Score"),  value: r ? card.score2(r.final_score) : "–" },
                        { label: qsTr("Avg"),    value: r ? card.score2(r.average_score) : "–" },
                        { label: qsTr("Games"),  value: r ? ("" + r.season_games) : "–" },
                        { label: qsTr("Points"), value: r ? ("" + r.points_sum) : "–" }
                    ]
                }
                StatTile {
                    required property var modelData
                    label: modelData.label
                    value: modelData.value
                }
            }
        }

        SeasonStatsSection {
            Layout.fillWidth: true
            showTitle: false
            counts: card.barStats
            stats: card.stats
        }
    }
}
