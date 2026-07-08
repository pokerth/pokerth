import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// PokerTH-Spielerprofil – https://www.pokerth.net/player?p=<id> bzw. ?u=<name>
// Daten nativ über  GET /pthranking/player/show?player_id=<id>|username=<name>
// (kein CSRF). Aufrufer setzt playerId (aus der Ranking-Zeile) oder username.
Rectangle {
    id: playerPage
    objectName: "pokerthPlayerPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property string baseUrl: "https://www.pokerth.net"

    property int playerId: 0
    property string username: ""

    property var player: null
    property int pos: 0
    property var last5: []
    property var games: []
    // Season-Stats-Rohdaten der API: barStats = Häufigkeit je Platz 1–10
    // (bar_stats), placePercents = zugehörige Prozent-Strings (stats[1]).
    property var barStats: []
    property var placePercents: []
    // Alle vom Spieler gespielten Saisons, neueste zuerst (API-Feld `seasons`,
    // Format "<Jahr>_<Quartal>", z. B. "2026_2").
    property var seasons: []
    property bool loading: false
    property string errorText: ""

    function score2(v) { return (Number(v) / 100).toFixed(2) }
    function datePart(s) { return s ? String(s).substring(0, 10) : "" }
    // "2026_2" → "2026 Q2"; unbekanntes Format unverändert durchreichen.
    function seasonLabel(s) {
        var parts = String(s).split("_")
        return parts.length === 2 ? (parts[0] + " Q" + parts[1]) : String(s)
    }

    function load() {
        loading = true
        errorText = ""
        var q = playerId > 0 ? ("player_id=" + playerId)
                             : ("username=" + encodeURIComponent(username))
        var xhr = new XMLHttpRequest()
        xhr.open("GET", baseUrl + "/pthranking/player/show?" + q)
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            playerPage.loading = false
            if (xhr.status !== 200) {
                playerPage.errorText = qsTr("Could not load player (HTTP %1).").arg(xhr.status || 0)
                return
            }
            try {
                var res = JSON.parse(xhr.responseText)
                if (!res.status) {
                    playerPage.errorText = qsTr("Player not found.")
                    return
                }
                playerPage.player = res.player
                playerPage.pos = res.pos || 0
                playerPage.last5 = res.last5 || []
                playerPage.games = res.games || []
                playerPage.barStats = res.bar_stats || []
                // stats[1] ist ein Objekt {"1":"8.3%",…}; in ein nach Platz
                // 1–10 geordnetes Array umformen, das die Section direkt nutzt.
                var pct = (res.stats && res.stats.length > 1) ? res.stats[1] : null
                var arr = []
                for (var p = 1; p <= 10; ++p)
                    arr.push(pct && pct[p] !== undefined ? pct[p] : "")
                playerPage.placePercents = arr
                playerPage.seasons = res.seasons || []
            } catch (e) {
                playerPage.errorText = qsTr("Could not parse server response.")
            }
        }
        xhr.send()
    }

    Component.onCompleted: load()

    // ── Inhalt ──────────────────────────────────────────────────────────────
    Flickable {
        id: contentFlick
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        anchors.leftMargin: 16
        // Scrollbar näher an den Fensterrand rücken, statt rechts Platz zu
        // verschwenden – der gewonnene Raum dient als Abstand zum Inhalt.
        anchors.rightMargin: 6
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        // Inhalt schmaler halten, solange die Scrollbar sichtbar ist, damit sie
        // den Text (v. a. die Datums-Spalte) rechts nicht überlappt.
        readonly property bool scrolling: contentHeight > height
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: content
            width: contentFlick.width - (contentFlick.scrolling ? 16 : 0)
            spacing: 14

            // ── Kopf: Avatar + Name + Land + Eckdaten ───────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Rectangle {
                    Layout.preferredWidth: 72
                    Layout.preferredHeight: 72
                    radius: 6
                    color: Config.StaticData.palette.secondary.col600
                    clip: true

                    Image {
                        anchors.fill: parent
                        visible: source != ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        source: (playerPage.player && playerPage.player.avatar_hash)
                                ? playerPage.baseUrl + "/images/avatars/game/"
                                  + playerPage.player.avatar_hash + "." + playerPage.player.avatar_mime
                                : ""
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Image {
                            readonly property string code:
                                (playerPage.player && playerPage.player.country_iso
                                 ? String(playerPage.player.country_iso) : "").toLowerCase()
                            visible: code !== ""
                            source: code !== "" ? "qrc:/resources/cflags/" + code + ".svg" : ""
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 15
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                        AppLabel {
                            text: playerPage.player ? playerPage.player.username : ""
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: Config.StaticData.palette.secondary.col100
                            font.pointSize: 16
                            font.bold: true
                        }
                    }
                    AppLabel {
                        visible: playerPage.player && playerPage.player.created
                        text: qsTr("Member since %1").arg(playerPage.datePart(playerPage.player ? playerPage.player.created : ""))
                        color: Config.StaticData.palette.secondary.col300
                        font.pixelSize: Config.Theme.fontSizeCaption
                    }
                    AppLabel {
                        visible: playerPage.player && playerPage.player.last_login
                        text: qsTr("Last login %1").arg(playerPage.datePart(playerPage.player ? playerPage.player.last_login : ""))
                        color: Config.StaticData.palette.secondary.col300
                        font.pixelSize: Config.Theme.fontSizeCaption
                    }
                }

                // Quellen-Umschalter oben rechts: ersetzt diese Seite durch die
                // Player-Page der gewählten Quelle (gleicher Nickname). BBC/WEC
                // kennen nur Nicknames – solange keiner bekannt ist (Seite wurde
                // nur mit playerId geöffnet und lädt noch), passiert nichts.
                CommunitySwitch {
                    id: communitySwitch
                    Layout.alignment: Qt.AlignTop
                    current: "pokerth"
                    onSelected: function(community) {
                        var nick = (playerPage.player && playerPage.player.username)
                                   ? playerPage.player.username : playerPage.username
                        if (nick === "")
                            return
                        playerPage.StackView.view.replace(
                            Config.Community.playerPageUrl(community),
                            Config.Community.playerPageProps(community, nick))
                    }
                }
            }

            // ── Aktuelle-Saison-Kennzahlen ──────────────────────────────────
            AppLabel {
                text: qsTr("Current season")
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }

            GridLayout {
                Layout.fillWidth: true
                columns: playerPage.compact ? 2 : 5
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: {
                        var r = (playerPage.player && playerPage.player.ranking) ? playerPage.player.ranking : null
                        return [
                            { label: qsTr("Rank"),   value: playerPage.pos > 0 ? ("#" + playerPage.pos) : "–" },
                            { label: qsTr("Score"),  value: r ? playerPage.score2(r.final_score) : "–" },
                            { label: qsTr("Avg"),    value: r ? playerPage.score2(r.average_score) : "–" },
                            { label: qsTr("Games"),  value: r ? ("" + r.season_games) : "–" },
                            { label: qsTr("Points"), value: r ? ("" + r.points_sum) : "–" }
                        ]
                    }
                    Rectangle {
                        id: statCell
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        radius: 6
                        color: Config.StaticData.palette.secondary.col600
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 2
                            AppLabel {
                                Layout.alignment: Qt.AlignHCenter
                                text: statCell.modelData.value
                                color: Config.StaticData.palette.secondary.col100
                                font.pixelSize: Config.Theme.fontSizeTitle
                                font.bold: true
                            }
                            AppLabel {
                                Layout.alignment: Qt.AlignHCenter
                                text: statCell.modelData.label
                                color: Config.StaticData.palette.secondary.col300
                                font.pixelSize: Config.Theme.fontSizeCaption
                            }
                        }
                    }
                }
            }

            // ── Letzte 5 Platzierungen ──────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                visible: playerPage.last5.length > 0
                spacing: 8
                AppLabel {
                    text: qsTr("Last 5:")
                    color: Config.StaticData.palette.secondary.col200
                    font.pixelSize: Config.Theme.fontSizeBody
                }
                Repeater {
                    model: playerPage.last5
                    Rectangle {
                        required property var modelData
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        radius: 14
                        color: modelData === 1 ? Config.Theme.colorAccent
                                               : Config.StaticData.palette.secondary.col600
                        AppLabel {
                            anchors.centerIn: parent
                            text: modelData
                            color: modelData === 1 ? "#101010" : Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.bold: true
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }

            // ── Season Stats (Charts wie pokerth.net) ───────────────────────
            SeasonStatsSection {
                Layout.fillWidth: true
                counts: playerPage.barStats
                percents: playerPage.placePercents
            }

            // ── Letzte Spiele ───────────────────────────────────────────────
            AppLabel {
                text: qsTr("Recent games")
                visible: playerPage.games.length > 0
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                visible: playerPage.games.length > 0
                // Auf wenige Zeilen begrenzt; längere Historie scrollt intern
                // (eigene vertikale Scrollbar) statt die ganze Seite zu strecken.
                readonly property int rowH: 30
                readonly property int maxRows: 6
                Layout.preferredHeight:
                    Math.min(playerPage.games.length, maxRows) * rowH + 2
                color: Config.StaticData.palette.secondary.col600
                border.color: Config.StaticData.palette.secondary.col500
                border.width: 1
                radius: 4
                clip: true

                ListView {
                    id: gamesList
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: playerPage.games
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Item {
                        id: gameRow
                        required property int index
                        required property var modelData
                        width: gamesList.width
                        height: 30

                        Rectangle {
                            anchors.fill: parent
                            color: gameRow.index % 2 === 0
                                   ? Config.StaticData.palette.secondary.col700
                                   : Config.StaticData.palette.secondary.col600
                        }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8
                            AppLabel {
                                text: qsTr("#%1").arg(gameRow.modelData.place)
                                Layout.preferredWidth: 40
                                color: gameRow.modelData.place === 1
                                       ? Config.Theme.colorAccent
                                       : Config.StaticData.palette.secondary.col100
                                font.pixelSize: Config.Theme.fontSizeBody
                                font.bold: gameRow.modelData.place === 1
                            }
                            AppLabel {
                                text: (gameRow.modelData.game && gameRow.modelData.game.name) ? gameRow.modelData.game.name : ""
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                color: Config.StaticData.palette.secondary.col100
                                font.pixelSize: Config.Theme.fontSizeBody
                            }
                            AppLabel {
                                text: playerPage.datePart(gameRow.modelData.start_time)
                                visible: !playerPage.compact
                                color: Config.StaticData.palette.secondary.col300
                                font.pixelSize: Config.Theme.fontSizeCaption
                            }
                        }
                    }
                }
            }

            // ── Alle gespielten Saisons ─────────────────────────────────────
            AppLabel {
                text: qsTr("Seasons")
                visible: playerPage.seasons.length > 0
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }

            Flow {
                Layout.fillWidth: true
                visible: playerPage.seasons.length > 0
                spacing: 8
                Repeater {
                    model: playerPage.seasons
                    Rectangle {
                        required property var modelData
                        implicitWidth: seasonText.implicitWidth + 20
                        implicitHeight: 26
                        radius: 13
                        color: Config.StaticData.palette.secondary.col600
                        border.color: Config.StaticData.palette.secondary.col500
                        border.width: 1
                        AppLabel {
                            id: seasonText
                            anchors.centerIn: parent
                            text: playerPage.seasonLabel(parent.modelData)
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                    }
                }
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: playerPage.loading
        visible: running
        implicitWidth: 48
        implicitHeight: 48
    }

    AppLabel {
        anchors.centerIn: parent
        width: parent.width - 32
        visible: !playerPage.loading && playerPage.errorText !== ""
        text: playerPage.errorText
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        color: "#d05050"
        font.pixelSize: Config.Theme.fontSizeBody
    }
}
