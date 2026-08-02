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
    // (bar_stats), stats = rohes stats-Feld (die Section leitet die Prozente ab).
    property var barStats: []
    property var stats: []
    // Achtung: `seasons` der API ist die *globale* Liste aller abgeschlossenen
    // Saisons (Format "<Jahr>_<Quartal>", z. B. "2026_2") – für jeden Spieler
    // identisch, nicht dessen Teilnahmen. Welche davon der Spieler gespielt hat,
    // ermittelt erst die PlayerSeasonCard je Saison; Karten ohne Ergebnis
    // blenden sich selbst aus.
    property var seasons: []
    // Zahl der Saisons, für die tatsächlich ein Ergebnis vorliegt (von den
    // Karten hochgezählt); steuert die Überschrift des Saison-Blocks.
    property int seasonResults: 0
    property bool loading: false
    property string errorText: ""

    function score2(v) { return (Number(v) / 100).toFixed(2) }
    function datePart(s) { return s ? String(s).substring(0, 10) : "" }
    // "2026_2" → "2026 Q2"; unbekanntes Format unverändert durchreichen.
    function seasonLabel(s) {
        var parts = String(s).split("_")
        return parts.length === 2 ? (parts[0] + " Q" + parts[1]) : String(s)
    }

    // Quellen-Umschalter: ersetzt diese Seite durch die Player-Page der
    // gewählten Quelle (gleicher Nickname). BBC/WEC kennen nur Nicknames –
    // solange keiner bekannt ist (Seite nur mit playerId geöffnet, lädt noch),
    // passiert nichts. Als Funktion, weil der Umschalter je nach Layout an zwei
    // Stellen (inline / eigene Zeile) sitzt und beide dieselbe Logik brauchen.
    function switchCommunity(community) {
        var nick = (player && player.username) ? player.username : username
        if (nick === "")
            return
        StackView.view.replace(Config.Community.playerPageUrl(community),
                               Config.Community.playerPageProps(community, nick))
    }

    function load() {
        loading = true
        errorText = ""
        // Saison-Karten werden über `seasons` neu erzeugt und zählen frisch –
        // den Ergebnis-Zähler daher hier zurücksetzen, nicht kumulieren.
        seasonResults = 0
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
                playerPage.stats = res.stats || []
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
            // Im Kompakt-/Portrait-Modus (schmales Phone) wandert der Quellen-
            // Umschalter in eine eigene Zeile darunter (rechtsbündig), damit der
            // Kopf – Avatar + Name + 3-Segment-Umschalter – nicht breiter wird als
            // das Display. Auf Desktop/Tablet bleibt er oben rechts inline.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    Rectangle {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 72
                        Layout.alignment: Qt.AlignTop
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
                        // Datums-Zeilen füllen die Breite und eliden – so bestimmen
                        // sie im schmalen Portrait nicht die Kopf-Mindestbreite.
                        AppLabel {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            visible: playerPage.player && playerPage.player.created
                            text: qsTr("Member since %1").arg(playerPage.datePart(playerPage.player ? playerPage.player.created : ""))
                            color: Config.StaticData.palette.secondary.col300
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                        AppLabel {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            visible: playerPage.player && playerPage.player.last_login
                            text: qsTr("Last login %1").arg(playerPage.datePart(playerPage.player ? playerPage.player.last_login : ""))
                            color: Config.StaticData.palette.secondary.col300
                            font.pixelSize: Config.Theme.fontSizeCaption
                        }
                    }

                    // Desktop/Tablet: Umschalter inline oben rechts.
                    CommunitySwitch {
                        visible: !playerPage.compact
                        Layout.alignment: Qt.AlignTop
                        current: "pokerth"
                        onSelected: playerPage.switchCommunity(community)
                    }
                }

                // Kompakt/Portrait: Umschalter in eigener Zeile, rechtsbündig.
                RowLayout {
                    Layout.fillWidth: true
                    visible: playerPage.compact
                    Item { Layout.fillWidth: true }
                    CommunitySwitch {
                        current: "pokerth"
                        onSelected: playerPage.switchCommunity(community)
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
                    StatTile {
                        required property var modelData
                        label: modelData.label
                        value: modelData.value
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
                stats: playerPage.stats
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
                                   ? Config.Theme.colorBox
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

            // ── Gespielte Saisons ───────────────────────────────────────────
            // Die Überschrift hängt an den tatsächlich gefundenen Ergebnissen,
            // nicht an der (globalen) Saison-Liste – sonst stünde sie auch bei
            // Spielern da, die noch keine Saison abgeschlossen haben.
            AppLabel {
                text: qsTr("Seasons")
                visible: playerPage.seasonResults > 0
                color: Config.StaticData.palette.secondary.col200
                font.pixelSize: Config.Theme.fontSizeBody
                font.bold: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Repeater {
                    model: playerPage.seasons
                    PlayerSeasonCard {
                        required property var modelData
                        baseUrl: playerPage.baseUrl
                        playerId: playerPage.player ? playerPage.player.player_id : 0
                        season: modelData
                        title: playerPage.seasonLabel(modelData)
                        onResultAvailable: ++playerPage.seasonResults
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
