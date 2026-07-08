import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Tisch-Ranking – native Version der pokerth.net-Tischansicht, die der
// Tischnamen-Link bisher im Browser öffnete. Die Web-Kette dahinter:
//   redirect_user_profile.php?tableview=1&nickN=…&table=…
//     → 301 /gametable?u1=…&u10=…  (VueJS, <gametable-component>)
//     → POST /pthranking/gametable/show  Body: { u1:…, …, u10:… }
// Antwort: { status, msg: [{ player_id, username, rank_pos, final_score,
//            average_score, season_games, points_sum }] } – Scores ×100.
// Statt der WebView wird der JSON-Endpunkt direkt abgefragt (Muster wie
// RankingPage/PokerthPlayerPage). Unbekannte Nicks (Gäste/ohne Saisonwertung)
// lässt der Server einfach weg. Aufrufer setzt nicks (Seat-Reihenfolge,
// GameTable.tableStatsNicks()) und tableName.
//
// Über den Quellen-Umschalter oben rechts lässt sich zusätzlich die aktuelle
// BBC-Saison- bzw. WEC-Monatswertung der Tischspieler anzeigen: dafür wird die
// jeweilige Rangliste geladen (eingebettete Initialdaten von
// GET <baseUrl>/results/ranking, ohne CSRF – Muster wie CommunityRankingView)
// und clientseitig auf die Tisch-Nicks gefiltert. rank_pos ist dort die
// Position in der Gesamtrangliste; Spieler ohne Wertung fehlen in der Liste.
Rectangle {
    id: tableStatsPage
    objectName: "gameTableStatsPage"
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: Config.StaticData.palette.secondary.col700

    readonly property bool compact: Config.Responsive.compact
    readonly property string baseUrl: "https://www.pokerth.net"

    property var nicks: []
    property string tableName: ""

    // Aktive Quelle des Umschalters: "pokerth" | "bbc" | "wec". Startwert ist
    // die im Backend vorausgewählte Default-Quelle (bei aktiven Community-
    // Inhalten), sonst PokerTH.
    property string community: (Config.Parameters.showCommunityContent
                                && Config.Community.has(Config.Parameters.defaultCommunity))
                               ? Config.Parameters.defaultCommunity : "pokerth"

    // Einheitliches Zeilenformat für alle Quellen:
    //   { rank_pos, player_id, username, games, mid, score }
    // mid = Avg (PokerTH) bzw. Points (BBC/WEC), Werte fertig formatiert.
    property var rows: []
    property bool loading: false
    property string errorText: ""
    // Läufer-Nummer gegen veraltete Antworten nach schnellem Umschalten.
    property int loadSeq: 0

    // ── Sortierung (clientseitig) ─────────────────────────────────────────────
    // Wenige Zeilen (max. Tischgröße) → keine Pagination nötig, nur Sortieren.
    // Default: nach Platzierung der Gesamtrangliste (beste zuerst).
    property string sortKey: "rank_pos"
    property string sortOrder: "asc"        // "asc" | "desc"
    readonly property bool ascending: sortOrder.indexOf("asc") === 0

    // Numerische Felder (rank_pos/games/mid/score) numerisch, username alphabetisch.
    readonly property var sortedRows: {
        var arr = rows.slice()
        var key = sortKey
        var dir = ascending ? 1 : -1
        arr.sort(function(a, b) {
            var av = a[key], bv = b[key]
            var an = parseFloat(av), bn = parseFloat(bv)
            var numeric = !isNaN(an) && !isNaN(bn)
                          && String(av).trim() !== "" && String(bv).trim() !== ""
            if (numeric)
                return an === bn ? 0 : (an < bn ? -dir : dir)
            var as = String(av === undefined || av === null ? "" : av).toLowerCase()
            var bs = String(bv === undefined || bv === null ? "" : bv).toLowerCase()
            return as === bs ? 0 : (as < bs ? -dir : dir)
        })
        return arr
    }

    // Klick auf einen Spaltenkopf: gleiches Feld → Richtung umkehren, sonst neues
    // Feld (Platzierung/Name aufsteigend, übrige Zahlen absteigend).
    function requestSort(key) {
        if (sortKey === key)
            sortOrder = ascending ? "desc" : "asc"
        else {
            sortKey = key
            sortOrder = (key === "username" || key === "rank_pos") ? "asc" : "desc"
        }
    }

    function score2(v) { return (Number(v) / 100).toFixed(2) }

    // Vue-Prop (HTML-entity-kodiert) aus dem Seiten-HTML lesen – wie
    // CommunityRankingView/CommunityPlayerView.
    function jsonAttr(html, name) {
        var m = html.match(new RegExp(":" + name + "=\"([^\"]*)\""))
        if (!m)
            return null
        var s = m[1].replace(/&quot;/g, "\"").replace(/&#39;/g, "'")
                    .replace(/&lt;/g, "<").replace(/&gt;/g, ">")
                    .replace(/&amp;/g, "&")
        try { return JSON.parse(s) } catch (e) { return null }
    }

    function loadData() {
        loading = true
        errorText = ""
        rows = []
        var seq = ++loadSeq
        if (community === "pokerth")
            loadPokerthData(seq)
        else
            loadCommunityData(seq)
    }

    function loadPokerthData(seq) {
        // Payload exakt wie die Webseite: immer u1…u10, fehlende Plätze leer.
        var payload = {}
        for (var i = 1; i <= 10; ++i)
            payload["u" + i] = (i <= nicks.length && nicks[i - 1]) ? String(nicks[i - 1]) : ""

        var xhr = new XMLHttpRequest()
        xhr.open("POST", baseUrl + "/pthranking/gametable/show")
        xhr.setRequestHeader("Content-Type", "application/json")
        xhr.setRequestHeader("X-Requested-With", "XMLHttpRequest")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || seq !== tableStatsPage.loadSeq)
                return
            tableStatsPage.loading = false
            if (xhr.status !== 200) {
                tableStatsPage.errorText =
                    qsTr("Could not load table ranking (HTTP %1).").arg(xhr.status || 0)
                return
            }
            try {
                var res = JSON.parse(xhr.responseText)
                var list = (res.status && res.msg) ? res.msg : []
                // Der Server liefert in Anfrage-(Seat-)Reihenfolge – für die
                // Ranking-Tabelle nach Platzierung sortieren (beste zuerst).
                list.sort(function(a, b) { return a.rank_pos - b.rank_pos })
                var mapped = []
                for (var i = 0; i < list.length; ++i) {
                    var r = list[i]
                    mapped.push({ rank_pos: r.rank_pos, player_id: r.player_id || 0,
                                  username: r.username,
                                  games: "" + r.season_games,
                                  mid: tableStatsPage.score2(r.average_score),
                                  score: tableStatsPage.score2(r.final_score) })
                }
                tableStatsPage.rows = mapped
            } catch (e) {
                tableStatsPage.errorText = qsTr("Could not parse server response.")
            }
        }
        xhr.send(JSON.stringify(payload))
    }

    function loadCommunityData(seq) {
        var comm = community
        var xhr = new XMLHttpRequest()
        xhr.open("GET", Config.Community.baseUrlFor(comm) + "/results/ranking")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE || seq !== tableStatsPage.loadSeq)
                return
            tableStatsPage.loading = false
            if (xhr.status !== 200) {
                tableStatsPage.errorText =
                    qsTr("Could not load table ranking (HTTP %1).").arg(xhr.status || 0)
                return
            }
            // Eingebettete Initialdaten = aktuelle Saison (BBC) bzw. aktueller
            // Monat (WEC), in Rang-Reihenfolge.
            var all = tableStatsPage.jsonAttr(xhr.responseText,
                                              comm === "bbc" ? "results" : "stats") || []
            var wanted = {}
            for (var i = 0; i < tableStatsPage.nicks.length; ++i) {
                if (tableStatsPage.nicks[i])
                    wanted[String(tableStatsPage.nicks[i]).toLowerCase()] = true
            }
            var mapped = []
            for (var j = 0; j < all.length; ++j) {
                var r = all[j]
                if (!wanted[(r.nickname || "").toLowerCase()])
                    continue
                mapped.push({ rank_pos: j + 1, player_id: 0,
                              username: r.nickname,
                              games: "" + r.games,
                              mid: "" + r.points,
                              score: "" + r.score })
            }
            tableStatsPage.rows = mapped
        }
        xhr.send()
    }

    Component.onCompleted: loadData()

    // ── Aufbau ────────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            AppLabel {
                text: tableStatsPage.tableName !== ""
                      ? qsTr("Table ranking – %1").arg(tableStatsPage.tableName)
                      : qsTr("Table ranking")
                Layout.fillWidth: true
                elide: Text.ElideRight
                color: Config.StaticData.palette.secondary.col200
                font.pointSize: 14
                font.bold: true
            }

            // Quellen-Umschalter oben rechts: lädt die Wertung der Tischspieler
            // aus der gewählten Quelle neu.
            CommunitySwitch {
                id: communitySwitch
                current: tableStatsPage.community
                onSelected: function(community) {
                    tableStatsPage.community = community
                    tableStatsPage.loadData()
                }
            }
        }

        AppLabel {
            text: {
                switch (tableStatsPage.community) {
                case "bbc": return qsTr("Current BBC season standings of the players at this table.")
                case "wec": return qsTr("Current WEC month standings of the players at this table.")
                }
                return qsTr("Current season standings of the players at this table.")
            }
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Config.StaticData.palette.secondary.col300
            font.pixelSize: Config.Theme.fontSizeCaption
        }

        // Kopfzeile der Tabelle – Spalten wie RankingPage (#, Player, Games,
        // Avg, Score), damit beide Ranglisten-Ansichten gleich lesen.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Config.StaticData.palette.secondary.col600
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                RankingHeaderCell {
                    label: qsTr("#")
                    Layout.preferredWidth: tableStatsPage.compact ? 48 : 60
                    sortKey: "rank_pos"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: tableStatsPage.requestSort(key)
                }
                RankingHeaderCell {
                    label: qsTr("Player")
                    Layout.fillWidth: true
                    sortKey: "username"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: tableStatsPage.requestSort(key)
                }
                RankingHeaderCell {
                    label: qsTr("Games")
                    visible: !tableStatsPage.compact
                    Layout.preferredWidth: 70
                    horizontalAlignment: Text.AlignRight
                    sortKey: "games"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: tableStatsPage.requestSort(key)
                }
                RankingHeaderCell {
                    // PokerTH: Saison-Durchschnitt; BBC/WEC: Punkte.
                    label: tableStatsPage.community === "pokerth" ? qsTr("Avg") : qsTr("Points")
                    visible: !tableStatsPage.compact
                    Layout.preferredWidth: 60
                    horizontalAlignment: Text.AlignRight
                    sortKey: "mid"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: tableStatsPage.requestSort(key)
                }
                RankingHeaderCell {
                    label: qsTr("Score")
                    Layout.preferredWidth: tableStatsPage.compact ? 56 : 80
                    horizontalAlignment: Text.AlignRight
                    sortKey: "score"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: tableStatsPage.requestSort(key)
                }
            }
        }

        // Tabelle
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Config.StaticData.palette.secondary.col600
            border.color: Config.StaticData.palette.secondary.col500
            border.width: 1
            radius: 4

            ListView {
                id: statsList
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: tableStatsPage.sortedRows
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: statsList.contentHeight > statsList.height + 4
                            ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff
                }

                delegate: Item {
                    id: statsDelegate
                    required property int index
                    required property var modelData
                    width: ListView.view.width
                    height: 34

                    Rectangle {
                        anchors.fill: parent
                        color: statsDelegate.index % 2 === 0
                               ? Config.StaticData.palette.secondary.col700
                               : Config.StaticData.palette.secondary.col600
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        // Platz für die Scrollbar, wenn sie sichtbar ist.
                        anchors.rightMargin: statsList.contentHeight > statsList.height + 4 ? 16 : 10
                        spacing: 8

                        AppLabel {
                            text: statsDelegate.modelData.rank_pos
                            Layout.preferredWidth: tableStatsPage.compact ? 48 : 60
                            // Top-3 der Gesamtrangliste hervorheben.
                            color: statsDelegate.modelData.rank_pos <= 3
                                   ? Config.Theme.colorAccent
                                   : Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.bold: statsDelegate.modelData.rank_pos <= 3
                        }
                        AppLabel {
                            id: nickLabel
                            text: statsDelegate.modelData.username
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            // Klickbar → Player-Page (per player_id, sonst username).
                            color: nickHover.hovered ? Config.Theme.colorAccent
                                                     : Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.underline: nickHover.hovered

                            HoverHandler { id: nickHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                // Player-Page der aktiven Quelle öffnen (PokerTH
                                // bevorzugt per player_id, BBC/WEC per Nickname).
                                onTapped: {
                                    if (tableStatsPage.community === "pokerth")
                                        tableStatsPage.StackView.view.push("PokerthPlayerPage.qml", {
                                            playerId: statsDelegate.modelData.player_id || 0,
                                            username: statsDelegate.modelData.username || ""
                                        })
                                    else
                                        tableStatsPage.StackView.view.push(
                                            Config.Community.playerPageUrl(tableStatsPage.community),
                                            Config.Community.playerPageProps(tableStatsPage.community,
                                                                             statsDelegate.modelData.username || ""))
                                }
                            }
                        }
                        AppLabel {
                            text: statsDelegate.modelData.games
                            visible: !tableStatsPage.compact
                            Layout.preferredWidth: 70
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeBody
                        }
                        AppLabel {
                            text: statsDelegate.modelData.mid
                            visible: !tableStatsPage.compact
                            Layout.preferredWidth: 60
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col200
                            font.pixelSize: Config.Theme.fontSizeBody
                        }
                        AppLabel {
                            text: statsDelegate.modelData.score
                            Layout.preferredWidth: tableStatsPage.compact ? 56 : 80
                            horizontalAlignment: Text.AlignRight
                            color: Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.bold: true
                        }
                    }
                }
            }

            // Lade-Anzeige
            BusyIndicator {
                anchors.centerIn: parent
                running: tableStatsPage.loading
                visible: running
                implicitWidth: 48
                implicitHeight: 48
            }

            // Leer- / Fehlerhinweis (Gäste und Spieler ohne Saisonwertung lässt
            // der Server weg – bei reinen Gast-Tischen bleibt die Liste leer).
            AppLabel {
                anchors.centerIn: parent
                width: parent.width - 32
                visible: !tableStatsPage.loading
                         && (tableStatsPage.errorText !== "" || tableStatsPage.rows.length === 0)
                text: tableStatsPage.errorText !== ""
                      ? tableStatsPage.errorText
                      : qsTr("No ranking data for the players at this table yet.")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                color: tableStatsPage.errorText !== ""
                       ? "#d05050" : Config.StaticData.palette.secondary.col300
                font.pixelSize: Config.Theme.fontSizeBody
            }
        }
    }
}
