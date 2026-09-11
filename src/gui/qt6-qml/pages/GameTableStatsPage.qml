import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts

import "../config" as Config
import "../components"

// Table ranking – the native version of the pokerth.net table view that the
// table name link used to open in the browser. The web chain behind it:
//   redirect_user_profile.php?tableview=1&nickN=…&table=…
//     → 301 /gametable?u1=…&u10=…  (VueJS, <gametable-component>)
//     → POST /pthranking/gametable/show  Body: { u1:…, …, u10:… }
// Reply: { status, msg: [{ player_id, username, rank_pos, final_score,
//            average_score, season_games, points_sum }] } – scores ×100.
// Instead of the WebView the JSON endpoint is queried directly (the pattern of
// RankingPage/PokerthPlayerPage). Unknown nicks (guests/without a season rating)
// are simply left out by the server. The caller sets nicks (seat order,
// GameTable.tableStatsNicks()) and tableName.
//
// Via the source switch at the top right, the current BBC season or WEC
// monthly rating of the table players can be shown in addition: for that the
// respective ranking is loaded (embedded initial data of
// GET <baseUrl>/results/ranking, without CSRF – the pattern of CommunityRankingView)
// and filtered on the table nicks on the client side. rank_pos is the
// position in the overall ranking there; players without a rating are missing from the list.
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
    // Settings of the running table (Lobby.currentGameInfo()) – passed by the
    // caller as a snapshot, so that the page stays consistent even after
    // leaving the game. Empty = the card is hidden.
    property var gameInfo: ({})

    // Active source of the switch: "pokerth" | "bbc" | "wec". The initial value is
    // the default source preselected in the backend (with community content
    // active), otherwise PokerTH.
    property string community: (Config.Parameters.showCommunityContent
                                && Config.Community.has(Config.Parameters.defaultCommunity))
                               ? Config.Parameters.defaultCommunity : "pokerth"

    // A uniform row format for all sources:
    //   { rank_pos, player_id, username, games, mid, score }
    // mid = avg (PokerTH) or points (BBC/WEC), values formatted ready for display.
    property var rows: []
    property bool loading: false
    property string errorText: ""
    // A run number against stale replies after switching quickly.
    property int loadSeq: 0

    // ── Sorting (client side) ─────────────────────────────────────────────────
    // Few rows (at most the table size) → no pagination needed, only sorting.
    // Default: by the position in the overall ranking (best first).
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

    // Click on a column header: the same field → reverse the direction, otherwise a new
    // field (position/name ascending, the other numbers descending).
    function requestSort(key) {
        if (sortKey === key)
            sortOrder = ascending ? "desc" : "asc"
        else {
            sortKey = key
            sortOrder = (key === "username" || key === "rank_pos") ? "asc" : "desc"
        }
    }

    // maxPlayers is set in every real game info – if it is missing (local game,
    // call without gameInfo), the table info card stays hidden.
    readonly property bool hasGameInfo:
        !!gameInfo && (gameInfo.maxPlayers || 0) > 0

    // A uniform row of the table info card.
    component InfoItem: AppLabel {
        Layout.fillWidth: true
        elide: Text.ElideRight
        color: Config.StaticData.palette.secondary.col200
        font.pixelSize: Config.Theme.fontSizeBody
    }

    function score2(v) { return (Number(v) / 100).toFixed(2) }

    // Read a Vue prop (HTML entity encoded) from the page HTML – as in
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
        // The payload exactly like the website: always u1…u10, missing places empty.
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
                // The server delivers in request (seat) order – sort it by position
                // for the ranking table (best first).
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
            // The embedded initial data = the current season (BBC) or the current
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

            // Source switch at the top right: reloads the rating of the table players
            // from the selected source.
            CommunitySwitch {
                id: communitySwitch
                current: tableStatsPage.community
                onSelected: function(community) {
                    tableStatsPage.community = community
                    tableStatsPage.loadData()
                }
            }
        }

        // ── Table info (settings of the running game) ─────────────────────────
        // The same data as the game details in the waiting room (GameWaitPage) –
        // at the table itself there is no other place where the settings
        // (blinds, starting money, timeouts …) could be looked up.
        Rectangle {
            Layout.fillWidth: true
            visible: tableStatsPage.hasGameInfo
            implicitHeight: infoGrid.implicitHeight + 20
            color: Config.StaticData.palette.secondary.col600
            border.color: Config.StaticData.palette.secondary.col500
            border.width: 1
            radius: 4

            GridLayout {
                id: infoGrid
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                // Narrow windows: one column, otherwise two as in the waiting room.
                columns: tableStatsPage.compact ? 1 : 2
                rowSpacing: 5
                columnSpacing: 14

                InfoItem {
                    text: qsTr("Type: %1").arg(
                              (typeof Lobby !== "undefined" && Lobby)
                              ? Lobby.gameTypeText(tableStatsPage.gameInfo.gameType || 1) : "")
                }
                InfoItem {
                    text: qsTr("Players: %1 / %2")
                          .arg(tableStatsPage.gameInfo.playerCount || 0)
                          .arg(tableStatsPage.gameInfo.maxPlayers || 0)
                }
                InfoItem {
                    text: qsTr("Small blind: %1").arg(tableStatsPage.gameInfo.firstSmallBlind || 0)
                }
                InfoItem {
                    text: qsTr("Start cash: %1").arg(tableStatsPage.gameInfo.startMoney || 0)
                }
                InfoItem {
                    text: (tableStatsPage.gameInfo.raiseIntervalMode || 1) === 1
                          ? qsTr("Blinds raise interval: %1 hands")
                            .arg(tableStatsPage.gameInfo.raiseEveryHands || 0)
                          : qsTr("Blinds raise interval: %1 minutes")
                            .arg(tableStatsPage.gameInfo.raiseEveryMinutes || 0)
                }
                InfoItem {
                    text: qsTr("Blinds raise mode: %1")
                          .arg((tableStatsPage.gameInfo.raiseMode || 1) === 1
                               ? qsTr("double blinds") : qsTr("manual blinds order"))
                }
                InfoItem {
                    text: qsTr("Action time: %1 sec")
                          .arg(tableStatsPage.gameInfo.playerActionTimeoutSec || 0)
                }
                InfoItem {
                    text: qsTr("Hand delay: %1 sec")
                          .arg(tableStatsPage.gameInfo.delayBetweenHandsSec || 0)
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

        // Header row of the table – columns as in RankingPage (#, player, games,
        // avg, score), so that both ranking views read the same.
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
                    onSortRequested: function(key) { tableStatsPage.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Player")
                    Layout.fillWidth: true
                    sortKey: "username"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: function(key) { tableStatsPage.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Games")
                    visible: !tableStatsPage.compact
                    Layout.preferredWidth: 70
                    horizontalAlignment: Text.AlignRight
                    sortKey: "games"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: function(key) { tableStatsPage.requestSort(key) }
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
                    onSortRequested: function(key) { tableStatsPage.requestSort(key) }
                }
                RankingHeaderCell {
                    label: qsTr("Score")
                    Layout.preferredWidth: tableStatsPage.compact ? 56 : 80
                    horizontalAlignment: Text.AlignRight
                    sortKey: "score"
                    activeKey: tableStatsPage.sortKey
                    sortOrder: tableStatsPage.sortOrder
                    onSortRequested: function(key) { tableStatsPage.requestSort(key) }
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
                // Reading without a mouse: Tab leads into the table, the arrows scroll
                // row by row (the ListView itself), page up/down and Home/End here.
                activeFocusOnTab: true
                Keys.onPressed: (event) => {
                    var maxY = Math.max(0, statsList.contentHeight - statsList.height)
                    if (event.key === Qt.Key_PageDown) {
                        statsList.contentY = Math.min(maxY, statsList.contentY + statsList.height * 0.9)
                        event.accepted = true
                    } else if (event.key === Qt.Key_PageUp) {
                        statsList.contentY = Math.max(0, statsList.contentY - statsList.height * 0.9)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Home) {
                        statsList.contentY = 0
                        event.accepted = true
                    } else if (event.key === Qt.Key_End) {
                        statsList.contentY = maxY
                        event.accepted = true
                    }
                }
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
                               ? Config.Theme.colorBox
                               : Config.StaticData.palette.secondary.col600
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        // Room for the scrollbar when it is visible.
                        anchors.rightMargin: statsList.contentHeight > statsList.height + 4 ? 16 : 10
                        spacing: 8

                        AppLabel {
                            text: statsDelegate.modelData.rank_pos
                            Layout.preferredWidth: tableStatsPage.compact ? 48 : 60
                            // Highlight the top 3 of the overall ranking.
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
                            // Clickable → player page (by player_id, otherwise username).
                            color: nickHover.hovered ? Config.Theme.colorAccent
                                                     : Config.StaticData.palette.secondary.col100
                            font.pixelSize: Config.Theme.fontSizeBody
                            font.underline: nickHover.hovered

                            HoverHandler { id: nickHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                // Open the player page of the active source (PokerTH
                                // preferably by player_id, BBC/WEC by nickname).
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

            // Empty/error notice (guests and players without a season rating are left
            // out by the server – at pure guest tables the list stays empty).
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
