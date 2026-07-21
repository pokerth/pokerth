import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

import "../config" as Config

// Ein abgeschlossenes Saison-Ergebnis der PokerTH-Spielerseite.
// Die Saison-Liste aus /pthranking/player/show ist global (alle je gewerteten
// Saisons), nicht spielerbezogen – ob der Spieler in einer Saison überhaupt
// gespielt hat, verrät erst
//   GET /pthranking/player/season/get/<playerId>/<season>
// über `status`. Deshalb lädt jede Karte selbst und blendet sich bei
// `status: false` komplett aus (genau wie die Website je Saison eine eigene
// Komponente rendert). Aufgeklappt zeigt sie Kennzahlen und die
// Platzierungs-Charts der Saison.
ColumnLayout {
    id: card

    property int playerId: 0
    property string season: ""
    // Vorformatiertes Label ("2026 Q2") – die Umwandlung kennt die Seite.
    property string title: ""
    property string baseUrl: "https://www.pokerth.net"

    property var ranking: null
    property int pos: 0
    property var barStats: []
    property var stats: []
    property bool expanded: false
    property bool requested: false

    readonly property bool compact: Config.Responsive.compact

    // Meldet der Seite, dass diese Saison ein Ergebnis hat – sie zählt daraus
    // ab, ob der Saison-Block überhaupt eine Überschrift bekommt.
    signal resultAvailable()

    function score2(v) { return (Number(v) / 100).toFixed(2) }

    // Einmalig laden, sobald Spieler-ID und Saison feststehen – die Seite kann
    // auch nur mit Nickname geöffnet werden, dann trifft die ID erst später ein.
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
                // Einzelne Saison ohne Ergebnis lassen wir still verschwinden –
                // ein Fehlertext je Karte wäre hier nur Rauschen.
            }
        }
        xhr.send()
    }

    Component.onCompleted: loadOnce()
    onPlayerIdChanged: loadOnce()

    Layout.fillWidth: true
    visible: ranking !== null
    spacing: 8

    // ── Kopfzeile: Saison + Kurzergebnis, klappt die Details auf ────────────
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
                // Im Kompakt-Modus reicht der Rang – der Score steht
                // aufgeklappt ohnehin in der Kachel.
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

                MultiEffect {
                    source: expanderCaret
                    anchors.fill: expanderCaret
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
